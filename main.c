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

#define BACKLOG 16
#define CONFIG_PATH "../config.txt"
#define equals(str1, str2) strcmp(str1,str2) == 0

typedef struct pthread_arg_t {
    int new_socket_fd;
} pthread_arg_t;

typedef struct configuration {
    unsigned int port;
    char * type;
} configuration;

void *pthread_routine(void *arg);

void process_routine (int socket_fd);

char * get_parameter(char * line, FILE * stream);

void read_config(int has_port, int has_type, configuration * config);

const char * port_flag = "--port=";
const char * type_flag="--type=";
const char * help_flag="--help";

/*
 * Variabili che andranno splittate nel file dei thread.
 */
pthread_attr_t pthread_attr;
pthread_arg_t *pthread_arg;
pthread_t pthread;

int main(int argc, char *argv[]) {
    configuration config;

    int has_port = 0, has_type = 0;
    char * input;
    for (int fd = 1; fd < argc; fd++) {
        input = argv[fd];
        if (strncmp(input, port_flag, strlen(port_flag)) == 0) {
            has_port = 1;
            config.port = strtoul(input + strlen(port_flag), NULL, 10);
            if (config.port == 0) {
                perror("Porta errata");
                exit(1);
            }
        } else if (strncmp(input, type_flag, strlen(type_flag)) == 0) {
            has_type = 1;
            config.type = input + strlen(type_flag);
            if (config.type == NULL) {
                perror("Errore scelta thread/process");
                exit(1);
            }
        } else if (strncmp(input, help_flag, strlen(help_flag)) == 0) {
            puts("Il server viene lanciato di default sulla porta 7070 in modalità multi-thread.\n\n"
                 "--port=<numero_porta> per specificare la porta su cui ascoltare all'avvio.\n"
                 "--type=<thread/process> per specificare la modalità di avvio del server.\n");
        }
    }

    read_config(has_port, has_type, &config);

    if (equals(config.type, "thread")) {
        if (pthread_attr_init(&pthread_attr) != 0) {
            perror("pthread_attr_init");
            exit(1);
        }

        if (pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED) != 0) {
            perror("pthread_attr_setdetachstate");
            exit(1);
        }
    }

    int socket_fd;

    struct sockaddr_in socket_addr;
    bzero(&socket_addr, sizeof socket_addr);
    socket_addr.sin_family = AF_INET;
    socket_addr.sin_port = htons(config.port);
    socket_addr.sin_addr.s_addr = INADDR_ANY;

    socklen_t socket_size;

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Errore durante la creazione della socket.");
        exit(1);
    }

    if (bind(socket_fd, (struct sockaddr *)&socket_addr, sizeof socket_addr) == -1) {
        perror("Errore durante il binding tra socket_fd e socket_address");
        exit(1);
    }

    if (listen(socket_fd, BACKLOG) == -1) {
        perror("Errore nel provare ad ascoltare sulla porta data in input.");
        exit(1);
    }

    fd_set read_fd_set;
    FD_ZERO (&read_fd_set);
    FD_SET (socket_fd, &read_fd_set);

    while (1)
    {
        if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
        {
            perror ("Errore durante la select.");
            exit (1);
        }

        for (int fd = 0; fd < FD_SETSIZE; fd++) {
            if (FD_ISSET (fd, &read_fd_set)) {
                if (fd == socket_fd) {
                    /* Nuova connessione */
                    socket_size = sizeof (socket_addr);
                    int new = accept (socket_fd, (struct sockaddr *) &socket_addr, &socket_size);
                    if (new < 0) {
                        perror ("Errore durante l'accept del nuovo client.");
                        exit (1);
                    }
                    FD_SET (new, &read_fd_set);
                }
                else {
                    if (equals(config.type, "thread")) {
                        pthread_arg = (pthread_arg_t *)malloc(sizeof (pthread_arg_t));
                        if (!pthread_arg) {
                            perror("Impossibile allocare memoria per gli argomenti di pthread.");
                            //free(pthread_arg);
                            continue;
                        }
                        pthread_arg->new_socket_fd = fd;

                        if (pthread_create(&pthread, &pthread_attr, pthread_routine, (void *)pthread_arg) != 0) {
                            perror("Impossibile creare un nuovo thread.");
                            free(pthread_arg);
                            continue;
                        }

                        FD_CLR (fd, &read_fd_set);

                    } else if (equals(config.type, "process")) {
                        int pid = fork();

                        if (pid < 0) {
                            perror("Errore nel fare fork.");
                            exit(1);
                        }

                        if (pid == 0) {
                            process_routine(fd);
                            exit(0);
                        }
                        else {
                            close(fd);
                            FD_CLR (fd, &read_fd_set);
                        }
                    } else {
                        perror ("Seleziona una modalità corretta di avvio (thread o process)");
                        exit (EXIT_FAILURE);
                    }
                }
            }
        }
    }
}

char * get_dir_files(char * path){
    int n=0, i=0;
    struct dirent *dir;
    DIR *d;
    d = opendir(path);
    
    while((readdir(d)) != NULL) {
        n++;
    }
    rewinddir(d);
    char *filesList[n];
    while((dir = readdir(d)) != NULL) {
        filesList[i] = (char*) malloc(strlen(dir->d_name)+1);
        strncpy(filesList[i],dir->d_name, strlen(dir->d_name));
        puts(dir->d_name);
        i++;
    }
    rewinddir(d);
    return *filesList;
}

void *pthread_routine(void *arg) {
    pthread_arg_t *args = (pthread_arg_t *) arg;
    int socket_fd = args->new_socket_fd;

    free(arg);
    char buffer[256];
    int res = recv(socket_fd, buffer, sizeof buffer, 0);
    if (res == 2) {
        memset(buffer, 0, sizeof buffer);
        char *files = get_dir_files("../files");
        send(socket_fd, files, strlen(files), 0);
        close(socket_fd);
    }
}

void read_config(int has_port, int has_type, configuration * config) {
    if (has_port && has_type) {
        return;
    }
    FILE *stream;
    char * line = NULL;
    stream = fopen(CONFIG_PATH, "r");
    if (stream == NULL) {
        perror("Impossibile aprire il file di configurazione.");
        exit(1);
    }
    char * port_param = get_parameter(line,stream);
    if (!has_port) {
        config->port = strtoul(port_param, NULL, 10);
        if (config->port == 0) {
            perror("Controllare che la porta sia scritta correttamente.");
            exit(1);
        }
    }
    char * type_param = get_parameter(line,stream);
    if (!has_type) {
        config->type = type_param;
        if (config->type == NULL) {
            perror("Seleziona una modalità corretta di avvio (thread o process)");
            exit(1);
        }
    }
}

char * get_parameter(char * line, FILE * stream) {
    size_t len;
    char * ptr;
    if (getline(&line, &len, stream) != -1) {
        strtok(line, ":");
        ptr = strtok(NULL, "\n");
        return ptr;
    }
    return NULL;
}

void process_routine(int socket_fd) {
    char buffer[256];
    int n = recv(socket_fd, buffer, sizeof buffer, 0);
    if (n < 0) {
        perror("ERROR reading from socket");
        exit(1);
    }
    puts(buffer);
    send(socket_fd, buffer, strlen(buffer), 0);
    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }
}