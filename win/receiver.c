#include <stdio.h>
#include <windows.h>
#include "win.h"

int main(int argc, char *argv[]) {
    int port = atoi(argv[0]);
    char *ip = argv[1];
    char *socket = (char *)argv[2];
    printf("%d", port);
    printf("%s", ip);
    printf("%s", socket);
}
