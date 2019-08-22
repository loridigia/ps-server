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
    SOCKET sock = INVALID_SOCKET;
    if (listen_on(port, &sock)) {
        fprintf(stderr, "Impossibile creare la socket su porta: %d", port);
        return;
    }

}

int listen_on(int port, SOCKET *sock) {
    WSADATA wsa;
    struct addrinfo *result = NULL, hints;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        perror("Errore con WSAStartup");
        return -1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    char str_port[6];
    sprintf(str_port, "%d", port);
    if (getaddrinfo(NULL, str_port, &hints, &result) != 0 ) {
        perror("Errore con getaddrinfo");
        WSACleanup();
        return -1;
    }

    *sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (*sock == INVALID_SOCKET) {
        printf("Errore durante la creazione della socket: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return -1;
    }

    if (bind(*sock, result->ai_addr, (int)result->ai_addrlen)) {
        printf("Errore durante l'operazione di binding: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(*sock);
        WSACleanup();
        return -1;
    }

    freeaddrinfo(result);

    if (listen(*sock, BACKLOG) == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(*sock);
        WSACleanup();
        return -1;
    }
    return 0;
}

int work_with_threads(int fd, char *client_ip, int port) {

}

void log_routine() {
}

void init(int argc, char *argv[]) {
    //daemon check
    if (is_daemon(argc, argv)) {
        perror("La modalità daemon è disponibile solo sotto sistemi UNIX.");
        exit(1);
    }

    //pipe

    //mapping del config per renderlo globale

    //loading configuration
    if (load_configuration(COMPLETE) == -1 || load_arguments(argc,argv) == -1) {
        exit(1);
    }

    //mutex / condition variables

    //creazione processo per log routine

    //inizio
    start();

    //attesa di evento per restart

    //while(1) Sleep(1000);
    Sleep(3000);
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
