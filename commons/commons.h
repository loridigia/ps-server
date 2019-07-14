#define PUBLIC_PATH "../public"
#define BACKLOG 32
#define equals(str1, str2) strcmp(str1,str2) == 0

typedef struct configuration {
    char *type;
    char *ip_address;
    unsigned int port;
} configuration;

void init_server(configuration *config, int argc, char *argv[]);
void read_arguments(configuration *config, int argc, char *argv[]);
void read_configuration(configuration *config);
char *get_parameter(char *line, FILE *stream);
char *get_ip();
int send_error(int socket_fd, char *err);
int is_file(char *path);
char *extract_route(char *buffer);
