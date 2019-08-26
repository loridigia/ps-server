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
#define logger_event_name "\\\\.\\logger\\Logger_event"

typedef struct thread_arg_receiver {
    SOCKET socket;
    int port;
    char *client_ip;
} thread_arg_receiver;

typedef struct thread_arg_sender {
    /*
    args->client_ip = client_ip;
    args->client_fd = client_fd;
    args->port = port;
    //args->file_in_memory = file_in_memory;
    args->route = client_buffer;
    args->size = size;
    */
} thread_arg_sender;

STARTUPINFO info;
PROCESS_INFORMATION process_info;
HANDLE logger_event;
HANDLE h_pipe;
DWORD dw_written;

int write_on_pipe(char *buffer);
DWORD WINAPI listener_routine(void *args);
DWORD WINAPI receiver_routine(void *args);
size_t getline(char **lineptr, size_t *n, FILE *stream);
