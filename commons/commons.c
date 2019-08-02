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
#define MIN_PORT 1024
#define MAX_PORT 65535

void load_arguments(int argc, char *argv[]) {
    char *input;
    for (int i = 1; i < argc; i++) {
        input = argv[i];
        char *endptr;
        if (strncmp(input, PORT_FLAG, strlen(PORT_FLAG)) == 0) {
            config->port = strtol(input + strlen(PORT_FLAG), &endptr, 10);
            if (config->port < MIN_PORT || config->port > MAX_PORT || *endptr != '\0' || endptr == (input + strlen(PORT_FLAG))) {
                fprintf(stderr,"Controllare che la porta sia scritta correttamente o che non sia well-known (< 1024). \n");
                exit(1);
            }
        } else if (strncmp(input, TYPE_FLAG, strlen(TYPE_FLAG)) == 0) {
            config->type = input + strlen(TYPE_FLAG);
            if (!(equals(config->type, "thread") || equals(config->type, "process"))) {
                fprintf(stderr,"Seleziona una modalità corretta di avvio (thread o process)\n");
                exit(1);
            }
        } else if (strncmp(input, HELP_FLAG, strlen(HELP_FLAG)) == 0) {
            puts("Il server viene lanciato di default sulla porta 7070 in modalità multi-thread.\n"
                 "--port=<numero_porta> per specificare la porta su cui ascoltare all'avvio.\n"
                 "--type=<thread/process> per specificare la modalità di avvio del server.\n");
            exit(0);
        } else {
            fprintf(stderr,"Parametro %s sconosciuto. --help per conocescere i parametri permessi", input);
            exit(1);
        }
    }
}

int load_configuration(int mode) {
    FILE *stream;
    char *line = NULL;
    stream = fopen(CONFIG_PATH, "r");
    if (stream == NULL) {
        fprintf(stderr,"Impossibile aprire il file di configurazione.\n");
        fclose(stream);
        return -1;
    }

    char *port_param = get_parameter(line,stream);
    char *endptr;
    config->port = strtol(port_param, &endptr, 10);
    if (config->port < 1024 || config->port > MAX_PORT || *endptr != '\0' || endptr == port_param) {
        fprintf(stderr,"Controllare che la porta sia scritta correttamente o che non sia well-known. \n");
        fclose(stream);
        return -1;
    }
    if (mode == PORT_ONLY) {
        fclose(stream);
        return 0;
    }
    char *type_param = get_parameter(line,stream);
    config->type = type_param;
    if (!(equals(config->type, "thread") || equals(config->type, "process"))) {
        fprintf(stderr,"Seleziona una modalità corretta di avvio (thread o process).\n");
        fclose(stream);
        return -1;
    }
    config->ip_address = get_ip();
    if (config->ip_address == NULL) {
        fprintf(stderr,"Impossibile ottenere l'ip del server.\n");
        fclose(stream);
        return -1;
    }
    fclose(stream);
    return 0;
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

char *get_client_buffer(int client_fd, int *err){
    char *client_buffer = (char*)calloc(sizeof(char),10);
    char *receiver_buffer = (char*)calloc(sizeof(char),10);
    int n_data,pointer,i=0;

    while ((n_data = recv(client_fd, receiver_buffer, 10, MSG_DONTWAIT)) > 0 ){
        if (i > 0){
            pointer = strlen(client_buffer);
            client_buffer = (char *)realloc(client_buffer,((10 * i) + n_data));
            memcpy(&client_buffer[pointer], receiver_buffer, n_data);
        } else {
            memcpy(client_buffer, receiver_buffer, n_data);
        }
        i++;
        memset(receiver_buffer, 0, strlen(receiver_buffer));
    }

    if (n_data == -1 && (errno != EAGAIN || errno != EWOULDBLOCK)){
        *err = -1;
    }
    return client_buffer;
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
    return send(socket_fd, err, strlen(err), 0);
}

int is_file(char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
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
            strcat(buffer,config->ip_address);
            strcat(buffer,"\t");
            char port[6];
            sprintf(port,"%d",config->port);
            strcat(buffer,port);
            strcat(buffer, "\n");
        }
        strcat(buffer, ".\n");
        rewinddir(d);
        closedir (d);
    }
    else {
        return NULL;
    }
    return buffer;
}

int index_of(char *values, char find) {
    const char *ptr = strchr(values, find);
    return ptr ? ptr-values : -1;
}

int send_file(int socket_fd, char *file, size_t file_size){
    char *new_str = malloc(strlen(file) + 2);
    strcpy(new_str, file);
    strcat(new_str, "\n");

    if (send(socket_fd, new_str, strlen(new_str), 0) == -1) {
        perror("Errore nel comunicare con la socket.\n");
        return 1;
    }
    munmap(file, file_size);
    free(new_str);
    close(socket_fd);
    return 0;
}

char *get_client_ip(struct sockaddr_in *socket_addr){
    struct in_addr ipAddr = socket_addr->sin_addr;
    char *str = malloc(INET_ADDRSTRLEN);
    inet_ntop( AF_INET, &ipAddr, str, INET_ADDRSTRLEN );
    return str;
}