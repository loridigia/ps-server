#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <windows.h>
#include <tchar.h>

#include "../shared/shared.h"

#include CONSTANTS
#include CORE_PATH

#pragma comment(lib,"ws2_32.lib")
#define pipename "\\\\.\\pipe\\LogPipe"

//memory object stuff
#define BUF_SIZE 256


STARTUPINFO info;
PROCESS_INFORMATION listener_info;

HANDLE logger_event;
HANDLE listener_event;
HANDLE h_pipe;
HANDLE mutex;
HANDLE hMapFile;


VOID get_shared_config(configuration *config);
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType);
DWORD WINAPI listener_routine(void *args);
DWORD WINAPI receiver_routine(void *args);
DWORD WINAPI sender_routine(void *args);
DWORD create_receiver_process(char *args);
size_t getline(char **lineptr, size_t *n, FILE *stream);
