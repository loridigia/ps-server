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
#include "commons.h"

#define CONFIG_PATH "../config.txt"
#define PORT_FLAG "--port="
#define TYPE_FLAG "--type="
#define HELP_FLAG "--help"

void read_arguments(configuration *config, int argc, char *argv[]) {
    char *input;
    for (int i = 1; i < argc; i++) {
        input = argv[i];
        char *endptr;
        if (strncmp(input, PORT_FLAG, strlen(PORT_FLAG)) == 0) {
            config->port = strtol(input, &endptr, 10);
            if (*endptr != '\0' || endptr == input) {
                perror("Controllare che la porta sia scritta correttamente. \n");
                exit(1);
            }
        } else if (strncmp(input, TYPE_FLAG, strlen(TYPE_FLAG)) == 0) {
            config->type = input + strlen(TYPE_FLAG);
            if (!(equals(config->type, "thread") || equals(config->type, "process"))) {
                perror("Seleziona una modalità corretta di avvio (thread o process)\n");
                exit(1);
            }
        } else if (strncmp(input, HELP_FLAG, strlen(HELP_FLAG)) == 0) {
            puts("Il server viene lanciato di default sulla porta 7070 in modalità multi-thread.\n\n"
                 "--port=<numero_porta> per specificare la porta su cui ascoltare all'avvio.\n"
                 "--type=<thread/process> per specificare la modalità di avvio del server.\n");
            exit(0);
        }
    }
}

void read_configuration(configuration *config) {
    int port_on = config->port > 0;
    int type_on = config->type != NULL;
    if (port_on && type_on) {
        return;
    }

    FILE *stream;
    char *line = NULL;
    stream = fopen(CONFIG_PATH, "r");
    if (stream == NULL) {
        perror("Impossibile aprire il file di configurazione.\n");
        exit(1);
    }
    char *port_param = get_parameter(line,stream);
    char *endptr;
    if (!port_on) {
        config->port = strtol(port_param, &endptr, 10);
        if (*endptr != '\0' || endptr == port_param) {
            perror("Controllare che la porta sia scritta correttamente. \n");
            exit(1);
        }
    }
    char *type_param = get_parameter(line,stream);
    if (!type_on) {
        config->type = type_param;
        if (!(equals(config->type, "thread") || equals(config->type, "process"))) {
            perror("Seleziona una modalità corretta di avvio (thread o process)\n");
            exit(1);
        }
    }
}

char *get_parameter(char *line, FILE *stream) {
    size_t len;
    char *ptr;
    if (getline(&line, &len, stream) != -1) {
        strtok(line, ":");
        ptr = strtok(NULL, "\n");
        return ptr;
    }
    return NULL;
}

char *get_ip() {
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
    return send(socket_fd, err, strlen(err), 0) == -1;
}


int is_file(char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

char *extract_route(char *buffer) {
    char *route = strdup(buffer);
    strtok(route, " ");
    return strtok(NULL, " ");
}

char *get_extension_code(const char *filename){
    const char *ext = strrchr(filename, '.');
    if (ext == NULL) return "1 ";
    else if (equals(ext, ".txt")) return "0 ";
    else if (equals(ext, ".gif")) return "g ";
    else if (equals(ext, ".jpeg") || equals(ext, ".jpg")) return "I ";
    else return "3";
}

int listen_on(int port, int *socket_fd, struct sockaddr_in *socket_addr) {
    bzero(socket_addr, sizeof *socket_addr);
    socket_addr->sin_family = AF_INET;
    socket_addr->sin_port = htons(port);
    socket_addr->sin_addr.s_addr = INADDR_ANY;

    if ((*socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Errore durante la creazione della socket.\n");
        return -1;
    }

    if (bind(*socket_fd, (struct sockaddr *)socket_addr, sizeof *socket_addr) == -1) {
        perror("Errore durante il binding tra socket_fd e socket_address\n");
        return -1;
    }

    if (listen(*socket_fd, BACKLOG) == -1) {
        perror("Errore nel provare ad ascoltare sulla porta data in input.\n");
        return -1;
    }

    return 0;
}