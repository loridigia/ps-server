#include <arpa/inet.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include "../shared/shared.h"

pthread_t pthread;
pthread_attr_t pthread_attr;
pthread_mutex_t mutex;
pthread_cond_t condition;
int pipe_fd[2];

typedef struct pthread_arg_sender {
    int size;
    int client_fd;
    int port;
    char *file_in_memory;
    char *route;
    char *client_ip;
} pthread_arg_sender;

typedef struct pthread_arg_listener {
    int port;
} pthread_arg_listener;

typedef struct pthread_arg_receiver {
    int new_socket_fd;
    int port;
    char *client_ip;
} pthread_arg_receiver;

void daemon_skeleton();
int is_daemon();