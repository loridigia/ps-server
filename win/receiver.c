#include <stdio.h>
#include <windows.h>
#include "win.h"

int main(int argc, char *argv[]) {
    int port = atoi(argv[0]);
    char *ip = argv[1];

    CHAR chBuf[256];
    DWORD dwRead, dwWritten;
    HANDLE hStdin, hStdout;

    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    if ((hStdout == INVALID_HANDLE_VALUE) || (hStdin == INVALID_HANDLE_VALUE)){
        perror("invalid handles");
        exit(1);
    }
}
