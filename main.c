#if defined(__linux__) || defined(__APPLE__)
    #include "unix/unix.h"
#elif defined(_WIN32)
    #include "win/win.h"
#endif

int serve_client(int client_fd, char *client_ip, int port);
int write_on_pipe(int size, char *name, int port, char *ip);
int work_with_threads(int fd, char *client_ip, int port);
int work_with_processes(int fd, char *client_ip, int port);
void *pthread_receiver_routine(void *arg);
void *pthread_listener_routine(void *arg);
void handle_requests(int port, int (*handle)(int fd, char* client_ip, int port));

int main(int argc, char *argv[]) {
    init(argc, argv);
}

void start() {
    if (equals(config->type, "thread")) {
        pthread_arg_listener *args =
                (pthread_arg_listener *)malloc(sizeof (pthread_arg_listener));
        if (args < 0) {
            perror("Impossibile allocare memoria per gli argomenti del thread di tipo 'listener'.\n");
            free(args);
            exit(1);
        }
        args->port = config->port;
        if (pthread_create(
                &pthread,
                &pthread_attr,
                pthread_listener_routine,
                (void *)args) != 0) {
            perror("Impossibile creare un nuovo thread.\n");
            free(args);
            exit(1);
        }
    } else {
        pid_t pid_child = fork();
        if (pid_child < 0) {
            perror("Errore durante la fork.\n");
            exit(1);
        } else if (pid_child == 0) {
            handle_requests(config->port, work_with_processes);
            exit(0);
        }
    }
}

void *pthread_listener_routine(void *arg) {
    pthread_arg_listener *args = (pthread_arg_listener *) arg;
    int port = args->port;
    free(args);
    handle_requests(port, work_with_threads);
    return NULL;
}

void handle_requests(int port, int (*handle)(int, char*, int)){
    int socket_fd;
    struct sockaddr_in socket_addr;
    if (listen_on(port, &socket_fd, &socket_addr) != 0) {
        return;
    }

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    fd_set read_fd_set;
    socklen_t socket_size;
    while (1) {
        FD_ZERO (&read_fd_set);
        FD_SET (socket_fd, &read_fd_set);

        if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, &timeout) < 0) {
            perror("Errore durante l'operazione di select.\n");
            continue;
        }

        if(config->port != port) {
            printf("Chiusura socket su porta %d. \n", port);
            break;
        }

        for (int fd = 0; fd < FD_SETSIZE; fd++) {
            if (FD_ISSET (fd, &read_fd_set)) {
                if (fd == socket_fd) {
                    socket_size = sizeof(socket_addr);
                    int new = accept(socket_fd, (struct sockaddr *) &socket_addr, &socket_size);
                    if (new < 0) {
                        perror("Errore nell'accettare la richiesta del client\n");
                        continue;
                    }
                    FD_SET (new, &read_fd_set);
                } else {
                    char *client_ip = get_client_ip(&socket_addr);
                    if (handle(fd, client_ip, port) == -1) {
                        FD_CLR (fd, &read_fd_set);
                        fprintf(stderr,"Errore nel comunicare con la socket %d. Client-ip: %s", fd, client_ip);
                        continue;
                    }
                }
            }
        }
    }
    close(socket_fd);
}


int work_with_threads(int fd, char *client_ip, int port) {
    pthread_arg_receiver *args =
            (pthread_arg_receiver *)malloc(sizeof (pthread_arg_receiver));
    if (args < 0) {
        perror("Impossibile allocare memoria per gli argomenti del thread di tipo 'receiver'.\n");
        free(args);
        return -1;
    }
    args->new_socket_fd = fd;
    args->port = port;
    args->client_ip = client_ip;

    if (pthread_create(&pthread, &pthread_attr, pthread_receiver_routine, (void *) args) != 0) {
        perror("Impossibile creare un nuovo thread di tipo 'receiver'.\n");
        free(args);
        return -1;
    }
    return 0;
}

int work_with_processes(int fd, char *client_ip, int port) {
    pid_t pid_child = fork();
    if (pid_child < 0) {
        perror("fork");
        exit(0);
    } else if (pid_child == 0) {
        serve_client(fd, client_ip, port);
    } else {
        close(fd);
    }
    return 0;
}

