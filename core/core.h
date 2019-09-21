/*
 * INTERFACCIA.
 * I parametri della funzione sono specificati nell'implementazione. (variano a seconda del SO)
 */

int listen_on();
int send_error();
int send_file();
int work_with_threads();
int work_with_processes();
int write_on_pipe();
int start();
int restart();
char *get_server_ip();
void log_routine();
void init();
void serve_client();
void handle_requests();
void *send_routine();
int _recv();