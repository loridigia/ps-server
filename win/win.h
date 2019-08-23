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

typedef struct thread_arg_receiver {
    SOCKET socket;
    SOCKET *client_id;
    int port;
    char *client_ip;
} thread_arg_receiver;

DWORD WINAPI listener_routine(void *args);
DWORD WINAPI receiver_routine(void *args);
size_t getline(char **lineptr, size_t *n, FILE *stream);

STARTUPINFO info;
PROCESS_INFORMATION process_info;

HANDLE h_pipe;
DWORD dw_written;

DWORD WINAPI listener_routine(void *arg);
size_t getline(char **lineptr, size_t *n, FILE *stream);
int write_on_pipe(char *buffer);