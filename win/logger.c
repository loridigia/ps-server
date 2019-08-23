#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#define pipename "\\\\.\\pipe\\LogPipe"
#define LOG_PATH "../log.txt"

void _log(char *buffer);

void _log(char *buffer);

int main(int argc, _TCHAR *argv[]) {
    HANDLE h_pipe;
    char buffer[1024];
    DWORD dw_read;

    h_pipe = CreateNamedPipe(pipename,
                             PIPE_ACCESS_DUPLEX,
                            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                             1,
                            1024 * 16,
                            1024 * 16,
                             NMPWAIT_USE_DEFAULT_WAIT,
                             NULL);
    while (h_pipe != INVALID_HANDLE_VALUE){
        if (ConnectNamedPipe(h_pipe, NULL) != FALSE)   // wait here for someone to connect to the pipe
        {
            while (ReadFile(h_pipe, buffer, sizeof(buffer) - 1, &dw_read, NULL) != FALSE){

                /* add terminating zero */
                buffer[dw_read] = '\0';

                /* write on file */
                _log(buffer);
            }
        }

        DisconnectNamedPipe(h_pipe);
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