#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "shared.h"

configuration *config = &conf;

int load_arguments(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        char *endptr, *input = argv[i];
        if (strncmp(input, PORT_FLAG, strlen(PORT_FLAG)) == 0) {
            config->server_port = strtol(input + strlen(PORT_FLAG), &endptr, 10);
            if (config->server_port < MIN_PORT || config->server_port > MAX_PORT || *endptr != '\0' || endptr == (input + strlen(PORT_FLAG))) {
                fprintf(stderr,"Controllare che la porta sia scritta correttamente o che non sia well-known (< 1024). \n");
                return -1;
            }
        } else if (strncmp(input, TYPE_FLAG, strlen(TYPE_FLAG)) == 0) {
            strcpy(config->server_type, input + strlen(TYPE_FLAG));
            if (!(equals(config->server_type, "thread") || equals(config->server_type, "process"))) {
                fprintf(stderr,"Seleziona una modalità corretta di avvio (thread o process)\n");
                return -1;
            }
        } else if (strncmp(input, HELP_FLAG, strlen(HELP_FLAG)) == 0) {
            puts("Il server viene lanciato di default sulla porta 7070 in modalità multi-thread.\n"
                 "--server_port=<numero_porta> per specificare la porta su cui ascoltare all'avvio.\n"
                 "--server_type=<thread/process> per specificare la modalità di avvio del server.\n"
                 "--daemon per lanciare il server in modalità daemon. (solo unix)\n");
            exit(0);
        }
    }
    return 0;
}

int load_configuration(int mode) {
    FILE *stream;
    char *line = NULL;
    stream = fopen(CONFIG_PATH, "r");
    if (stream == NULL) {
        perror("Impossibile aprire il file di configurazione.\n");
        fclose(stream);
        return -1;
    }

    char *endptr, *port_param = get_parameter(line,stream);
    config->server_port = strtol(port_param, &endptr, 10);
    if (config->server_port < MIN_PORT || config->server_port > MAX_PORT || *endptr != '\0' || endptr == port_param) {
        fprintf(stderr,"Controllare che la porta sia scritta correttamente o che non sia well-known. \n");
        fclose(stream);
        return -1;
    }
    if (mode == PORT_ONLY) {
        free(port_param);
        fclose(stream);
        return 0;
    }

    char *type_param = get_parameter(line,stream);
    strcpy(config->server_type, type_param);
    if (!(equals(config->server_type, "thread") || equals(config->server_type, "process"))) {
        fprintf(stderr,"Seleziona una modalità corretta di avvio (thread o process).\n");
        fclose(stream);
        return -1;
    }
    char *ip = get_server_ip();
    strcpy(config->server_ip, ip);
    if (ip == NULL) {
        fprintf(stderr,"Impossibile ottenere l'ip del server.\n");
        fclose(stream);
        return -1;
    }

    free(ip);
    free(port_param);
    free(type_param);
    fclose(stream);
    return 0;
}

char *get_extension_code(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (ext == NULL) {
        return "1 ";
    } else if (equals(ext, ".txt")) {
        return "0 ";
    } else if (equals(ext, ".gif")) {
        return "g ";
    } else if (equals(ext, ".jpeg") || equals(ext, ".jpg")) {
        return "I ";
    } return "3 ";
}

int index_of(char *values, char find) {
    if (values == NULL) return -1;
    const char *ptr = strchr(values, find);
    return ptr ? ptr-values : -1;
}

char *get_parameter(char *line, FILE *stream) {
    size_t len;
    if (_getline(&line, &len, stream) != -1) {
        char *ptr = (char*)malloc(len+1);
        if (ptr == NULL) {
            perror("Errore durante la malloc. (get_parameter)\n");
            free(line);
            return ptr;
        }
        if (strtok(line, ":") == NULL) {
            perror("Errore nella struttura del file di configurazione. (get_parameter)\n");
            free(line);
            return ptr;
        }
        sprintf(ptr, "%s",strtok(NULL, "\n"));
        free(line);
        return ptr;
    }
    return NULL;
}

int is_file(char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        return 0;
    }
    return S_ISREG(path_stat.st_mode);
}

int is_daemon(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        char *input = argv[i];
        if (strncmp(input, DAEMON_FLAG, strlen(DAEMON_FLAG)) == 0) {
            return 1;
        }
    }
    return 0;
}

char *get_client_buffer(int socket, int *n) {
    int size = 32, chunk = 32, len = 0, max_size = 512;
    char *client_buffer = (char*)calloc(size, sizeof(char));
    if (client_buffer == NULL) {
        free(client_buffer);
        perror("Errore durante la malloc. (get_client_buffer)\n");
        *n = -1;
        return NULL;
    }

    while ((*n = _recv(socket, client_buffer + len, chunk, 0)) > 0 ) {
        for (int i = len; i < len + *n -1; i++) {
            if (client_buffer[i] == '\r' && client_buffer[i+1] == '\n') {
                client_buffer[i] = '\0';
                return client_buffer;
            }
        }

        len += *n;

        if (len + chunk >= size) {
            size *= 2;
            client_buffer = (char*)realloc(client_buffer, size);
            if (client_buffer == NULL) {
                free(client_buffer);
                perror("Errore durante la realloc. (get_client_buffer)\n");
                *n = -1;
                return NULL;
            }
        }

        if (len > max_size) {
            free(client_buffer);
            *n = -1;
            return NULL;
        }
    }
    return client_buffer;
}

int write_infos() {
    char data[MAX_INFOS_SIZE];
    FILE *file = fopen(INFO_PATH, "w");
    if(file == NULL) {
        fprintf(stderr,"Impossibile leggere il file di infos.\n");
        return -1;
    }
    sprintf(data, "%s:%d", config->server_type, config->main_pid);
    char *err = "Impossibile scrivere sul file: infos.txt.\n";
    if (fputs(data, file) == EOF) {
        perror(err);
        fclose(file);
        return -1;
    }
    if (fclose(file) == EOF) {
        perror(err);
        return -1;
    }
    return 0;
}

char *get_file_listing(char *route, char *path) {
    DIR *d;
    struct dirent *dir;
    int len = 0;
    int row_size = MAX_EXT_LENGTH + MAX_FILENAME_LENGTH + strlen(route) + MAX_IP_SIZE + MAX_PORT_LENGTH + 1;
    int size = row_size;
    char *buffer = (char*)calloc(row_size,sizeof(char));
    if (buffer == NULL) {
        perror("Errore durante la calloc del buffer. (get_file_listing)\n");
        return buffer;
    }
    if ((d = opendir (path)) != NULL) {
        while ((dir = readdir (d)) != NULL) {
            if (equals(dir->d_name, ".") || equals(dir->d_name, "..")) {
                continue;
            }
            len += sprintf(buffer + len,
                           "%s%s\t%s\t%s\t%d\n",
                           get_extension_code(dir->d_name),
                           dir->d_name,
                           route,
                           config->server_ip,
                           config->server_port);
            if (len + row_size >= size) {
                size *= 2;
                buffer = (char *)realloc(buffer, size);
                if (buffer == NULL) {
                    perror("Errore durante la realloc del buffer. (get_file_listing)\n");
                    rewinddir(d);
                    closedir (d);
                    return buffer;
                }
            }
        }
        strcat(buffer + len, ".\n");
        rewinddir(d);
        closedir (d);
    }
    else {
        free(buffer);
        return NULL;
    }
    return buffer;
}