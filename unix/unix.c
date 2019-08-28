#include "unix.h"

extern configuration *config;

void daemon_skeleton() {
    pid_t pid;
    pid = fork();
    if (pid < 0) {
        exit(1);
    } else if (pid > 0) {
        exit(0);
    }
    umask(0);
    if (setsid() < 0) {
        exit(1);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

void init(int argc, char *argv[]) {
    if (is_daemon(argc, argv)) {
        daemon_skeleton();
    }

    if (pipe(pipe_fd) == -1) {
        perror("Errore nel creare la pipe.\n");
        exit(1);
    }

    config = mmap(NULL, sizeof config, PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (pthread_attr_init(&pthread_attr) != 0) {
        perror("Errore nell'inizializzazione degli attributi del thread.\n");
        exit(1);
    }

    if (pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED) != 0) {
        perror("Errore nel settare DETACHED_STATE al thread.\n");
        exit(1);
    }

    if (load_configuration(COMPLETE) == -1 || load_arguments(argc,argv) == -1) {
        exit(1);
    }

    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&mutex, &mutexAttr);

    pthread_condattr_t condAttr;
    pthread_condattr_setpshared(&condAttr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&condition, &condAttr);

    pid_t pid_child = fork();

    if (pid_child < 0) {
        perror("Errore durante la fork.\n");
        exit(0);
    } else if (pid_child == 0) {
        log_routine();
        exit(0);
    }

    start();
    signal(SIGHUP, restart);
    while(1) sleep(1);
}

void log_routine() {
    char buffer[8192];
    while(1) {
        bzero(buffer, sizeof buffer);
        pthread_cond_wait(&condition, &mutex);
        pthread_mutex_lock(&mutex);
        read(pipe_fd[0], buffer, sizeof buffer);
        pthread_mutex_unlock(&mutex);
        _log(buffer);
    }
}

char *get_client_ip(struct sockaddr_in *socket_addr){
    struct in_addr ipAddr = socket_addr->sin_addr;
    char *str = malloc(INET_ADDRSTRLEN);
    inet_ntop( AF_INET, &ipAddr, str, INET_ADDRSTRLEN );
    return str;
}

int send_file(int socket_fd, char *file, size_t file_size){
    char new[strlen(file)+2];
    sprintf(new, "%s\n", file);
    int res = 0;
    if (send(socket_fd, new, strlen(new), 0) == -1) {
        res = -1;
    }
    munmap(file, file_size);
    close(socket_fd);
    return res;
}

int listen_on(int port, int *socket_fd, struct sockaddr_in *socket_addr) {
    bzero(socket_addr, sizeof *socket_addr);
    socket_addr->sin_family = AF_INET;
    socket_addr->sin_port = htons(port);
    socket_addr->sin_addr.s_addr = INADDR_ANY;

    if ((*socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Errore durante la creazione della socket.\n");
        return -1;
    } if (bind(*socket_fd, (struct sockaddr *)socket_addr, sizeof *socket_addr) == -1) {
        perror("Errore durante il binding tra socket_fd e socket_address\n");
        return -1;
    } if (listen(*socket_fd, BACKLOG) == -1) {
        perror("Errore nel provare ad ascoltare sulla porta data in input.\n");
        return -1;
    }
    return 0;
}

char *get_server_ip() {
    struct ifaddrs *addrs;
    getifaddrs(&addrs);
    struct ifaddrs *tmp = addrs;

    while (tmp) {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
            return inet_ntoa(pAddr->sin_addr);
        } tmp = tmp->ifa_next;
    }
    //freeifaddrs(addrs);
    return NULL;
}

int send_error(int socket_fd, char *err) {
    return send(socket_fd, err, strlen(err), 0);
}

void start() {
    if (equals(config->server_type, "thread")) {
        if (pthread_create(
                &pthread,
                &pthread_attr,
                listener_routine,
                (void *) &config->server_port) != 0) {
            perror("Impossibile creare un nuovo thread.\n");
            exit(1);
        }
    } else {
        pid_t pid_child = fork();
        if (pid_child < 0) {
            perror("Errore durante la fork.\n");
            exit(1);
        } else if (pid_child == 0) {
            handle_requests(config->server_port, work_with_processes);
            exit(0);
        }
    }
}


void *listener_routine(void *arg) {
    int port = *(int*)arg;
    handle_requests(port, work_with_threads);
    return NULL;
}

void handle_requests(int port, int (*handle)(int, char*, int)){
    int server_socket;
    fd_set active_fd_set, read_fd_set;
    struct sockaddr_in server_addr, client_addr;
    socklen_t size;

    if (listen_on(port, &server_socket, &server_addr) != 0) {
        fprintf(stderr, "Impossibile creare la socket su porta: %d", port);
        return;
    }

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    FD_ZERO (&active_fd_set);
    FD_SET (server_socket, &active_fd_set);

    while (1) {
        read_fd_set = active_fd_set;

        int selected;
        if ((selected = select (FD_SETSIZE, &read_fd_set, NULL, NULL, &timeout)) < 0) {
            perror("Errore durante l'operazione di select.\n");
            break;
        }

        if(config->server_port != port) {
            printf("Chiusura socket su porta %d. \n", port);
            close(server_socket);
            return;
        }

        if (selected == 0) continue;

        for (int fd = 0; fd < FD_SETSIZE; ++fd) {
            if (FD_ISSET (fd, &read_fd_set)) {
                if (fd == server_socket) {
                    size = sizeof(client_addr);
                    int new = accept(server_socket, (struct sockaddr *) &client_addr, &size);
                    if (new < 0) {
                        perror("Errore nell'accettare la richiesta del client\n");
                        continue;
                    }
                    FD_SET (new, &active_fd_set);
                } else {
                    char *client_ip = get_client_ip(&client_addr);
                    if (handle(fd, client_ip, port) == -1) {
                        FD_CLR (fd, &read_fd_set);
                        close(fd);
                        fprintf(stderr, "Errore nel comunicare con la socket %d. Client-ip: %s", fd, client_ip);
                        continue;
                    }
                    FD_CLR (fd, &active_fd_set);
                }
            }
        }
    }
}


int work_with_threads(int fd, char *client_ip, int port) {
    thread_arg_receiver *args =
            (thread_arg_receiver *)malloc(sizeof (thread_arg_receiver));
    if (args < 0) {
        perror("Impossibile allocare memoria per gli argomenti del thread di tipo 'receiver'.\n");
        free(args);
        return -1;
    }
    args->client_socket = fd;
    args->port = port;
    args->client_ip = client_ip;

    if (pthread_create(&pthread, &pthread_attr, receiver_routine, (void *) args) != 0) {
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

void *receiver_routine(void *arg) {
    thread_arg_receiver *args = (thread_arg_receiver *) arg;
    serve_client(args->client_socket, args->client_ip, args->port);
    free(arg);
    return NULL;
}

int write_on_pipe(int size, char* name, int port, char *client_ip){
    char *buffer = malloc(strlen(name) + sizeof(size) + strlen(client_ip) + sizeof(port) + LOG_MIN_SIZE);
    sprintf(buffer, "name: %s | size: %d | ip: %s | server_port: %d\n", name, size, client_ip, port);
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
    thread_arg_sender *args = (thread_arg_sender *) arg;
    int send = send_file(args->client_socket, args->file_in_memory, args->size);
    if (send == -1){
        fprintf(stderr, "Errore nel comunicare con la socket. ('sender')\n");
    } else {
        if (write_on_pipe(args->size, args->route, args->port, args->client_ip) < 0) {
            fprintf(stderr, "Errore nello scrivere sulla pipe LOG.\n");
            return NULL;
        }
    } if (equals(config->server_type,"process")) {
        exit(0);
    }
    return NULL;
}

int serve_client(int client_fd, char *client_ip, int port) {
    char *err;
    int n;

    char *client_buffer = get_client_buffer(client_fd, &n, MSG_DONTWAIT);

    if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        err = "Errore nel ricevere i dati.\n";
        fprintf(stderr,"%s",err);
        send_error(client_fd, err);
        close(client_fd);
        return -1;
    }

    unsigned int end = index_of(client_buffer, '\r');
    if (end == -1) {
        end = strlen(client_buffer);
    }
    client_buffer[end] = '\0';

    char path[strlen(PUBLIC_PATH) + strlen(client_buffer)];
    sprintf(path,"%s%s", PUBLIC_PATH, client_buffer);

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
        if (size == 0) truncate(path, 0);

        if (flock(file_fd, LOCK_EX) < 0) {
            perror("Impossibile lockare il file.\n");
            close(client_fd);
            return -1;
        }
        char *file_in_memory = mmap(NULL, size, PROT_READ, MAP_PRIVATE, file_fd, 0);
        if (flock(file_fd, LOCK_UN) < 0) {
            perror("Impossibile unlockare il file.\n");
            close(client_fd);
            return -1;
        }

        if (file_in_memory == MAP_FAILED) {
            perror("Errore nell'operazione di mapping del file.\n");
            close(client_fd);
            return -1;
        }

        thread_arg_sender *args =
                (thread_arg_sender *)malloc(sizeof (thread_arg_sender));

        if (args < 0) {
            perror("Impossibile allocare memoria per gli argomenti del thread di tipo 'sender'.\n");
            free(args);
            return -1;
        }

        args->client_ip = client_ip;
        args->client_socket = client_fd;
        args->port = port;
        args->file_in_memory = file_in_memory;
        args->route = client_buffer;
        args->size = size;

        if (equals(config->server_type, "thread")) {
            send_routine(args);
        } else if (equals(config->server_type, "process")) {
            if (pthread_create(&pthread, &pthread_attr, send_routine, (void *) args) != 0) {
                perror("Impossibile creare un nuovo thread di tipo 'sender'.\n");
                free(args);
                return -1;
            }
        }

    } else {
        char *listing_buffer;
        if ((listing_buffer = get_file_listing(client_buffer, path)) == NULL) {
            err = "File o directory non esistente.\n";
            fprintf(stderr,"%s",err);
            send_error(client_fd, err);
            close(client_fd);
            return -1;
        }
        else {
            if (send(client_fd, listing_buffer, strlen(listing_buffer), 0) < 0) {
                perror("Errore nel comunicare con la socket.\n");
                close(client_fd);
                return -1;
            }
        }
        close(client_fd);
    }
    return 0;
}

void _log(char *buffer) {
    FILE *file = fopen(LOG_PATH, "a");
    if (file == NULL) {
        perror("Errore nell'operazione di scrittura sul log.\n");
        fclose(file);
        return;
    }
    fprintf(file, "%s", buffer);
    fclose(file);
}

size_t _getline(char **lineptr, size_t *n, FILE *stream) {
    return getline(lineptr, n, stream);
}

int _recv(int s,char *buf,int len,int flags) {
    return recv(s,buf,len,flags);
}