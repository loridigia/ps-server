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
    char *mem_file;
} pthread_arg_sender;

void *pthread_routine(void *arg);

void *pthread_send_file(void *arg);

void *pthread_listener_routine(void *arg);

void handle_requests(int port, int (*handle)(int, fd_set*));

int work_with_threads(int fd, fd_set *read_fd_set);

configuration config;
pthread_t pthread;
pthread_attr_t pthread_attr;

int main(int argc, char *argv[]) {
    load_arguments(&config, argc, argv);
    if (load_configuration(&config) == -1) {
        exit(1);
    }

    if (pthread_attr_init(&pthread_attr) != 0) {
        perror("Errore nell'inizializzazione degli attributi del thread.\n");
        exit(1);
    }

    if (pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED) != 0) {
        perror("Errore nel settare DETACHED_STATE al thread.\n");
        exit(1);
    }

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
    // for debugging purpose
    while(1);
}

char *get_file_listing(char *route, char *path, char *buffer) {
    DIR *d;
    struct dirent *dir;

    if ((d = opendir (path)) != NULL) {
        while ((dir = readdir (d)) != NULL) {
            if (equals(dir->d_name, ".") || equals(dir->d_name, "..")) {
                continue;
            }
            strcat(buffer, get_extension_code(dir->d_name));
            strcat(buffer, dir->d_name);
            strcat(buffer, "\t");
            strcat(buffer, route);
            if (path[strlen(path)-1] != '/') {
                strcat(buffer, "/");
            }
            strcat(buffer,"\t");
            strcat(buffer,config.ip_address);
            strcat(buffer,"\t");
            char port[6];
            sprintf(port,"%d",config.port);
            strcat(buffer,port);
            strcat(buffer, "\n");
        }
        strcat(buffer, ".\n");
        rewinddir(d);
        closedir (d);
    }
    else {
        perror("Impossibile aprire la directory.\n");
        return NULL;
    }
    return buffer;
}

void *pthread_routine(void *arg) {
    pthread_arg_receiver *args = (pthread_arg_receiver *) arg;
    int socket_fd = args->new_socket_fd;

    free(arg);
    char client_buffer[512];
    bzero(client_buffer, sizeof client_buffer);

    if (recv(socket_fd, client_buffer, sizeof client_buffer, 0) == -1) {
        perror("Errore nel ricevere dati dalla socket.\n");
    }

    char method[4];
    memcpy(method, &client_buffer, 3);
    method[3] = '\0';

    if (!(equals(method,"GET"))) {
        if (send_error(socket_fd, "Il server accetta solo richieste di tipo GET.\n") == -1) {
            perror("Errore nel comunicare con la socket. (accetta solo conn. GET)\n");
        }
        close(socket_fd);
        return NULL;
    }

    char *files;
    char listing_buffer[4096];
    bzero(listing_buffer, sizeof listing_buffer);
    char *route = extract_route(client_buffer);

    char path[128];
    strcpy(path, PUBLIC_PATH);
    strcat(path, route);

    if (is_file(path)) {

        /*read file */
        int fd = open(path, O_RDONLY);
        if (fd == -1) {
            char *err = "Errore nell'apertura del file.\n";
            perror(err);
            send_error(socket_fd, err);
            //exit(1);
        }

        struct stat v;
        if (stat(path,&v) == -1) {
            perror("Errore nel prendere la grandezza del file.\n");
        }
        /*map file in memory */
        flock(fd, LOCK_EX);
        char *file_in_memory = mmap(NULL, v.st_size, PROT_READ, MAP_SHARED, fd, 0);
        flock(fd, LOCK_UN);
        // controllare errore
        // ma il file va unmappato?



        /*create thread and send file */
        pthread_arg_sender *pthread_file = (pthread_arg_sender *)malloc(sizeof (pthread_arg_sender));
        if (!pthread_file) {
            perror("Impossibile allocare memoria per gli argomenti di pthread.\n");
            free(pthread_file);
            return NULL;
        }
        pthread_file->new_socket_fd = socket_fd;
        pthread_file->mem_file = file_in_memory;

        if (pthread_create(&pthread, &pthread_attr, pthread_send_file, (void *)pthread_file) != 0){
            perror("Impossibile creare un nuovo thread.\n");
        }

    } else {
        files = get_file_listing(route, path, listing_buffer);
        if (files == NULL) {
            if (send_error(socket_fd, "Directory non esistente.\n") == -1) {
                perror("Errore nel comunicare con la socket.\n");
            }
        }
        else {
            if (send(socket_fd, files, strlen(files), 0) == -1) {
                perror("Errore nel comunicare con la socket. (invio lista files)\n");
            }
        }
        close(socket_fd);
    }
    return NULL;

}

void *pthread_send_file(void *arg){
    pthread_arg_sender *args = (pthread_arg_sender *) arg;
    int socket_fd = args->new_socket_fd;
    char *file_mem = args->mem_file;
    //int file_fd = args->file_fd;

    free(arg);

    char *new_str = malloc(strlen(file_mem) + 2);
    strcpy(new_str, file_mem);
    strcat(new_str, "\n");

    if (send(socket_fd, new_str, strlen(new_str), 0) == -1) {
        perror("dddErrore nel comunicare con la socket. (invio lista files)\n");
    }
    // controllo errori
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
