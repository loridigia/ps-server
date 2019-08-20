#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "shared.h"

#define CONFIG_PATH "../config.txt"
#define PORT_FLAG "--port="
#define TYPE_FLAG "--type="
#define HELP_FLAG "--help"
#define MIN_PORT 1024
#define MAX_PORT 65535

int load_arguments(int argc, char *argv[]) {
    char *input;
    for (int i = 1; i < argc; i++) {
        input = argv[i];
        char *endptr;
        if (strncmp(input, PORT_FLAG, strlen(PORT_FLAG)) == 0) {
            config->server_port = strtol(input + strlen(PORT_FLAG), &endptr, 10);
            if (config->server_port < MIN_PORT || config->server_port > MAX_PORT || *endptr != '\0' || endptr == (input + strlen(PORT_FLAG))) {
                fprintf(stderr,"Controllare che la porta sia scritta correttamente o che non sia well-known (< 1024). \n");
                return -1;
            }
        } else if (strncmp(input, TYPE_FLAG, strlen(TYPE_FLAG)) == 0) {
            config->server_type = input + strlen(TYPE_FLAG);
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

    char *port_param = get_parameter(line,stream);
    char *endptr;
    config->server_port = strtol(port_param, &endptr, 10);
    if (config->server_port < 1024 || config->server_port > MAX_PORT || *endptr != '\0' || endptr == port_param) {
        fprintf(stderr,"Controllare che la porta sia scritta correttamente o che non sia well-known. \n");
        fclose(stream);
        return -1;
    }
    if (mode == PORT_ONLY) {
        fclose(stream);
        return 0;
    }
    char *type_param = get_parameter(line,stream);
    config->server_type = type_param;
    if (!(equals(config->server_type, "thread") || equals(config->server_type, "process"))) {
        fprintf(stderr,"Seleziona una modalità corretta di avvio (thread o process).\n");
        fclose(stream);
        return -1;
    }
    config->server_ip = get_server_ip();
    if (config->server_ip == NULL) {
        perror("Impossibile ottenere l'ip del server.\n");
        fclose(stream);
        return -1;
    }
    fclose(stream);
    return 0;
}

char *get_extension_code(const char *filename){
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
    const char *ptr = strchr(values, find);
    return ptr ? ptr-values : -1;
}

void restart() {
    if (load_configuration(PORT_ONLY) != -1) {
        start();
    }
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