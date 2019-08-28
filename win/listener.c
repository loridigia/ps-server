#include <stdio.h>
#include "win.h"

extern configuration *config;

int main(int argc, char *argv[]) {
    printf("listner ready");
    int port = atoi(argv[0]);

    handle_requests(port, work_with_processes);

}
