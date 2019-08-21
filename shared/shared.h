#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include "../core/core.h"

#define PUBLIC_PATH "../public"
#define LOG_PATH "../log.txt"
#define BACKLOG 32
#define LOG_MIN_SIZE 38
#define COMPLETE 0
#define PORT_ONLY 1
#define MAX_EXT_LENGTH 3
#define MAX_PORT_LENGTH 5
#define MAX_NAME_LENGTH 256
#define equals(str1, str2) strcmp(str1,str2) == 0

#if defined(__linux__) || defined(__APPLE__)
    #define UNIX_OS 1
#elif defined(_WIN32)
    #define UNIX_OS 0
#endif

typedef struct configuration {
    char *server_type;
    char *server_ip;
    unsigned int server_port;
} configuration;

configuration conf;



int load_arguments(int argc, char *argv[]);
int load_configuration(int first_start);
int index_of(char *values, char find);
int is_file(char *path);
char *get_extension_code(const char *filename);
char *get_client_buffer(int client_fd, int *err);
char *get_parameter(char *line, FILE *stream);
char *get_file_listing(char *route, char *path, int *size);
void restart();
void _log();
