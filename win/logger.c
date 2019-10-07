#include <stdio.h>
#include <windows.h>
#include <zconf.h>
#include "../core/constants.h"
#include CORE_PATH
#define PIPENAME "\\\\.\\pipe\\LogPipe"
#define LOGGER_EVENT "Logger_Event"
#define PIPE_EVENT "Global\\Pipe_Event"

void _log(char *buffer);
BOOL WINAPI CtrlHandler2(DWORD fdwCtrlType);
void print_error(char *err);
void print_WSA_error(char *err);

int main(int argc, char *argv[]) {
    const int PIPE_SIZE = 1024 * 16;
    HANDLE logger_event;
    HANDLE pipe_event;
    HANDLE h_pipe;
    DWORD dw_read;
    int size = MAX_FILENAME_LENGTH + MAX_IP_LENGTH + MAX_PORT_LENGTH + MIN_LOG_LENGTH;

    if (SetConsoleCtrlHandler(CtrlHandler2, TRUE) == 0) {
        print_error("Impossibile aggiungere la funzione di handling per CTRL+BREAK (logger - SetConsoleCtrlHandler)");
        exit(1);
    }

    h_pipe = CreateNamedPipe(PIPENAME,
                             PIPE_ACCESS_DUPLEX,
                             PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                             1,
                             PIPE_SIZE,
                             PIPE_SIZE,
                             NMPWAIT_USE_DEFAULT_WAIT,
                             NULL);
    if (h_pipe == INVALID_HANDLE_VALUE) {
        print_error("Errore durante la creazione della named pipe (logger - CreateNamedPipe)");
        exit(1);
    }

    pipe_event = CreateEventA(NULL, FALSE, FALSE, PIPE_EVENT);
    if (pipe_event == NULL) {
        print_error("Errore nell'operazione di creazione del pipe_event (logger- CreateEventA)");
        exit(1);
    }

    logger_event = OpenEvent(EVENT_MODIFY_STATE, FALSE, LOGGER_EVENT);
    if (logger_event == NULL) {
        print_error("Errore durante l'apertura dell'evento logger (logger - OpenEvent)");
        exit(1);
    }

    if (SetEvent(logger_event) == 0) {
        print_error("Impossibile settare il logger event (logger - SetEvent)");
        exit(1);
    }

    char buffer[size];
    while (TRUE) {
        memset(buffer, 0, size);
        print_error("on wait");

        if (WaitForSingleObject(pipe_event, INFINITE) == WAIT_FAILED) {
            print_error("Errore durante l'attesa del setevent (logger - WaitForSingleObject)");
            exit(1);
        }
        print_error("after wait");

        if (!ReadFile(h_pipe, buffer, sizeof(buffer), &dw_read, NULL)){
            print_error("Errore nel leggere dalla pipe (logger - readfile )");
            exit(1);
        }
        buffer[dw_read] = '\0';
        _log(buffer);
        //ResetEvent(pipe_event);
    }
}

void _log(char *buffer) {
    FILE *file = fopen(LOG_PATH, "a");
    print_error(buffer);
    if (file == NULL) {
        print_error("Errore nell'operazione di scrittura sul log");
        return;
    }
    fprintf(file, "%s", buffer);
    if (fclose(file) == EOF) {
        print_error("Errore durante la chiusura del file di log");
    }
}

BOOL WINAPI CtrlHandler2(DWORD fdwCtrlType){
    if (fdwCtrlType == CTRL_BREAK_EVENT) {
        return TRUE;
    }
}

void print_error(char *err) {
    fprintf(stderr, "%s : %lu\n", err,  GetLastError());
}

void print_WSA_error(char *err) {
    fprintf(stderr,"%s : %d\n",err, WSAGetLastError());
}