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
    WSADATA wsa;
    SOCKET master , new_socket , client_socket[30] , s;
    struct sockaddr_in server, address;
    int max_clients = 30 , activity, addrlen, i, valread;
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

    printf("\nInitialising Winsock...");
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        printf("Failed. Error Code : %d",WSAGetLastError());
        exit(EXIT_FAILURE);
    }

    printf("Initialised.\n");

    //Create a socket
    if((master = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d" , WSAGetLastError());
        exit(EXIT_FAILURE);
    }

    printf("Socket created.\n");

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( port );

    //Bind
    if( bind(master ,(struct sockaddr *)&server , sizeof(server)) == SOCKET_ERROR)
    {
        printf("Bind failed with error code : %d" , WSAGetLastError());
        exit(EXIT_FAILURE);
    }

    puts("Bind done");

    //Listen to incoming connections
    listen(master , 3);

    //Accept and incoming connection
    puts("Waiting for incoming connections...");

    addrlen = sizeof(struct sockaddr_in);

    while(TRUE)
    {
        //clear the socket fd set
        FD_ZERO(&readfds);

        //add master socket to fd set
        FD_SET(master, &readfds);

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
        if (FD_ISSET(master , &readfds))
        {
            if ((new_socket = accept(master , (struct sockaddr *)&address, (int *)&addrlen))<0)
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
