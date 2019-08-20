/* ---------------------------- UNIX ----------------------------- */
int listen_on(int port, int *socket_fd, struct sockaddr_in *socket_addr);
int send_error(int socket_fd, char *err);
int send_file(int socket_fd, char *file, size_t file_size);
char *get_client_ip(struct sockaddr_in *socket_addr);
char *get_client_buffer(int client_fd, int *err);

/* --------------------------- WINDOWS --------------------------- */
int listen_on();
int send_error();
int send_file();
char *get_client_ip();
char *get_client_buffer();

/* --------------------------- COMMONS --------------------------- */
char *get_server_ip();
void start();
void log_routine();
void init(int argc, char *argv[]);