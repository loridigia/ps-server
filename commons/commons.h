#define PUBLIC_PATH "../public"
#define LOG_PATH "../log.txt"
#define BACKLOG 32
#define COMPLETE 0
#define PORT_ONLY 1
#define equals(str1, str2) strcmp(str1,str2) == 0

#include <errno.h>

typedef struct configuration {
    char *type;
    char *ip_address;
    unsigned int port;
} configuration;

configuration *config;

void load_arguments(int argc, char *argv[]);
int load_configuration(int first_start);

int listen_on(int port, int *socket_fd, struct sockaddr_in *socket_addr);
int send_error(int socket_fd, char *err);
int is_file(char *path);
int index_of(char *values, char find);
int send_file(int socket_fd, char *file, size_t file_size);

char *get_parameter(char *line, FILE *stream);
char *get_ip();
char *get_client_ip(struct sockaddr_in *socket_addr);
char *get_client_buffer(int client_fd);
char *get_extension_code(const char *filename);
char *get_file_listing(char *route, char *path, char *buffer);

