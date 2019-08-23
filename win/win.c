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
    SOCKET sock;
    struct sockaddr_in server;
    int addrlen;
    if (listen_on(port, &server, &addrlen, &sock)) {
        fprintf(stderr, "Impossibile creare la socket su porta: %d", port);
        return;
    }

    SOCKET new_socket , client_socket[30] , s;
    struct sockaddr_in address;
    int max_clients = 30 , activity, i, valread;
    char *message = "it works! :D";

    //size of our receive buffer, this is string length.
    int MAXRECV = 1024;
    //set of socket descriptors
    fd_set readfds;
    //1 extra for null character, string termination
    char *buffer;
    buffer =  (char*) malloc((MAXRECV + 1) * sizeof(char));

    for(i = 0 ; i < 30;i++)
    {
        client_socket[i] = 0;
    }

    while(TRUE)
    {
        //clear the socket fd set
        FD_ZERO(&readfds);

        //add master socket to fd set
        FD_SET(sock, &readfds);

        //add child sockets to fd set
        for (  i = 0 ; i < max_clients ; i++)
        {
            s = client_socket[i];
            if(s > 0)
            {
                FD_SET( s , &readfds);
            }
        }

        //wait for an activity on any of the sockets, timeout is NULL , so wait indefinitely
        activity = select( 0 , &readfds , NULL , NULL , NULL);

        if ( activity == SOCKET_ERROR )
        {
            printf("select call failed with error code : %d" , WSAGetLastError());
            exit(EXIT_FAILURE);
        }

        //If something happened on the master socket , then its an incoming connection
        if (FD_ISSET(sock , &readfds))
        {
            if ((new_socket = accept(sock , (struct sockaddr *)&address, (int *)&addrlen)) < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            //inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

            //send new connection greeting message
            if( send(new_socket, message, strlen(message), 0) != strlen(message) )
            {
                perror("send failed");
            }

            puts("Welcome message sent successfully");

            //add new socket to array of sockets
            for (i = 0; i < max_clients; i++)
            {
                if (client_socket[i] == 0)
                {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets at index %d \n" , i);
                    break;
                }
            }
        }

        //else its some IO operation on some other socket :)
        for (i = 0; i < max_clients; i++)
        {
            s = client_socket[i];
            //if client presend in read sockets
            if (FD_ISSET( s , &readfds))
            {
                //get details of the client
                getpeername(s , (struct sockaddr*)&address , (int*)&addrlen);

                //Check if it was for closing , and also read the incoming message
                //recv does not place a null terminator at the end of the string (whilst printf %s assumes there is one).
                valread = recv( s , buffer, MAXRECV, 0);

                if( valread == SOCKET_ERROR)
                {
                    int error_code = WSAGetLastError();
                    if(error_code == WSAECONNRESET)
                    {
                        //Somebody disconnected , get his details and print
                        printf("Host disconnected unexpectedly , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

                        //Close the socket and mark as 0 in list for reuse
                        closesocket( s );
                        client_socket[i] = 0;
                    }
                    else
                    {
                        printf("recv failed with error code : %d" , error_code);
                    }
                }
                if ( valread == 0)
                {
                    //Somebody disconnected , get his details and print
                    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

                    //Close the socket and mark as 0 in list for reuse
                    closesocket( s );
                    client_socket[i] = 0;
                }

                    //Echo back the message that came in
                else
                {
                    //add null character, if you want to use with printf/puts or other string handling functions
                    buffer[valread] = '\0';
                    printf("%s:%d - %s \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port), buffer);
                    send( s , buffer , valread , 0 );
                    closesocket( s );
                    client_socket[i] = 0;
                }
            }
        }
    }

    closesocket(s);
    WSACleanup();
}

int listen_on(int port, struct sockaddr_in *server, int *addrlen, SOCKET *sock) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        printf("Failed. Error Code : %d",WSAGetLastError());
        return -1;
    }

    if((*sock = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d" , WSAGetLastError());
        return -1;
    }

    server->sin_family = AF_INET;
    server->sin_addr.s_addr = INADDR_ANY;
    server->sin_port = htons( port );

    if(bind(*sock , (struct sockaddr *)server , sizeof(*server)) == SOCKET_ERROR)
    {
        printf("Bind failed with error code : %d" , WSAGetLastError());
        return -1;
    }

    listen(*sock , BACKLOG);
    *addrlen = sizeof(struct sockaddr_in);
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
