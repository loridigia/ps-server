#include <arpa/inet.h>
#include <sys/mman.h>
#include <ifaddrs.h>
#include <pthread.h>
#include <stdio.h>
#include "../core/core.h"
#include "../shared/shared.h"

int pipe_fd[2];

pthread_t pthread;
pthread_attr_t pthread_attr;
pthread_mutex_t mutex;
pthread_cond_t condition;

void _log(char *buffer);
void daemon_skeleton();
void *listener_routine(void *arg);
void *receiver_routine(void *arg);
