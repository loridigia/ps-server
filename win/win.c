#include "win.h"

extern configuration *config;

void start(){
    if (equals(config->server_type, "thread")) {
        HANDLE thread = CreateThread(NULL, 0, listener_routine, &config->server_port, 0, NULL);
        if (thread == NULL) {
            //handle error
        }
    } else {
        // processes
    }
}

DWORD WINAPI listener_routine(LPVOID param) {
    int port = *((int*)param);
    handle_requests(port, work_with_threads);
}

void handle_requests(int port, int (*handle)(int, char*, int)) {
    SOCKET sock, client_socket[BACKLOG];
    struct sockaddr_in server;
    int addrlen;

    if (listen_on(port, &server, &addrlen, &sock)) {
        fprintf(stderr, "Impossibile creare la socket su porta: %d", port);
        return;
    }

    for(int i = 0 ; i < BACKLOG;i++) {
        client_socket[i] = 0;
    }

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    fd_set read_fd_set;
    while(TRUE) {
        FD_ZERO(&read_fd_set);
        FD_SET(sock, &read_fd_set);

        for (int i = 0 ; i < BACKLOG ; i++) {
            SOCKET s = client_socket[i];
            if(s > 0) FD_SET(s, &read_fd_set);
        }

        if (select(0 , &read_fd_set , NULL , NULL , &timeout) == SOCKET_ERROR) {
            printf("select call failed with error code : %d" , WSAGetLastError());
            exit(EXIT_FAILURE);
        }

        if(config->server_port != port) {
            printf("Chiusura socket su porta %d. \n", port);
            break;
        }

        struct sockaddr_in address;
        SOCKET new_socket;
        if (FD_ISSET(sock , &read_fd_set)) {
            if ((new_socket = accept(sock , (struct sockaddr *)&address, (int *)&addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            for (int i = 0; i < BACKLOG; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    break;
                }
            }
        }

        for (int i = 0; i < BACKLOG; i++) {
            SOCKET s = client_socket[i];
            if (FD_ISSET(s, &read_fd_set)) {
                getpeername(s , (struct sockaddr*)&address , (int*)&addrlen);
                char *client_ip = inet_ntoa(address.sin_addr);
                char buffer[1024];
                int valread = recv( s , buffer, sizeof(buffer), 0);

                if (valread == SOCKET_ERROR) {
                    if(WSAGetLastError() == WSAECONNRESET) {
                        fprintf(stderr,"Errore nel comunicare con la socket %d. Client-ip: %s", s, client_ip);
                        closesocket( s );
                        client_socket[i] = 0;
                    }
                } else if (valread == 0) {
                    closesocket(s);
                    client_socket[i] = 0;
                } else {
                    printf("%s: %s \n" , client_ip, buffer);
                    closesocket(s);
                    client_socket[i] = 0;
                }
            }
        }
    }
    WSACleanup();
}

int listen_on(int port, struct sockaddr_in *server, int *addrlen, SOCKET *sock) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0) {
        printf("Failed. Error Code : %d",WSAGetLastError());
        return -1;
    }

    if ((*sock = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET) {
        printf("Could not create socket : %d" , WSAGetLastError());
        return -1;
    }

    server->sin_family = AF_INET;
    server->sin_addr.s_addr = INADDR_ANY;
    server->sin_port = htons( port );

    if (bind(*sock , (struct sockaddr *)server , sizeof(*server)) == SOCKET_ERROR) {
        printf("Bind failed with error code : %d" , WSAGetLastError());
        return -1;
    }

    listen(*sock , BACKLOG);
    *addrlen = sizeof(struct sockaddr_in);
    return 0;
}

int work_with_threads(int fd, char *client_ip, int port) {

}

int write_on_pipe(char *buffer) {
    if (h_pipe != INVALID_HANDLE_VALUE){
        WriteFile(h_pipe, buffer, 12, &dw_written, NULL);
        CloseHandle(h_pipe);
    }
}

void init(int argc, char *argv[]) {
    //daemon check
    if (is_daemon(argc, argv)) {
        perror("La modalità daemon è disponibile solo sotto sistemi UNIX.");
        exit(1);
    }

    //logger process
    if (CreateProcess("logger.exe", NULL, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &info, &process_info) == 0){
        perror("Errore nell'eseguire il processo logger");
        exit(1);
    } else {
        /*
        // distruggi il processo e il suo main thread
        CloseHandle(process_info.hProcess);
        CloseHandle(process_info.hThread);
         */
    }

    //create pipe
    h_pipe = CreateFile(pipename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    write_on_pipe("pippo");
    //mapping del config per renderlo globale

    //loading configuration
    if (load_configuration(COMPLETE) == -1 || load_arguments(argc,argv) == -1) {
        perror("Errore nel caricare la configurazione");
        exit(1);
    }

    //mutex / condition variables

    //creazione processo per log routine

    //inizio
    start();

    //attesa di evento per restart

    while(1) Sleep(1000);
    //Sleep(3000);
}

char *get_server_ip(){
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
    return ip;
}

/* This code is public domain -- Will Hartung 4/9/09 */
size_t getline(char **lineptr, size_t *n, FILE *stream) {
    char *bufptr = NULL;
    char *p = bufptr;
    size_t size;
    int c;

    if (lineptr == NULL) {
        return -1;
    }
    if (stream == NULL) {
        return -1;
    }
    if (n == NULL) {
        return -1;
    }
    bufptr = *lineptr;
    size = *n;

    c = fgetc(stream);
    if (c == EOF) {
        return -1;
    }
    if (bufptr == NULL) {
        bufptr = malloc(128);
        if (bufptr == NULL) {
            return -1;
        }
        size = 128;
    }
    p = bufptr;
    while(c != EOF) {
        if ((p - bufptr) > (size - 1)) {
            size = size + 128;
            bufptr = realloc(bufptr, size);
            if (bufptr == NULL) {
                return -1;
            }
        }
        *p++ = c;
        if (c == '\n') {
            break;
        }
        c = fgetc(stream);
    }

    *p++ = '\0';
    *lineptr = bufptr;
    *n = size;

    return p - bufptr - 1;
}
