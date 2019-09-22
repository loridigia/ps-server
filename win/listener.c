#include <stdio.h>
#include "win.h"

extern configuration *config;
BOOL WINAPI CtrlHandler2(DWORD fdwCtrlType);

int main(int argc, char *argv[]) {

    if (SetConsoleCtrlHandler(CtrlHandler2, TRUE) == 0) {
        print_error("Impossibile aggiungere la funzione di handling per CTRL+BREAK");
        exit(1);
    }

    int port = atoi(argv[0]);

    if (get_shared_config() < 0) {
        exit(1);
    }
    handle_requests(port, work_with_processes);
}

BOOL WINAPI CtrlHandler2(DWORD fdwCtrlType){
    if (fdwCtrlType == CTRL_BREAK_EVENT) {
        return TRUE;
    }
}

