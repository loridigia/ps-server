#include "win.h"

HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;

extern configuration *config;

void init(int argc, char *argv[]) {
    if (SetConsoleCtrlHandler(CtrlHandler, TRUE) == 0) {
        perror("Impossibile aggiungere la funzione di handling per CTRL+BREAK. \n");
        exit(1);
    }

    mutex = CreateMutexA(NULL,FALSE, GLOBAL_MUTEX);
    if (mutex == NULL) {
        perror("Errore nell'operazione di creazione del mutex. \n");
        exit(1);
    }

    logger_event = CreateEventA(NULL, TRUE, FALSE, LOGGER_EVENT);
    if (logger_event == NULL) {
        perror("Errore nell'operazione di creazione del logger_event. \n");
        exit(1);
    }

    PROCESS_INFORMATION logger_info;
    if (CreateProcess("logger.exe", NULL, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &info, &logger_info) == 0){
        perror("Errore durante l'esecuzione del processo logger. \n");
        exit(1);
    }

    if (WaitForSingleObject(logger_event, INFINITE) == WAIT_FAILED) {
        perror("Errore durante l'attesa della creazione del processo logger. \n");
        exit(1);
    }

    if (load_configuration(COMPLETE) == -1 || load_arguments(argc,argv) == -1) {
        exit(1);
    } config->main_pid = GetCurrentProcessId();

    map_handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, BUF_SIZE, GLOBAL_CONFIG);
    if (map_handle == NULL){
        perror("Errore nel creare memory object. \n");
        exit(1);
    }

    if (set_shared_config() == -1) {
        exit(1);
    }

    start();

    if (write_infos() == -1) {
        exit(1);
    }

    while(1) Sleep(1000);
}

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType){
    if (fdwCtrlType == CTRL_BREAK_EVENT) {
        restart();
        return TRUE;
    }
}

void restart() {
    if (load_configuration(PORT_ONLY) == -1) {
        printf("error");
    }
<
    set_shared_config();
    start();

}

int set_shared_config(){
    LPCTSTR pBuf = (LPTSTR) MapViewOfFile(map_handle, FILE_MAP_ALL_ACCESS, 0, 0, BUF_SIZE);
    if (pBuf == NULL){
        perror("Errore nel mappare la view del file. (init) \n");
        CloseHandle(map_handle);
        return -1;
    }

    CopyMemory((PVOID)pBuf, config, sizeof(configuration));
    if ((FlushViewOfFile(pBuf, sizeof(configuration)) && UnmapViewOfFile(pBuf)) == 0) {
        perror("Errore durante l'operazione di flushing / unmapping della view del file. \n");
        return -1;
    }

    return 0;
}