void *pthread_receiver_routine(void *arg) {
    pthread_arg_receiver *args = (pthread_arg_receiver *) arg;
    serve_client(args->new_socket_fd, args->client_ip, args->port);
    free(arg);
    return NULL;
}

int write_on_pipe(int size, char* name, int port, char *client_ip){
    char *buffer = malloc(strlen(name) + sizeof(size) + strlen(client_ip) + sizeof(port) + LOG_MIN_SIZE);
    sprintf(buffer, "name: %s | size: %d | ip: %s | port: %d\n", name, size, client_ip, port);
    pthread_mutex_lock(&mutex);
    if (write(pipe_fd[1], buffer, strlen(buffer)) < 0) {
        pthread_mutex_unlock(&mutex);
        free(buffer);
        return -1;
    }
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&condition);
    free(buffer);
    return 0;
}

void *send_routine(void *arg) {
    pthread_arg_sender *args = (pthread_arg_sender *) arg;
    int send = send_file(args->client_fd, args->file_in_memory, args->size);
    if (send == -1){
        fprintf(stderr, "Errore nel comunicare con la socket. ('sender')\n");
    } else {
        if (write_on_pipe(args->size, args->route, args->port, args->client_ip) < 0) {
            fprintf(stderr, "Errore nello scrivere sulla pipe LOG.\n");
            return NULL;
        }
    } if (equals(config->type,"process")) {
        exit(0);
    }
    return NULL;
}

int serve_client(int client_fd, char *client_ip, int port) {
    int res = 0;
    char *err;
    char *client_buffer = get_client_buffer(client_fd, &res);

    if (res == -1) {
        err = "Errore nel ricevere i dati.\n";
        fprintf(stderr,"%s",err);
        send_error(client_fd, err);
        close(client_fd);
        return -1;
    }

    int end = index_of(client_buffer, '\r');
    if (end == -1) {
        end = strlen(client_buffer);
    }

    char route[end + 1];
    strcpy(route, client_buffer);
    route[sizeof route - 1] = '\0';

    char path[sizeof(PUBLIC_PATH) + strlen(route)];
    strcpy(path, PUBLIC_PATH);
    strcat(path, route);

    if (is_file(path)) {
        int file_fd = open(path, O_RDONLY);
        if (file_fd == -1) {
            err = "Errore nell'apertura del file. \n";
            fprintf(stderr,"%s%s",err,path);
            send_error(client_fd, err);
            close(client_fd);
            return -1;
        }

        struct stat v;
        if (stat(path,&v) == -1) {
            perror("Errore nel prendere la grandezza del file.\n");
            close(client_fd);
            return -1;
        }

        int size = v.st_size;
        if (size == 0) truncate(path, 4);

        flock(file_fd, LOCK_EX);
        char *file_in_memory = mmap(NULL, size, PROT_READ, MAP_PRIVATE, file_fd, 0);
        flock(file_fd, LOCK_UN);
        if (file_in_memory == MAP_FAILED) {
            perror("Errore nell'operazione di mapping del file.\n");
            close(client_fd);
            return -1;
        }

        pthread_arg_sender *args =
                (pthread_arg_sender *)malloc(sizeof (pthread_arg_sender));

        if (args < 0) {
            perror("Impossibile allocare memoria per gli argomenti del thread di tipo 'sender'.\n");
            free(args);
            return -1;
        }

        args->client_ip = client_ip;
        args->client_fd = client_fd;
        args->port = port;
        args->file_in_memory = file_in_memory;
        args->route = route;
        args->size = size;

        if (equals(config->type, "thread")) {
            send_routine(args);
        } else if (equals(config->type, "process")) {
            if (pthread_create(&pthread, &pthread_attr, send_routine, (void *) args) != 0) {
                perror("Impossibile creare un nuovo thread di tipo 'sender'.\n");
                free(args);
                return -1;
            }
        }

    } else {
        char *listing_buffer;
        int size;
        if ((listing_buffer = get_file_listing(route, path, &size)) == NULL) {
            err = "File o directory non esistente.\n";
            fprintf(stderr,"%s",err);
            send_error(client_fd, err);
            close(client_fd);
            return -1;
        }
        else {
            if (send(client_fd, listing_buffer, size, 0) != 0) {
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