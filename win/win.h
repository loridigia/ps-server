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

STARTUPINFO info;
PROCESS_INFORMATION processInfo;

HANDLE hPipe;
DWORD dwWritten;

DWORD WINAPI listener_routine(void *arg);
size_t getline(char **lineptr, size_t *n, FILE *stream);
int write_on_pipe(char *buffer);