void start(){
    if (equals(config->server_type, "thread")) {
        h_pipe = CreateFile(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (h_pipe == INVALID_HANDLE_VALUE && GetLastError() != ERROR_PIPE_BUSY) {
            perror("Errore nella creazione della pipe");
            exit(1);
        }

        HANDLE thread = CreateThread(NULL, 0, listener_routine, &config->server_port, 0, NULL);
        if (thread == NULL) {
            perror("Errore nel creare il thread listener");
            exit(1);
        }
    } else {
        char *arg = malloc(MAX_PORT_LENGTH);
        sprintf(arg, "%d", config->server_port);

        if (CreateProcess("listener.exe", arg, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &info, &listener_info) == 0){
            perror("Errore nell'eseguire il processo listener");
            exit(1);
        }
    }
}

DWORD WINAPI listener_routine(void *args) {
    int port = *((int*)args);
    handle_requests(port, work_with_threads);
}

DWORD WINAPI receiver_routine(void *args) {
    thread_arg_receiver *params = (thread_arg_receiver*)args;
    serve_client(params->client_socket, params->client_ip, params->port);
    free(args);
}

DWORD WINAPI sender_routine(void *args) {
    thread_arg_sender *arg = (thread_arg_sender*)args;
    send_routine(arg);
    free(args);
}

void handle_requests(int port, int (*handle)(SOCKET, char*, int)) {
    SOCKET sock, client_socket[BACKLOG];
    struct sockaddr_in server;
    int addrlen;


    if (listen_on(port, &server, &addrlen, &sock)) {
        printf("Impossibile creare la socket su porta: %d", port);
        return;
    }

    for(int i = 0 ; i < BACKLOG;i++) {
        client_socket[i] = 0;
    }

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    fd_set read_fd_set;
    while(TRUE) {

        FD_ZERO(&read_fd_set);
        FD_SET(sock, &read_fd_set);

        int selected;
        if ((selected = select(0 , &read_fd_set , NULL , NULL , &timeout)) == SOCKET_ERROR) {
            perror("Errore durante l'operazione di select.\n");
            exit(EXIT_FAILURE);
        }


        if(config->server_port != port) {
            printf("Chiusura socket su porta %d. \n", port);
            closesocket(sock);
            return;
        }

        if (selected == 0) continue;

        struct sockaddr_in address;
        SOCKET new_socket;
        if (FD_ISSET(sock , &read_fd_set)) {
            if ((new_socket = accept(sock , (struct sockaddr *)&address, (int *)&addrlen)) < 0) {
                perror("Errore nell'accettare la richiesta del client\n");
                exit(EXIT_FAILURE);
            }

            for (int i = 0; i < BACKLOG; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    FD_SET(new_socket, &read_fd_set);
                    break;
                }
            }
        }

        for (int i = 0; i < BACKLOG; i++) {
            SOCKET s = client_socket[i];
            if (FD_ISSET(s, &read_fd_set)) {
                getpeername(s , (struct sockaddr*)&address , (int*)&addrlen);
                char *client_ip = inet_ntoa(address.sin_addr);
                if (handle(s, client_ip, port) < 0) {
                    fprintf(stderr,"Errore nel comunicare con la socket %d. Client-ip: %s", s, client_ip);
                    closesocket(s);
                }
                client_socket[i] = 0;
            }
        }
    }
    WSACleanup();
}

int listen_on(int port, struct sockaddr_in *server, int *addrlen, SOCKET *sock) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0) {
        printf("Failed. Error Code : %d", WSAGetLastError());
        return -1;
    }

    if ((*sock = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET) {
        printf("Could not create socket : %d", WSAGetLastError());
        return -1;
    }

    server->sin_family = AF_INET;
    server->sin_addr.s_addr = INADDR_ANY;
    server->sin_port = htons(port);

    if (bind(*sock , (struct sockaddr *)server , sizeof(*server)) == SOCKET_ERROR) {
        printf("Bind failed with error code : %d" , WSAGetLastError());
        return -1;
    }

    listen(*sock , BACKLOG);
    *addrlen = sizeof(struct sockaddr_in);
    return 0;
}

int work_with_processes(SOCKET socket, char *client_ip, int port){
    char *args = malloc(MAX_PORT_LENGTH + MAX_IP_SIZE + 2);
    sprintf(args, "%d %s", port, client_ip);

    SECURITY_ATTRIBUTES saAttr;

    // Set the bInheritHandle flag so pipe handles are inherited.
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Create a pipe for the child process's STDIN.
    if (! CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
        perror("Errore creazione pipe STDIN");

    // Ensure the write handle to the pipe for STDIN is not inherited.
    if (! SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) )
        perror("Stdin SetHandleInformation");


    WSAPROTOCOL_INFO protocol_info;
    DWORD dwWrite;
    DWORD child_id;

    child_id = create_receiver_process(args);

    WSADuplicateSocketA(socket, child_id, &protocol_info);
    if (! WriteFile(g_hChildStd_IN_Wr, &protocol_info, sizeof(protocol_info), &dwWrite, NULL)){
        perror("errore nello scrivere su pipe STD_IN");
        exit(1);
    }

    closesocket(socket);
}

