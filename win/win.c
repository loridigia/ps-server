#include "win.h"

HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;

extern configuration *config;

void init(int argc, char *argv[]) {
    if (SetConsoleCtrlHandler(CtrlHandler, TRUE) == 0) {
        print_error("Impossibile aggiungere la funzione di handling per CTRL+BREAK");
        exit(1);
    }

    mutex = CreateMutexA(NULL,FALSE, GLOBAL_MUTEX);
    if (mutex == NULL) {
        print_error("Errore nell'operazione di creazione del mutex");
        exit(1);
    }

    logger_event = CreateEventA(NULL, TRUE, FALSE, LOGGER_EVENT);
    if (logger_event == NULL) {
        print_error("Errore nell'operazione di creazione del logger_event");
        exit(1);
    }

    PROCESS_INFORMATION logger_info;
    if (CreateProcess("logger.exe", NULL, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &info, &logger_info) == 0){
        print_error("Errore durante l'esecuzione del processo logger");
        exit(1);
    }

    if (WaitForSingleObject(logger_event, INFINITE) == WAIT_FAILED) {
        print_error("Errore durante l'attesa della creazione del processo logger");
        exit(1);
    }

    if (load_configuration(COMPLETE) == -1 || load_arguments(argc,argv) == -1) {
        exit(1);
    } config->main_pid = GetCurrentProcessId();

    map_handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, BUF_SIZE, GLOBAL_CONFIG);
    if (map_handle == NULL){
        print_error("Errore nel creare memory object");
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

int restart() {
    if (load_configuration(PORT_ONLY) == -1) {
        printf("error");
        return -1;
    }
    set_shared_config();
    if (start() < 0) {
        perror("Impossibile fare il restart del server. \n");
        return -1;
    }
    return 0;
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

int start(){
    if (equals(config->server_type, "thread")) {
        h_pipe = CreateFile(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (h_pipe == INVALID_HANDLE_VALUE && GetLastError() != ERROR_PIPE_BUSY) {
            print_error("Errore nella creazione della pipe");
            return -1;
        }

        HANDLE thread = CreateThread(NULL, 0, listener_routine, &config->server_port, 0, NULL);
        if (thread == NULL) {
            print_error("Errore durante la creazione del thread listener");
            return -1;
        }
    } else {
        char *arg = (char*) malloc(MAX_PORT_LENGTH);
        sprintf(arg, "%d", config->server_port);

        if (CreateProcess("listener.exe", arg, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &info, &listener_info) == 0) {
            print_error("Errore nell'eseguire il processo listener");
            return -1;
        }
    }
    return 0;
}

DWORD WINAPI listener_routine(void *args) {
    int port = *((int*)args);
    handle_requests(port, work_with_threads);
}

DWORD WINAPI receiver_routine(void *arg) {
    thread_arg_receiver *args = (thread_arg_receiver*)arg;
    serve_client(args->client_socket, args->client_ip, args->port);
    free(arg);
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

    if (listen_on(port, &server, &addrlen, &sock) < 0) {
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
            print_WSA_error("Errore durante l'operazione di select");
            return;
        }


        if(config->server_port != port) {
            printf("Chiusura socket su porta: %d", port);
            if (closesocket(sock) == SOCKET_ERROR) {
                print_error("Impossibile chiudere la socket (cambio porta)");
            }
            return;
        }

        if (selected == 0) continue;

        struct sockaddr_in address;
        SOCKET new_socket;
        if (FD_ISSET(sock , &read_fd_set)) {
            if ((new_socket = accept(sock , (struct sockaddr *)&address, (int *)&addrlen)) < 0) {
                print_WSA_error("Errore nell'accettare la richiesta del client");
                return;
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
                if (getpeername(s , (struct sockaddr*)&address , (int*)&addrlen) < 0) {
                    print_WSA_error("Impossibile ottenere l'ip del client");
                }
                char *client_ip = inet_ntoa(address.sin_addr);
                if (handle(s, client_ip, port) < 0) {
                    print_error("Errore nel comunicare con la socket");
                    if (closesocket(sock) == SOCKET_ERROR) {
                        print_error("Impossibile chiudere la socket (handle failed)");
                    }
                }
                FD_CLR(s, &read_fd_set);
                client_socket[i] = 0;
            }
        }
    }
    WSACleanup();
}

int listen_on(int port, struct sockaddr_in *server, int *addrlen, SOCKET *sock) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0) {
        print_WSA_error("Impossibile avviare la Winsock DLL");
        return -1;
    }

    if ((*sock = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET) {
        print_WSA_error("Impossibile creare la server socket");
        return -1;
    }

    server->sin_family = AF_INET;
    server->sin_addr.s_addr = INADDR_ANY;
    server->sin_port = htons(port);

    if (bind(*sock , (struct sockaddr *)server , sizeof(*server)) == SOCKET_ERROR) {
        print_WSA_error("Errore durante il binding tra socket_fd e socket_address");
        return -1;
    }

    if (listen(*sock , BACKLOG) == -1) {
        print_WSA_error("Errore nel provare ad ascoltare sulla porta data in input");
    }
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
    thread_arg_receiver *args = (thread_arg_receiver *)malloc(sizeof(thread_arg_receiver));
    if (args == NULL) {
        print_error("Impossibile allocare memoria per gli argomenti del thread di tipo 'receiver'");
        free(args);
        return -1;
    }
    args->client_socket = socket;
    args->port = port;
    args->client_ip = client_ip;

    if (CreateThread(NULL, 0, receiver_routine, (HANDLE*)args, 0, NULL) == NULL) {
        print_error("Impossibile creare un nuovo thread di tipo 'receiver'");
        free(args);
        return -1;
    }
    return 0;
}

void serve_client(SOCKET socket, char *client_ip, int port) {
    int n; char *err;
    char *client_buffer = get_client_buffer(socket, &n);

    if (n < 0) {
        err = "Errore nel ricevere i dati o richiesta mal posta";
        print_WSA_error(err);
        send_error(socket, err);
        if (closesocket(socket) == SOCKET_ERROR) {
            print_error("Impossibile chiudere la socket (get_client_buffer check)");
        }
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
            err = "Errore nell'apertura del file";
            print_error(err);
            send_error(socket, err);
            if (closesocket(socket) == SOCKET_ERROR) {
                print_error("Impossibile chiudere la socket (file opening)");
            }
            return;
        }

        DWORD size = GetFileSize(handle,NULL);
        if (size == INVALID_FILE_SIZE) {
            print_error("Impossibile prendere la size del file");
            send_error("Impossibile gestire la richiesta.\n");
            if (closesocket(socket) == SOCKET_ERROR) {
                print_error("Impossibile chiudere la socket (getting size)");
            }
            return;
        }

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
                print_error("Impossibile lockare il file");
                if (closesocket(socket) == SOCKET_ERROR) {
                    print_error("Impossibile chiudere la socket (locking check)");
                }
                return;
            }

            HANDLE file = CreateFileMappingA(handle,NULL,PAGE_READONLY,0,size,"file");
            if (file == NULL) {
                print_error("Impossibile mappare il file");
                if (closesocket(socket) == SOCKET_ERROR) {
                    print_error("Impossibile chiudere la socket (mapping check)");
                }
                success = UnlockFileEx(handle,0,size,0,&sOverlapped);
                if (!success) {
                    print_error("Impossibile unlockare il file");
                }
                return;
            }

            view = (char*)MapViewOfFile(file, FILE_MAP_READ, 0, 0, 0);
            if (view == NULL) {
                print_error("Impossibile creare la view");
                if (closesocket(socket) == SOCKET_ERROR) {
                    print_error("Impossibile chiudere la socket (view check)");
                }
                if (UnmapViewOfFile(file) == 0) {
                    print_error("Impossibile unmappare il file (view check)");
                }
                if (!UnlockFileEx(handle,0,size,0,&sOverlapped)) {
                    print_error("Impossibile unlockare il file");
                }
                return;
            }

            success = UnlockFileEx(handle,0,size,0,&sOverlapped);
            if (!success) {
                print_error("Impossibile unlockare il file");
                if (closesocket(socket) == SOCKET_ERROR) {
                    print_error("Impossibile chiudere la socket (unlocking check)");
                }
                return;
            }

        } else {
            view = "";
        }

        thread_arg_sender *args = (thread_arg_sender *)malloc(sizeof(thread_arg_sender));

        if (args == NULL) {
            print_error("Impossibile allocare memoria per gli argomenti del thread di tipo 'sender'");
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
                print_error("Impossibile creare un nuovo thread di tipo 'sender'");
                free(args);
                return;
            }
            if (WaitForSingleObject(handle, INFINITE) == WAIT_FAILED) {
                print_error("Errore durante l'attesa del thread sender");
                return -1;
            }
        }
        if (CloseHandle(handle) == 0) {
            print_error("Impossibile chiudere l'handle del file");
        }
    } else {
        char *listing_buffer;
        if ((listing_buffer = get_file_listing(client_buffer, path)) == NULL) {
            send_error(socket, "File o directory non esistente.");
            if (closesocket(socket) == SOCKET_ERROR) {
                print_error("Impossibile chiudere la socket (listing failed)");
            }
            return;
        }
        else {
            if (send(socket, listing_buffer, strlen(listing_buffer), 0) < 0) {
                print_WSA_error("Errore nel comunicare con la socket");
                if (closesocket(socket) == SOCKET_ERROR) {
                    print_error("Impossibile chiudere la socket (send failed)");
                }
                return;
            }
            if (closesocket(socket) == SOCKET_ERROR) {
                print_error("Impossibile chiudere la socket");
            }
        }
    }
}

