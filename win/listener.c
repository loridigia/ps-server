#include <stdio.h>
#include "win.h"

BOOL WINAPI CtrlHandlerListener(DWORD fdwCtrlType);
extern configuration *config;

int main(int argc, char *argv[]) {

    SetConsoleCtrlHandler(CtrlHandlerListener, TRUE);

    int port = atoi(argv[0]);

    if (get_shared_config() < 0) {
        exit(1);
    }
    handle_requests(port, work_with_processes);
}

BOOL WINAPI CtrlHandlerListener(DWORD fdwCtrlType){
    if (fdwCtrlType == CTRL_BREAK_EVENT) {
        return TRUE;
    }
}

