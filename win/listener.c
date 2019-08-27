#include <stdio.h>
#include "win.h"

int main(int argc, char *argv[]) {
    int * port = (int*)argv[0];

    printf("%d", port);
}