DWORD create_receiver_process(char *args){
    PROCESS_INFORMATION receiver_info;
    STARTUPINFO si_start_info;

    ZeroMemory( &receiver_info, sizeof(PROCESS_INFORMATION) );

    ZeroMemory( &si_start_info, sizeof(STARTUPINFO) );
    si_start_info.cb = sizeof(STARTUPINFO);
    si_start_info.hStdInput = g_hChildStd_IN_Rd;
    si_start_info.dwFlags |= STARTF_USESTDHANDLES;

    if (CreateProcess("receiver.exe", args, NULL, NULL, TRUE, 0, NULL, NULL, &si_start_info, &receiver_info) == 0) {
        printf("-%lu-", GetLastError());
        perror("Errore nell'eseguire il processo receiver");
        exit(1);
    }

    return receiver_info.dwProcessId;
}

int work_with_threads(SOCKET socket, char *client_ip, int port) {
    thread_arg_receiver *args =
            (thread_arg_receiver *)malloc(sizeof(thread_arg_receiver));
    args->client_socket = socket;
    args->port = port;
    args->client_ip = client_ip;
    if (CreateThread(NULL, 0, receiver_routine, (HANDLE*)args, 0, NULL) == NULL) {
        perror("Impossibile creare un nuovo thread di tipo 'receiver'.\n");
        free(args);
        return -1;
    }
    return 0;
}

void serve_client(SOCKET socket, char *client_ip, int port) {
    int n; char *err;

    char *client_buffer = get_client_buffer(socket, &n);

    if (n < 0) {
        err = "Errore nel ricevere i dati o richiesta mal posta.\n";
        fprintf(stderr,"%s",err);
        send_error(socket, err);
        closesocket(socket);
        return;
    }

    char path[strlen(PUBLIC_PATH) + strlen(client_buffer) + 1];
    sprintf(path,"%s%s", PUBLIC_PATH, client_buffer);

    BOOL success = FALSE;
    if (is_file(path) != 0) {
        HANDLE handle = CreateFile(TEXT(path),
                                   GENERIC_READ | GENERIC_WRITE,
                                   0,
                                   NULL,
                                   OPEN_EXISTING,
                                   0,
                                   NULL);

        if (handle == INVALID_HANDLE_VALUE) {
            err = "Errore nell'apertura del file. \n";
            fprintf(stderr,"%s%s",err,path);
            send_error(socket, err);
            closesocket(socket);
            return;
        }
        DWORD size = GetFileSize(handle,NULL);
        char *view;
        if (size > 0) {
            OVERLAPPED sOverlapped;
            sOverlapped.Offset = 0;
            sOverlapped.OffsetHigh = 0;

            success = LockFileEx(handle,
                                 LOCKFILE_EXCLUSIVE_LOCK |
                                 LOCKFILE_FAIL_IMMEDIATELY,
                                 0,
                                 size,
                                 0,
                                 &sOverlapped);

            if (!success) {
                perror("Impossibile lockare il file.\n");
                closesocket(socket);
                return;
            }

            HANDLE file = CreateFileMappingA(handle,NULL,PAGE_READONLY,0,size,"file");
            if (file == NULL) {
                perror("Impossibile mappare il file.\n");
                closesocket(socket);
                return;
            }

            view = (char*)MapViewOfFile(file, FILE_MAP_READ, 0, 0, 0);
            if (view == NULL) {
                perror("Impossibile creare la view.\n");
                closesocket(socket);
                return;
            }

            success = UnlockFileEx(handle,0,size,0,&sOverlapped);
            if (!success) {
                perror("Impossibile unlockare il file.\n");
                closesocket(socket);
                return;
            }

        } else {
            view = "";
        }

        thread_arg_sender *args = (thread_arg_sender *)malloc(sizeof(thread_arg_sender));

        if (args < 0) {
            perror("Impossibile allocare memoria per gli argomenti del thread di tipo 'sender'.\n");
            free(args);
            return;
        }

        args->client_ip = client_ip;
        args->client_socket = socket;
        args->port = port;
        args->file_in_memory = view;
        args->route = client_buffer;
        args->size = size;

        if (equals(config->server_type, "thread")) {
            send_routine(args);
        } else if (equals(config->server_type, "process")) {
            HANDLE handle = CreateThread(NULL, 0, sender_routine, (HANDLE*)args, 0, NULL);
            if (handle == NULL) {
                perror("Impossibile creare un nuovo thread di tipo 'listener'.\n");
                free(args);
                return;
            }
            WaitForSingleObject(handle, INFINITE);
        }
        CloseHandle(handle);
    } else {
        char *listing_buffer;
        if ((listing_buffer = get_file_listing(client_buffer, path)) == NULL) {
            err = "File o directory non esistente.\n";
            fprintf(stderr,"%s",err);
            send_error(socket, err);
            closesocket(socket);
            return;
        }
        else {
            if (send(socket, listing_buffer, strlen(listing_buffer), 0) < 0) {
                perror("Errore nel comunicare con la socket.\n");
                closesocket(socket);
                return;
            }
            closesocket(socket);
        }
    }
}

