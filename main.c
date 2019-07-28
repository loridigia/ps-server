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
    int port;
    char *client_ip;
} pthread_arg_receiver;


int serve_client(int client_fd, int port, char *client_ip);
int write_on_pipe(int size, char *name, int port, char *ip);
int work_with_threads(int fd, fd_set *read_fd_set, char *client_ip);
void *pthread_receiver_routine(void *arg);
void *pthread_listener_routine(void *arg);
void handle_requests(int port, int (*handle)(int, fd_set*, char*));
void start();
void restart();
void write_log();

pthread_t pthread;
pthread_attr_t pthread_attr;

pthread_mutex_t mutex;
pthread_cond_t condition;

int pipe_fd[2];
int counter = 0;

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

    if (pipe(pipe_fd) == -1){
        fprintf(stderr, "Pipe Failed" );
        return 1;
    }

    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&mutex, &mutexAttr);

    pthread_condattr_t condAttr;
    pthread_condattr_setpshared(&condAttr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&condition, &condAttr);

    pid_t pid_child = fork();

    if (pid_child < 0) {
        perror("fork");
        exit(0);
    }

    if (pid_child == 0) {
        write_log();
    }

    start();
    signal(SIGHUP, restart);
    while(1) {
        sleep(1);
    }
}

void write_log() {
    while(1) {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&condition, &mutex);
        char buffer[1024];
        bzero(buffer, sizeof buffer);

        read(pipe_fd[0], buffer, sizeof buffer);
        FILE *file = fopen(LOG_PATH, "a");

        fprintf(file, "%s", buffer);
        fclose(file);

        pthread_mutex_unlock(&mutex);
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
        } else {
            counter++;
        }
    }
}

void restart() {
    load_configuration(PORT_ONLY);
    start();
}

void *pthread_listener_routine(void *arg) {
    pthread_arg_listener *args = (pthread_arg_listener *) arg;
    handle_requests(args->port, work_with_threads);
    return NULL;
}

void handle_requests(int port, int (*handle)(int, fd_set*, char*)){
    int socket_fd;
    struct sockaddr_in socket_addr;
    if (listen_on(port, &socket_fd, &socket_addr) != 0) {
        return;
    }

    fd_set read_fd_set;
    socklen_t socket_size;
    while (1) {
        FD_ZERO (&read_fd_set);
        FD_SET (socket_fd, &read_fd_set);

        if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
            perror("Errore durante l'operazione di select.\n");
            return;
        }

        if(config.port != port) {
            fprintf(stderr, "porta diversa, interrompo listing \n");
            break;
        }

        for (int fd = 0; fd < FD_SETSIZE; fd++) {
            if (FD_ISSET (fd, &read_fd_set)) {
                if (fd == socket_fd) {
                    socket_size = sizeof(socket_addr);
                    int new = accept(socket_fd, (struct sockaddr *) &socket_addr, &socket_size);
                    if (new < 0) {
                        perror("Errore durante l'accept del nuovo client.\n");
                        return;
                    }
                    FD_SET (new, &read_fd_set);
                } else {
                    char *client_ip = get_client_ip(&socket_addr);
                    if (handle(fd, &read_fd_set, client_ip) == -1) {
                        return;
                    }
                }
            }
        }
    }
    close(socket_fd);
}


int work_with_threads(int fd, fd_set *read_fd_set, char *client_ip) {
    pthread_arg_receiver *pthread_arg = (pthread_arg_receiver *)malloc(sizeof (pthread_arg_receiver));
    if (!pthread_arg) {
        perror("Impossibile allocare memoria per gli argomenti di pthread.\n");
        free(pthread_arg);
        return -1;
    }
    pthread_arg->new_socket_fd = fd;
    pthread_arg->port = config.port;
    pthread_arg->client_ip = client_ip;
    pthread_t pthread_rcv;

    if (pthread_create(&pthread_rcv, &pthread_attr, pthread_receiver_routine, (void *) pthread_arg) != 0) {
        perror("Impossibile creare un nuovo thread.\n");
        free(pthread_arg);
        return -1;
    }

    FD_CLR (fd, read_fd_set);
    return 0;
}


void *pthread_receiver_routine(void *arg) {
    pthread_arg_receiver *args = (pthread_arg_receiver *) arg;
    serve_client(args->new_socket_fd, args->port, args->client_ip);
    free(arg);
    return NULL;
}

int write_on_pipe(int size, char* name, int port, char *ip){
    pthread_mutex_lock(&mutex);
    char *buffer = malloc(strlen(name) + 7 + sizeof(size) + 12 + strlen(ip) + 11 + sizeof(port) + 8);
    sprintf(buffer, "name: %s\nfile_size: %d\nclient_ip: %s\nport: %d\n\n", name, size, ip, port);
    if (write(pipe_fd[1], buffer, strlen(buffer)) == -1){
        pthread_mutex_unlock(&mutex);
        return -1;
    } else {
        pthread_cond_signal(&condition);
        pthread_mutex_unlock(&mutex);
    }
    return 0;
}

int serve_client(int client_fd, int port, char *client_ip) {

    // da cambiare
    char *client_buffer = get_client_buffer(client_fd);
    int end = index_of(client_buffer, '\r');

    char *files;

    char route[end+1];
    strncpy(route, client_buffer, sizeof route-1);
    route[sizeof route-1] = '\0';

    char path[sizeof(PUBLIC_PATH) + strlen(route)];
    strcpy(path, PUBLIC_PATH);
    strcat(path, route);

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

        int size = v.st_size;
        if (write_on_pipe(size, route, port, client_ip) != 0) {
            perror("Errore nello scrivere sulla pipe LOG.\n");
        }

        if (size == 0) {
            char *err = "Il file richiesto è vuoto.\n";
            perror(err);
            send_error(client_fd, err);
            return -1;
        }

        flock(file_fd, LOCK_EX);
        char *file_in_memory = mmap(NULL, v.st_size, PROT_READ, MAP_PRIVATE, file_fd, 0);
        if (file_in_memory == MAP_FAILED) {
            perror("Errore nell'operazione di mapping del file.\n");
        }
        flock(file_fd, LOCK_UN);

        if (send_file(client_fd, file_in_memory, v.st_size) != 0){
            perror("Impossibile creare un nuovo thread.\n");
            return -1;
        }

    } else {
        //da fare dinamico
        char listing_buffer[2048];
        bzero(listing_buffer, sizeof listing_buffer);
        files = get_file_listing(route, path, listing_buffer);
        if (files == NULL) {
            if (send_error(client_fd, "File o directory non esistente.\n") != 0) {
                perror("Errore nel comunicare con la socket.\n");
                close(client_fd);
                return -1;
            }
        }
        else {
            if (send(client_fd, listing_buffer, sizeof listing_buffer, 0) != 0) {
                if ((errno != EAGAIN || errno != EWOULDBLOCK)) { 
                    perror("Errore nel comunicare con la socket.\n");
                    close(client_fd);
                    return -1;
                }
            }
        }
        close(client_fd);
    }
    return 0;
}



