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

void daemon_skeleton();