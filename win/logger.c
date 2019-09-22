#include <stdio.h>
#include <windows.h>
#include <zconf.h>

#include "../core/constants.h"
#include CORE_PATH

#define PIPENAME "\\\\.\\pipe\\LogPipe"
#define LOGGER_EVENT "Logger_Event"

void _log(char *buffer);
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType);

int main(int argc, char *argv[]) {

    SetConsoleCtrlHandler(CtrlHandler, TRUE);

    HANDLE logger_event;
    HANDLE h_pipe;
    char buffer[8192];
    DWORD dw_read;

    //init pipe
    h_pipe = CreateNamedPipe(PIPENAME,
                             PIPE_ACCESS_DUPLEX,
                            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                             1,
                            1024 * 16,
                            1024 * 16,
                             NMPWAIT_USE_DEFAULT_WAIT,
                             NULL);

    //prendi l'handle dell'evento
    logger_event = OpenEvent(EVENT_MODIFY_STATE, FALSE, LOGGER_EVENT);
    if (logger_event == NULL) {
        fprintf(stderr, "Errore nell'aprire evento logger");
        exit(1);
    }

    //cambia lo stato dell'evento quando la pipe è creata
    SetEvent(logger_event);

    while (h_pipe != INVALID_HANDLE_VALUE){
        if (ConnectNamedPipe(h_pipe, NULL) != FALSE)   // aspetta qui che qualcosa si connetta alla pipe
        {
            while (ReadFile(h_pipe, buffer, sizeof(buffer), &dw_read, NULL) != FALSE){
                buffer[dw_read] = '\0';
                _log(buffer);
            }
        }
        DisconnectNamedPipe(h_pipe);
    }

    CloseHandle(logger_event);
    CloseHandle(h_pipe);
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

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType){
    if (fdwCtrlType == CTRL_BREAK_EVENT) {
        return TRUE;
    }
}