void *send_routine(void *arg) {
    thread_arg_sender *args = (thread_arg_sender *) arg;
    if (send(args->client_socket, args->file_in_memory, args->size,0) < 0) {
        print_WSA_error("Errore nel comunicare con la socket. (sender)");
    } else {
        if (write_on_pipe(args->size, args->route, args->port, args->client_ip) < 0) {
            print_error("Errore nello scrivere sulla pipe LOG");
        }
    }

    if (args->size > 0) {
        UnmapViewOfFile(args->file_in_memory);
    }
    if (closesocket(args->client_socket) == SOCKET_ERROR) {
        print_error("Impossibile chiudere la socket (send routine)");
    }
    return NULL;
}

int write_on_pipe(int size, char* route, int port, char *client_ip) {
    if (WaitForSingleObject(mutex, INFINITE) == WAIT_FAILED) {
        print_error("Errore durante l'attesa per l'acquisizione del mutex in scrittura");
        return -1;
    }
    DWORD dw_written;
    char buffer[strlen(route) + MAX_IP_SIZE + MAX_PORT_LENGTH + MIN_LOG_SIZE];
    sprintf(buffer, "name: %s | size: %d | ip: %s | server_port: %d\n", route, size, client_ip, port);
    if (h_pipe != INVALID_HANDLE_VALUE ) {
        if (WriteFile(h_pipe, buffer, strlen(buffer), &dw_written, NULL) == FALSE) {
            print_error("Errore durante la scrittura su pipe");
        }
    }
    if (!ReleaseMutex(mutex)) {
        print_error("Impossibile rilasciare il mutex");
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

void print_error(char *err) {
    fprintf(stderr, "%s : %s\n", err, strerror(GetLastError()));
}

void print_WSA_error(char *err) {
    fprintf(stderr,"%s : %s\n",err,strerror(WSAGetLastError()));
}