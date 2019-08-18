#include "linux.h"

void daemon_skeleton() {
    /* Our process ID and Session ID */
    pid_t pid, sid;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        exit(1);
    }
    /* If we got a good PID, then
       we can exit the parent process. */
    if (pid > 0) {
        exit(0);
    }

    /* Change the file mode mask */
    umask(0);

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        /* Log the failure */
        exit(1);
    }

    int my_pid = getpid(); //debugging purpose
    fprintf(stdout,"Server running...\n PID -> %d\n",my_pid);

    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

void init(int argc, char *argv[]) {
    daemon_skeleton();
    config = mmap(NULL, sizeof config, PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (load_configuration(COMPLETE) == -1 || load_arguments(argc,argv) == -1) {
        exit(1);
    }

    if (pthread_attr_init(&pthread_attr) != 0) {
        perror("Errore nell'inizializzazione degli attributi del thread.\n");
        exit(1);
    }

    if (pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED) != 0) {
        perror("Errore nel settare DETACHED_STATE al thread.\n");
        exit(1);
    }

    if (pipe(pipe_fd) == -1) {
        perror("Errore nel creare la pipe.\n");
        exit(1);
    }

    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&mutex, &mutexAttr);

    pthread_condattr_t condAttr;
    pthread_condattr_setpshared(&condAttr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&condition, &condAttr);

    pid_t pid_child = fork();

    if (pid_child < 0) {
        perror("Errore durante la fork.\n");
        exit(0);
    } else if (pid_child == 0) {
        write_log();
        exit(0);
    }

    start();
    signal(SIGHUP, restart);
    while(1) sleep(1);
}