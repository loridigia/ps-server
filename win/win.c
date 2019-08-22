#include "win.h"
extern configuration *config;

void start(){
    if (equals(config->server_type, "thread")) {
        HANDLE thread = CreateThread(NULL, 0, listener_routine, NULL, 0, NULL);
        if (thread == NULL) {
            //handle error
        }
    } else {
        // processes
    }
}

DWORD WINAPI listener_routine(void *arg) {
    int port;
    memcpy(&port, &config->server_port, sizeof(config->server_port));
    fprintf(stdout, "%d", port);
    //handle request
}

void log_routine() {
}

void init(int argc, char *argv[]) {

    if (is_daemon(argc, argv)) {
        perror("La modalità daemon è disponibile solo sotto sistemi UNIX.");
        exit(1);
    }

    if (load_configuration(COMPLETE) == -1 || load_arguments(argc,argv) == -1) {
        exit(1);
    }

    puts(config->server_ip);

    start();
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
