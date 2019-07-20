#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/file.h>
#include <sys/mman.h>
#include "commons/commons.h"

typedef struct pthread_arg_listener {
    int port;
} pthread_arg_listener;

typedef struct pthread_arg_receiver {
    int new_socket_fd;
} pthread_arg_receiver;

typedef struct pthread_arg_sender {
    int new_socket_fd;
    char *file;
    size_t file_size;
} pthread_arg_sender;

void *pthread_routine(void *arg);
int serve_client(int client_fd);
void *pthread_sender_routine(void *arg);
void *pthread_listener_routine(void *arg);
void handle_requests(int port, int (*handle)(int, fd_set*));
int work_with_threads(int fd, fd_set *read_fd_set);
int is_get_method(char *client_buffer);
void start();
void restart();

pthread_t pthread;
pthread_attr_t pthread_attr;

int main(int argc, char *argv[]) {
    load_configuration(COMPLETE);
    load_arguments(argc, argv);

    //debugging purpose
    int pid = getpid();
    fprintf(stdout,"Pid:%d\n",pid);

    if (pthread_attr_init(&pthread_attr) != 0) {
        perror("Errore nell'inizializzazione degli attributi del thread.\n");
        exit(1);
    }

    if (pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED) != 0) {
        perror("Errore nel settare DETACHED_STATE al thread.\n");
        exit(1);
    }
    start();
    signal(SIGHUP, restart);
    while(1) {
        sleep(1);
    }
}

void start() {
    if (equals(config.type, "thread")) {
        pthread_arg_listener *pthread_arg =
                (pthread_arg_listener *)malloc(sizeof (pthread_arg_listener));
        if (!pthread_arg) {
            perror("Impossibile allocare memoria per gli argomenti del thread.\n");
            free(pthread_arg);
            exit(1);
        }
        pthread_arg->port = config.port;

        if (pthread_create(
                &pthread,
                &pthread_attr,
                pthread_listener_routine,
                (void *)pthread_arg) != 0) {
            perror("Impossibile creare un nuovo thread.\n");
            free(pthread_arg);
            exit(1);
        }
    }
}

void restart() {
    load_configuration(PORT_ONLY);
    start();
}

void *pthread_routine(void *arg) {
    pthread_arg_receiver *args = (pthread_arg_receiver *) arg;
    serve_client(args->new_socket_fd);
    free(arg);
    return NULL;
}

int serve_client(int client_fd) {

    char *client_buffer = get_client_buffer(client_fd);
    //send(client_fd, client_buffer, sizeof(client_buffer), 0); //test: bug " file/dir non esiste"

    puts(client_buffer); //test: bug " file/dir non esiste"

    char *files;
    //da allocare dinamicamente
    char listing_buffer[8192];
    bzero(listing_buffer, sizeof listing_buffer);
    char *route = strdup(client_buffer);
    char path[sizeof(PUBLIC_PATH) + strlen(route)];
    strcpy(path, PUBLIC_PATH);
    strcat(path, route);


    puts(path); //test: bug " file/dir non esiste"
    puts(route); //test: bug " file/dir non esiste"

    if (is_file(path)) {
        int file_fd = open(path, O_RDONLY);
        if (file_fd == -1) {
            char *err = "Errore nell'apertura del file.\n";
            perror(err);
            send_error(client_fd, err);
            return -1;
        }

        struct stat v;
        if (stat(path,&v) == -1) {
            perror("Errore nel prendere la grandezza del file.\n");
        }

        flock(file_fd, LOCK_EX);
        char *file_in_memory = mmap(NULL, v.st_size, PROT_READ, MAP_PRIVATE, file_fd, 0);
        if (file_in_memory == MAP_FAILED) {
            perror("Errore nell'operazione di mapping del file.\n");
        }
        flock(file_fd, LOCK_UN);

        pthread_arg_sender *pthread_sender = (pthread_arg_sender *)malloc(sizeof (pthread_arg_sender));
        if (!pthread_sender) {
            perror("Impossibile allocare memoria per gli argomenti del thread sender.\n");
            free(pthread_sender);
            return -1;
        }
        pthread_sender->new_socket_fd = client_fd;
        pthread_sender->file = file_in_memory;
        pthread_sender->file_size = v.st_size;

        if (pthread_create(&pthread, &pthread_attr, pthread_sender_routine, (void *)pthread_sender) != 0){
            perror("Impossibile creare un nuovo thread.\n");
            return -1;
        }

    } else {
        files = get_file_listing(route, path, listing_buffer);
        if (files == NULL) {
            if (send_error(client_fd, "File o directory non esistente.\n") == -1) {
                perror("Errore nel comunicare con la socket.\n");
                close(client_fd);
                return -1;
            }
        }
        else {
            if (send(client_fd, files, strlen(files), 0) == -1) {
                perror("Errore nel comunicare con la socket.\n");
                close(client_fd);
                return -1;
            }
        }
        close(client_fd);
    }
    return 0;
}

void *pthread_sender_routine(void *arg){
    pthread_arg_sender *args = (pthread_arg_sender *) arg;
    int socket_fd = args->new_socket_fd;
    char *file = args->file;
    size_t file_size = args->file_size;

    free(arg);

    char *new_str = malloc(strlen(file) + 2);
    strcpy(new_str, file);
    strcat(new_str, "\n");

    if (send(socket_fd, new_str, strlen(new_str), 0) == -1) {
        perror("Errore nel comunicare con la socket.\n");
    }
    munmap(file, file_size);
    free(new_str);
    close(socket_fd);
    return NULL;
}

void handle_requests(int port, int (*handle)(int, fd_set*)) {
    int socket_fd;
    struct sockaddr_in socket_addr;
    if (listen_on(port, &socket_fd, &socket_addr) != 0) {
        return;
    }

    fd_set read_fd_set;
    FD_ZERO (&read_fd_set);
    FD_SET (socket_fd, &read_fd_set);

    socklen_t socket_size;
    while (1) {
        if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
            perror("Errore durante l'operazione di select.\n");
            return;
        }

        for (int fd = 0; fd < FD_SETSIZE; fd++) {
            if (FD_ISSET (fd, &read_fd_set)) {
                if (fd == socket_fd) {
                    socket_size = sizeof (socket_addr);
                    int new = accept(socket_fd, (struct sockaddr *) &socket_addr, &socket_size);
                    if (new < 0) {
                        perror("Errore durante l'accept del nuovo client.\n");
                        return;
                    }
                    FD_SET (new, &read_fd_set);
                }
                else {
                    if (handle(fd, &read_fd_set) == -1) {
                        return;
                    }
                }
            }
        }
    }
}

void *pthread_listener_routine(void *arg) {
    pthread_arg_listener *args = (pthread_arg_listener *) arg;
    handle_requests(args->port, work_with_threads);
    return NULL;
}

int work_with_threads(int fd, fd_set *read_fd_set) {
    pthread_arg_receiver *pthread_arg = (pthread_arg_receiver *)malloc(sizeof (pthread_arg_receiver));
    if (!pthread_arg) {
        perror("Impossibile allocare memoria per gli argomenti di pthread.\n");
        free(pthread_arg);
        return -1;
    }
    pthread_arg->new_socket_fd = fd;

    if (pthread_create(&pthread, &pthread_attr, pthread_routine, (void *)pthread_arg) != 0) {
        perror("Impossibile creare un nuovo thread.\n");
        free(pthread_arg);
        return -1;
    }

    FD_CLR (fd, read_fd_set);
    return 0;
}
