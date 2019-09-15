#include <stdio.h>
#include "win.h"

extern configuration *config;

int main(int argc, char *argv[]) {

    int port = atoi(argv[0]);

    printf("listener ON port: %d\n", port);

    //READ_ONLY from shared mem config
    get_shared_config(config);

    printf("listener config read: %d\n", config->server_port);

    handle_requests(port, work_with_processes);

    printf("im out");

}
