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

#define   LOCK_SH   1    /* shared lock */
#define   LOCK_EX   2    /* exclusive lock */
#define   LOCK_NB   4    /* don't block when locking */
#define   LOCK_UN   8    /* unlock */

#define BACKLOG 16
#define CONFIG_PATH "../config.txt"
#define PUBLIC_PATH "../public"
#define equals(str1, str2) strcmp(str1,str2) == 0

typedef struct pthread_arg_t {
    int new_socket_fd;
} pthread_arg_t;

typedef struct configuration {
    char * type;
    char * ip;
    unsigned int port;
} configuration;

void *pthread_routine(void *arg);

void process_routine (int socket_fd);

char * get_parameter(char * line, FILE * stream);

void read_config(int has_port, int has_type);

char * extract_route(char * buffer);

int is_file(char * path);

int send_error(int socket_fd, char * err);

char * extract_name_only(char * file);

char * get_ip();

const char * port_flag = "--port=";
const char * type_flag="--type=";
const char * help_flag="--help";

/*
 * Variabili che andranno splittate nel file dei thread.
 */
pthread_attr_t pthread_attr;
pthread_arg_t *pthread_arg;
pthread_t pthread;

configuration config;

int main(int argc, char *argv[]) {

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

    read_config(has_port, has_type);
    config.ip = get_ip();

    if (config.ip == NULL) {
        perror("Impossibile ottenere l'ip del server.");
        exit(1);
    }

    if (equals(config.type, "thread")) {
        if (pthread_attr_init(&pthread_attr) != 0) {
            perror("pthread_attr_init\n");
            exit(1);
        }

        if (pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED) != 0) {
            perror("pthread_attr_setdetachstate\n");
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
        perror("Errore durante la creazione della socket.\n");
        exit(1);
    }

    if (bind(socket_fd, (struct sockaddr *)&socket_addr, sizeof socket_addr) == -1) {
        perror("Errore durante il binding tra socket_fd e socket_address\n");
        exit(1);
    }

    if (listen(socket_fd, BACKLOG) == -1) {
        perror("Errore nel provare ad ascoltare sulla porta data in input.\n");
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
                        perror ("Errore durante l'accept del nuovo client.\n");
                        exit (1);
                    }
                    FD_SET (new, &read_fd_set);
                }
                else {
                    if (equals(config.type, "thread")) {
                        pthread_arg = (pthread_arg_t *)malloc(sizeof (pthread_arg_t));
                        if (!pthread_arg) {
                            perror("Impossibile allocare memoria per gli argomenti di pthread.\n");
                            //free(pthread_arg);
                            continue;
                        }
                        pthread_arg->new_socket_fd = fd;

                        if (pthread_create(&pthread, &pthread_attr, pthread_routine, (void *)pthread_arg) != 0) {
                            perror("Impossibile creare un nuovo thread.\n");
                            free(pthread_arg);
                            continue;
                        }

                        FD_CLR (fd, &read_fd_set);

                    } else if (equals(config.type, "process")) {
                        int pid = fork();

                        if (pid < 0) {
                            perror("Errore nel fare fork.\n");
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
                        perror ("Seleziona una modalità corretta di avvio (thread o process)\n");
                        exit (EXIT_FAILURE);
                    }
                }
            }
        }
    }
}


char *get_number_ext(const char *filename){
    const char *ext = strrchr(filename, '.');
    if (ext == NULL) return "1 ";
    else if (equals(ext, ".txt")) return "0 ";
    else if (equals(ext, ".gif")) return "g ";
    else if (equals(ext, ".jpeg") || equals(ext, ".jpg")) return "I ";
    else return "3";
}

char * get_dir_files(char * route, char * path, char * buffer) {
    DIR *d;
    struct dirent *dir;

    if ((d = opendir (path)) != NULL) {
        while ((dir = readdir (d)) != NULL) {
            if (equals(dir->d_name, ".") || equals(dir->d_name, "..")) {
                continue;
            }
            strcat(buffer, get_number_ext(dir->d_name));
            strcat(buffer, dir->d_name);
            strcat(buffer, "\t");
            strcat(buffer, route);
            if (path[strlen(path)-1] != '/') {
                strcat(buffer, "/");
            }
            strcat(buffer,"\t");
            strcat(buffer,config.ip);
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
        perror ("Impossibile aprire la directory");
        return NULL;
    }

    return buffer;
}

void *pthread_routine(void *arg) {
    pthread_arg_t *args = (pthread_arg_t *) arg;
    int socket_fd = args->new_socket_fd;

    free(arg);
    char client_buffer[512];
    bzero(client_buffer, sizeof client_buffer);

    if (recv(socket_fd, client_buffer, sizeof client_buffer, 0) == -1) {
        perror("Errore nel ricevere dati dalla socket. (ricezione richiesta)\n");
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
    char * route = extract_route(client_buffer);

    char path[128];
    strcpy(path, PUBLIC_PATH);
    strcat(path, route);

    if (is_file(path)) {
        /* read file */
        int fd = open(path, O_RDONLY);
        if(fd == -1)
        {
            perror("Errore nell'apertura del file");
            exit(1);
        }
        int lock = flock(fd, LOCK_SH); //LOCK
        struct stat v;
        stat(path,&v);
        char *p = malloc(v.st_size * sizeof(char));
        int res = read(fd,p,v.st_size);
        puts(p);
        int release = flock(fd, LOCK_UN); //UNLOCK

        /* map file in memory */

        /* create thread */

        /* send file using thread */
    } else {
        files = get_dir_files(route, path, listing_buffer);
        if (files == NULL) {
            if (send_error(socket_fd, "Directory non esistente.\n") == -1) {
                perror("Errore nel comunicare con la socket. (directory non esistente)\n");
            }
        }
        else {
            if (send(socket_fd, files, strlen(files), 0) == -1) {
                perror("Errore nel comunicare con la socket. (invio lista files)\n");
            }
        }
    }

    close(socket_fd);
    return NULL;

}

void read_config(int has_port, int has_type) {
    if (has_port && has_type) {
        return;
    }
    FILE *stream;
    char * line = NULL;
    stream = fopen(CONFIG_PATH, "r");
    if (stream == NULL) {
        perror("Impossibile aprire il file di configurazione.\n");
        exit(1);
    }
    char * port_param = get_parameter(line,stream);
    if (!has_port) {
        config.port = strtoul(port_param, NULL, 10);
        if (config.port == 0) {
            perror("Controllare che la porta sia scritta correttamente.\n");
            exit(1);
        }
    }
    char * type_param = get_parameter(line,stream);
    if (!has_type) {
        config.type = type_param;
        if (config.type == NULL) {
            perror("Seleziona una modalità corretta di avvio (thread o process)\n");
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

char * extract_route(char * buffer) {
    char * route = strdup(buffer);
    strtok(route, " ");
    return strtok(NULL, " ");
}

/*
char * extract_name_only(char * file) {
    if (strchr(file, '.') == NULL) {
        return file;
    }

    int size = strlen(file);
    char *end = file + size;
    while (*end != '.') {
        end--;
        size--;
    }

    *end = '\0';
    return end-size;
}
*/

int is_file(char * path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

int send_error(int socket_fd, char * err) {
    return send(socket_fd, err, strlen(err), 0) == -1;
}

char * get_ip() {
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