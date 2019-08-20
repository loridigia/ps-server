#include "unix.h"

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
    //daemon
    if (is_daemon()) {
        daemon_skeleton();
    }

    //pipe
    if (pipe(pipe_fd) == -1) {
        perror("Errore nel creare la pipe.\n");
        exit(1);
    }

    // configure
    config = mmap(NULL, sizeof config, PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    //tread
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

    //mutex, conditions
    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&mutex, &mutexAttr);

    pthread_condattr_t condAttr;
    pthread_condattr_setpshared(&condAttr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&condition, &condAttr);

    //listener
    pid_t pid_child = fork();

    if (pid_child < 0) {
        perror("Errore durante la fork.\n");
        exit(0);
    } else if (pid_child == 0) {
        log_routine(); //diventa routine
        exit(0);
    }

    //start
    start();
    signal(SIGHUP, restart);
    while(1) sleep(1);
}

int is_daemon(int argc, char *argv[]) {
    const char *flag = "--daemon";
    char *input;
    for (int i = 1; i < argc; i++) {
        input = argv[i];
        if (strncmp(input, flag, strlen(flag)) == 0) {
            return 1;
        }
    }
    return 0;
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
    char *new_str = malloc(strlen(file) + 2);
    strcpy(new_str, file);
    strcat(new_str, "\n");
    int res = 0;
    if (send(socket_fd, new_str, strlen(new_str), 0) == -1) {
        res = -1;
    }
    munmap(file, file_size);
    free(new_str);
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

char *get_client_buffer(int client_fd, int *err){
    int size = 10, chunk = 10;
    char *client_buffer = (char*)calloc(sizeof(char), size);
    int len = 0, n = 0;

    while ((n = recv(client_fd, client_buffer + len, chunk, MSG_DONTWAIT)) > 0 ) {
        len += n;
        if (len + chunk >= size) {
            size *= 2;
            client_buffer = (char*)realloc(client_buffer, size);
        }
    }

    if (n == -1 && errno != EAGAIN && errno != EWOULDBLOCK){
        *err = -1;
    }

    return client_buffer;
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