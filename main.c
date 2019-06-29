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
#define PORT 7070

typedef struct pthread_arg_t {
    int new_socket_fd;
    struct sockaddr_in client_address;
    /* TODO: Put arguments passed to threads here. See lines 116 and 139. */
} pthread_arg_t;

/* Thread routine to serve connection to client. */
void *pthread_routine(void *arg);

/* Signal handler to handle SIGTERM and SIGINT signals. */
void signal_handler(int signal_number);

void read_config();

const char *config[2];

int main(int argc, char *argv[]) {
    read_config();
    int port, socket_fd, new_socket_fd;
    struct sockaddr_in address;
    pthread_attr_t pthread_attr;
    pthread_arg_t *pthread_arg;
    pthread_t pthread;
    socklen_t client_address_len;

    // buona pratica inizializzare tutta la struttura con degli zeri
    memset(&address, 0, sizeof address);

    // famiglia di indirizzi ipv4
    address.sin_family = AF_INET;

    // assegno la porta su cui il socket deve ascoltare
    address.sin_port = htons(PORT);

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

    // attesa
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
        new_socket_fd = accept(socket_fd, (struct sockaddr *)&pthread_arg->client_address, &client_address_len);
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

    /* close(socket_fd);
     * TODO: If you really want to close the socket, you would do it in
     * signal_handler(), meaning socket_fd would need to be a global variable.
     */
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
    //free(buffer);


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
    size_t len;
    stream = fopen("../config.txt", "r");
    if (stream == NULL) {
        perror("Non posso aprire il file");
        exit(1);
    }

    const char * ptr = NULL;
    int i = 0;

    while (getline(&line, &len, stream) != -1) {
        strtok(line, ":");
        ptr = strtok(NULL, "\n");
        config[i] = strdup(ptr);
        i++;
    }
}