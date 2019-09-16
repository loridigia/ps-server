#include <stdio.h>
#include "win.h"

DWORD WINAPI handler_routine();
BOOL WINAPI CtrlHandlerListener(DWORD fdwCtrlType);

extern configuration *config;


int main(int argc, char *argv[]) {

    SetConsoleCtrlHandler(CtrlHandlerListener, TRUE);

    int port = atoi(argv[0]);

    printf("listener ON port: %d\n", port);

    //READ_ONLY from shared mem config
    get_shared_config(config);

    handle_requests(port, work_with_processes);

    printf("listener di %d out\n", port);

}

BOOL WINAPI CtrlHandlerListener(DWORD fdwCtrlType){
    if (fdwCtrlType == CTRL_BREAK_EVENT) {
        return TRUE;
    }
}

