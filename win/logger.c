#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include "../shared/shared.h"
#define pipename "\\\\.\\pipe\\LogPipe"

int main(int argc, _TCHAR *argv[]) {
    printf("logger process starts");

    HANDLE hPipe;
    char buffer[1024];
    DWORD dwRead;

    hPipe = CreateNamedPipe(pipename,
                            PIPE_ACCESS_DUPLEX,
                            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                            1,
                            1024 * 16,
                            1024 * 16,
                            NMPWAIT_USE_DEFAULT_WAIT,
                            NULL);
    while (hPipe != INVALID_HANDLE_VALUE){
        if (ConnectNamedPipe(hPipe, NULL) != FALSE)   // wait here for someone to connect to the pipe
        {
            while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &dwRead, NULL) != FALSE){

                /* add terminating zero */
                buffer[dwRead] = '\0';

                /* write on file */
                _log(buffer);
            }
        }

        DisconnectNamedPipe(hPipe);
    }
}

void _log(char *buffer) {
    FILE *file = fopen(LOG_PATH, "a");
    if (file == NULL) {
        perror("Errore nell'operazione di scrittura sul log.\n");
        fclose(file);
        return;
    }
    fprintf(file, "%s", buffer);
    fclose(file);
}
