#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include "../core/core.h"
#include "../shared/shared.h"
#pragma comment(lib,"ws2_32.lib")
#define pipename "\\\\.\\pipe\\LogPipe"

//memory object stuff
#define BUF_SIZE 256

STARTUPINFO info;
PROCESS_INFORMATION listener_info;
PROCESS_INFORMATION logger_info;
HANDLE logger_event;
HANDLE h_pipe;
DWORD dw_written;
HANDLE mutex;

DWORD WINAPI listener_routine(void *args);
DWORD WINAPI receiver_routine(void *args);
size_t getline(char **lineptr, size_t *n, FILE *stream);
