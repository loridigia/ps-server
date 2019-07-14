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

void init_server(configuration *config, int argc, char *argv[]) {
    read_arguments(config, argc, argv);
    read_configuration(config);
    config->ip_address = get_ip();
    if (config->ip_address == NULL) {
        /* qualche errore */
    }
}

void read_arguments(configuration *config, int argc, char *argv[]) {
    char *input;
    for (int i = 1; i < argc; i++) {
        input = argv[i];
        if (strncmp(input, PORT_FLAG, strlen(PORT_FLAG)) == 0) {
            config->port = strtoul(input + strlen(PORT_FLAG), NULL, 10);
            if (config->port == 0) {
                perror("Porta errata");
                exit(1);
            }
        } else if (strncmp(input, TYPE_FLAG, strlen(TYPE_FLAG)) == 0) {
            config->type = input + strlen(TYPE_FLAG);
            if (config->type == NULL) {
                perror("Errore scelta thread/process");
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
    if (port_on && type_on) { return; }

    FILE *stream;
    char *line = NULL;
    stream = fopen(CONFIG_PATH, "r");
    if (stream == NULL) {
        perror("Impossibile aprire il file di configurazione.\n");
        exit(1);
    }
    char *port_param = get_parameter(line,stream);
    if (!port_on) {
        config->port = strtoul(port_param, NULL, 10);
        if (config->port == 0) {
            perror("Controllare che la porta sia scritta correttamente.\n");
            exit(1);
        }
    }
    char *type_param = get_parameter(line,stream);
    if (!type_on) {
        config->type = type_param;
        if (config->type == NULL) {
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