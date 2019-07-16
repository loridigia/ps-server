#define PUBLIC_PATH "../public"
#define BACKLOG 32
#define equals(str1, str2) strcmp(str1,str2) == 0

typedef struct configuration {
    char *type;
    char *ip_address;
    unsigned int port;
} configuration;

configuration config;

void load_arguments(int argc, char *argv[]);
int load_configuration();
char *get_parameter(char *line, FILE *stream);
char *get_ip();
int send_error(int socket_fd, char *err);
int is_file(char *path);
char *extract_route(char *buffer);
char *get_extension_code(const char *filename);
int listen_on(int port, int *socket_fd, struct sockaddr_in *socket_addr);
char *get_file_listing(char *route, char *path, char *buffer);
