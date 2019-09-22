#include <stdio.h>
#include "win.h"

#define PIPENAME "\\\\.\\pipe\\LogPipe"

extern configuration *config;

int main(int argc, char *argv[]) {
    WSAPROTOCOL_INFO protocol_info;
    HANDLE stdin_handle, stdout_handle;

    int port = atoi(argv[0]);
    char *ip = argv[1];

    if (get_shared_config() < 0) {
        exit(1);
    }

    mutex = CreateMutex(NULL,FALSE, GLOBAL_MUTEX);
    if (mutex == NULL){
        fprintf(stderr,"Errore nell'apertura del mutex. \n");
        exit(1);
    }

    h_pipe = CreateFile(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h_pipe == INVALID_HANDLE_VALUE && GetLastError() != ERROR_PIPE_BUSY){
        print_error("Errore nella creazione della pipe");
        exit(1);
    }

    freopen("CONOUT$", "w", stderr);
    freopen("CONOUT$", "w", stdout);

    stdout_handle = CreateFile("CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (stdout_handle == INVALID_HANDLE_VALUE) {
        print_error("stdout_handle non valido");
        exit(1);
    }

    stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
    if (stdin_handle == INVALID_HANDLE_VALUE) {
        print_error("stdin_handle non valido");
        exit(1);
    }

    if (SetStdHandle(STD_OUTPUT_HANDLE, stdout_handle) == 0 ) {
        print_error("Impossibile settare stdout_handle");
        exit(1);
    }

    if (SetStdHandle(STD_ERROR_HANDLE, stdout_handle) == 0 ) {
        print_error("Impossibile settare stderr_handle");
        exit(1);
    }

    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2,0);
    WSAStartup( wVersionRequested, &wsaData );

    DWORD dw_read;
    ReadFile(stdin_handle, &protocol_info, sizeof(protocol_info), &dw_read, NULL);
    SOCKET socket = WSASocket(protocol_info.iAddressFamily, protocol_info.iSocketType, protocol_info.iProtocol, &protocol_info, 0, WSA_FLAG_OVERLAPPED);

    if (socket == INVALID_SOCKET) {
        printf("errore socket: %lu", WSAGetLastError());
        exit(1);
    }

    serve_client(socket, ip, port);
    closesocket(socket);
}
