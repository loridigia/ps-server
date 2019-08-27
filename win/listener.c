#include <stdio.h>
#include "win.h"

int main(int argc, char *argv[]) {
    unsigned int port = conf.server_port;
    printf("%u", &conf.server_port);
}
