#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BACKLOG 16

typedef struct pthread_arg_t {
    int new_socket_fd;
    struct sockaddr_in client_address;
    /* TODO: Put arguments passed to threads here. See lines 116 and 139. */
} pthread_arg_t;

typedef struct configuration {
    unsigned int port;
    char * type;
} configuration;

struct configuration config;

/* Thread routine to serve connection to client. */
void *pthread_routine(void *arg);

/* Signal handler to handle SIGTERM and SIGINT signals. */
void signal_handler(int signal_number);

char * getParameter(char * line, FILE * stream);

void workWithThreads(int socket_fd);

void workWithProcesses(int socket_fd);

void doprocessing (int socket_fd);

void read_config();

int main(int argc, char *argv[]) {
    read_config();
    int socket_fd;
    struct sockaddr_in address;

    // buona pratica inizializzare tutta la struttura con degli zeri
    bzero(&address, sizeof address);

    // famiglia di indirizzi ipv4
    address.sin_family = AF_INET;

    // assegno la porta su cui il socket deve ascoltare
    address.sin_port = htons(config.port);

    // la socket può essere associata a qualunque tipo di IP
    address.sin_addr.s_addr = INADDR_ANY;


    // creo la TCP socket (SOCK_STREAM riferisce a TCP)
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // associo l'indirizzo address alla socket
    if (bind(socket_fd, (struct sockaddr *)&address, sizeof address) == -1) {
        perror("bind");
        exit(1);
    }

    /*
     * mi metto in ascolto sulla socket
     * BACKLOG definisce il max numero di richieste in coda
    */
    if (listen(socket_fd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    /* Assign signal handlers to signals
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    */

    /* close(socket_fd);
     * TODO: If you really want to close the socket, you would do it in
     * signal_handler(), meaning socket_fd would need to be a global variable.
     */

    if (strcmp(config.type, "thread") == 0) {
        workWithThreads(socket_fd);
    } else if (strcmp(config.type, "process") == 0) {
        workWithProcesses(socket_fd);
    } else {
        perror("La modalità di avvio può essere solo 'thread' o 'process'");
        exit(1);
    }

    return 0;
}

void *pthread_routine(void *arg) {
    pthread_arg_t *pthread_arg = (pthread_arg_t *)arg;
    int new_socket_fd = pthread_arg->new_socket_fd;
    struct sockaddr_in client_address = pthread_arg->client_address;
    /* TODO: Get arguments passed to threads here. See lines 22 and 116. */

    free(arg);

    /* TODO: Put client interaction code here. For example, use
     * write(new_socket_fd,,) and read(new_socket_fd,,) to send and receive
     * messages with the client.
     */
    char buffer[128];
    recv(new_socket_fd, buffer, sizeof buffer, 0);
    puts(buffer);
    send(new_socket_fd, buffer, strlen(buffer), 0);


    close(new_socket_fd);
    return NULL;
}

void signal_handler(int signal_number) {
    /* TODO: Put exit cleanup code here. */
    exit(0);
}

void read_config() {
    FILE *stream;
    char * line = NULL;
    stream = fopen("../config.txt", "r");
    if (stream == NULL) {
        perror("Non posso aprire il file");
        exit(1);
    }
    // prendo il numero di porta
    config.port = strtoul(getParameter(line,stream), NULL, 10);
    if (config.port == 0) {
        perror("Porta errata");
        exit(1);
    }
    // prendo il tipo di avvio (thread/process)
    config.type = getParameter(line,stream);
    if (config.type == NULL) {
        perror("Errore scelta thread/process");
        exit(1);
    }
}

char * getParameter(char * line, FILE * stream) {
    size_t len;
    char * ptr;
    if (getline(&line, &len, stream) != -1) {
        strtok(line, ":");
        ptr = strtok(NULL, "\n");
        return ptr;
    }
    return NULL;
}

void workWithThreads(int socket_fd) {
    pthread_attr_t pthread_attr;
    pthread_arg_t *pthread_arg;
    pthread_t pthread;
    socklen_t client_address_len;

    // il processo viene inizializzato con attributi di default
    if (pthread_attr_init(&pthread_attr) != 0) {
        perror("pthread_attr_init");
        exit(1);
    }

    // setta l'attribute DETACHED al thread. (detached -> quando il thread muore libera tutte le risorse)
    if (pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED) != 0) {
        perror("pthread_attr_setdetachstate");
        exit(1);
    }

    while (1) {
        /* Create pthread argument for each connection to client. */
        /* TODO: malloc'ing before accepting a connection causes only one small
         * memory when the program exits. It can be safely ignored.
         */
        pthread_arg = (pthread_arg_t *)malloc(sizeof (pthread_arg_t));
        if (!pthread_arg) {
            perror("malloc");
            break;
            //continue;
        }

        /* Accept connection to client. */
        client_address_len = sizeof pthread_arg->client_address;
        int new_socket_fd = accept(socket_fd, (struct sockaddr *)&pthread_arg->client_address, &client_address_len);
        if (new_socket_fd == -1) {
            perror("accept");
            free(pthread_arg);
            continue;
        }

        /* Initialise pthread argument. */
        pthread_arg->new_socket_fd = new_socket_fd;
        /* TODO: Initialise arguments passed to threads here. See lines 22 and
         * 139.
         */

        /* Create thread to serve connection to client. */
        if (pthread_create(&pthread, &pthread_attr, pthread_routine, (void *)pthread_arg) != 0) {
            perror("pthread_create");
            free(pthread_arg);
            continue;
        }
    }
}

void workWithProcesses(int socket_fd) {
    struct sockaddr_in cli_addr;
    socklen_t clilen;
    while (1) {
        int new_socket_fd = accept(socket_fd, (struct sockaddr *) &cli_addr, &clilen);

        if (new_socket_fd < 0) {
            perror("ERROR on accept");
            exit(1);
        }

        /* Create child process */
        int pid = fork();

        if (pid < 0) {
            perror("ERROR on fork");
            exit(1);
        }

        if (pid == 0) {
            /* This is the client process */
            close(socket_fd);
            doprocessing(new_socket_fd);
            close(new_socket_fd);
            exit(0);
        }
        else {
            close(new_socket_fd);
        }

    } /* end of while */
}

void doprocessing (int sock) {
    int n;
    char buffer[256];
    bzero(buffer,256);
    n = read(sock,buffer,255);

    if (n < 0) {
        perror("ERROR reading from socket");
        exit(1);
    }

    printf("Here is the message: %s\n",buffer);
    n = write(sock,"I got your message",18);

    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }

}