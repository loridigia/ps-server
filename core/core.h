/*
 * INTERFACCIA.
 * I parametri della funzione sono specificati nell'implementazione. (variano a seconda del SO)
 */

int listen_on();
int send_error();
int send_file();
int serve_client();
int work_with_threads();
int work_with_processes();
int write_on_pipe();
char *get_client_ip();
char *get_server_ip();
char *get_client_buffer();
void start();
void log_routine();
void init();
void handle_requests();
size_t _getline(char **lineptr, size_t *n, FILE *stream);