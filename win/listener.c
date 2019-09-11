#include <stdio.h>
#include "win.h"

extern configuration *config;

int main(int argc, char *argv[]) {

    printf("listener ON \n");
    int port = atoi(argv[0]);

    //READ_ONLY from shared mem config
    get_shared_config(config);

    handle_requests(port, work_with_processes);

}