void *send_routine(void *arg) {
    thread_arg_sender *args = (thread_arg_sender *) arg;
    if (send(args->client_socket, args->file_in_memory, args->size,0) < 0){
        fprintf(stderr, "Errore nel comunicare con la socket. ('sender')\n");
    } else {
        if (write_on_pipe(args->size, args->route, args->port, args->client_ip) < 0) {
            fprintf(stderr, "Errore nello scrivere sulla pipe LOG.\n");
            return NULL;
        }
    }

    if (args->size > 0) {
        UnmapViewOfFile(args->file_in_memory);
    }
    closesocket(args->client_socket);
    return NULL;
}

int write_on_pipe(int size, char* name, int port, char *client_ip) {
    WaitForSingleObject(mutex,INFINITE);
    DWORD dw_written;
    char *buffer = malloc(strlen(name) + sizeof(size) + strlen(client_ip) + sizeof(port) + CHUNK);
    sprintf(buffer, "name: %s | size: %d | ip: %s | server_port: %d\n", name, size, client_ip, port);
    if (h_pipe != INVALID_HANDLE_VALUE ) {
        if (WriteFile(h_pipe, buffer, strlen(buffer), &dw_written, NULL) == FALSE ){
            printf("ERRORE WRITEFILE %lu", GetLastError());
        }
    }
    if (!ReleaseMutex(mutex)) {
        fprintf(stderr, "Impossibile rilasciare il mutex.\n");
    }
}

char *get_server_ip(){
    /*
    WORD wVersionRequested;
    WSADATA wsaData;
    char name[255];
    char *ip = NULL;
    PHOSTENT hostinfo;
    wVersionRequested = MAKEWORD( 2, 0 );
    if ( WSAStartup( wVersionRequested, &wsaData ) == 0 ){
        if( gethostname ( name, sizeof(name)) == 0){
            if((hostinfo = gethostbyname(name)) != NULL){
                ip = inet_ntoa (*(struct in_addr *)*hostinfo->h_addr_list);
            }
        }
        WSACleanup();
    }
     */
    char *ip = (char*) malloc(256);
    return ip;
}

int send_error(SOCKET socket, char *err) {
    return send(socket, err, strlen(err), 0);
}

int _recv(int s,char *buf,int len,int flags) {
    return recv(s,buf,len,flags);
}

void get_shared_config(configuration *configuration){
    HANDLE handle_mapped_file;

    handle_mapped_file = OpenFileMapping(FILE_MAP_READ, FALSE, GLOBAL_CONFIG);
    if (handle_mapped_file == NULL){
        perror("Errore nell'aprire memory object listener");
        CloseHandle(handle_mapped_file);
        exit(1);
    }
    config = MapViewOfFile(handle_mapped_file, FILE_MAP_READ, 0, 0, BUF_SIZE);
    if (config == NULL){
        perror("Errore nel mappare la view del file");
        CloseHandle(handle_mapped_file);
        exit(1);
    }

    CloseHandle(handle_mapped_file);
}