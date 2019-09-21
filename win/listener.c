#include <stdio.h>
#include "win.h"

BOOL WINAPI CtrlHandlerListener(DWORD fdwCtrlType);
extern configuration *config;

int main(int argc, char *argv[]) {

    SetConsoleCtrlHandler(CtrlHandlerListener, TRUE);

    char *endptr;
    int port = strtol(argv[0], &endptr, 10);
    if (*endptr != '\0' || endptr == argv[0]) {
        fprintf(stderr,"Controllare che la porta sia scritta correttamente o che non sia well-known. \n");
        return -1;
    }

    get_shared_config(config);
    handle_requests(port, work_with_processes);
}

BOOL WINAPI CtrlHandlerListener(DWORD fdwCtrlType){
    if (fdwCtrlType == CTRL_BREAK_EVENT) {
        return TRUE;
    }
}

