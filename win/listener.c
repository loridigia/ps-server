#include <stdio.h>
#include "win.h"

int main(int argc, char *argv[]) {
    int *port = (int*)argv[0];
    handle_requests(port, work_with_processes());

    printf("%d", port);
}
