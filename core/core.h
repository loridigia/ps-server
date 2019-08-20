/* ---------------------------- UNIX ----------------------------- */
int listen_on(int port, int *socket_fd, struct sockaddr_in *socket_addr);
int send_error(int socket_fd, char *err);
int send_file(int socket_fd, char *file, size_t file_size);
char *get_client_ip(struct sockaddr_in *socket_addr);
char *get_client_buffer(int client_fd, int *err);
int serve_client(int client_fd, char *client_ip, int port);
int work_with_threads(int fd, char *client_ip, int port);
int work_with_processes(int fd, char *client_ip, int port);
void handle_requests(int port, int (*handle)(int fd, char* client_ip, int port));

/* --------------------------- WINDOWS --------------------------- */
int listen_on();
int send_error();
int send_file();
char *get_client_ip();
char *get_client_buffer();
int serve_client();
int work_with_threads();
int work_with_processes();
void handle_requests();

/* --------------------------- COMMONS --------------------------- */
char *get_server_ip();
void start();
void log_routine();
void init(int argc, char *argv[]);
void *pthread_receiver_routine(void *arg);
void *pthread_listener_routine(void *arg);
int write_on_pipe(int size, char *name, int port, char *ip);