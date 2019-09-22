#include <stdio.h>
#include "win.h"

#define PIPENAME "\\\\.\\pipe\\LogPipe"

extern configuration *config;

int main(int argc, char *argv[]) {
    WSAPROTOCOL_INFO protocolInfo;
    DWORD dwRead;
    HANDLE hStdin, hStdout;

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
        perror("Errore nella creazione della pipe");
        exit(1);
    }

    freopen("CONOUT$", "w", stderr);
    freopen("CONOUT$", "w", stdout);

    hStdout = CreateFile("CONOUT$",  GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    if ((hStdout == INVALID_HANDLE_VALUE) || (hStdin == INVALID_HANDLE_VALUE)){
        perror("invalid handles");
        exit(1);
    }

    SetStdHandle(STD_OUTPUT_HANDLE, hStdout);
    SetStdHandle(STD_ERROR_HANDLE, hStdout);

    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD( 2, 0 );
    WSAStartup( wVersionRequested, &wsaData );

    ReadFile(hStdin, &protocolInfo, sizeof(protocolInfo), &dwRead, NULL);
    SOCKET socket = WSASocket(protocolInfo.iAddressFamily, protocolInfo.iSocketType, protocolInfo.iProtocol, &protocolInfo, 0, WSA_FLAG_OVERLAPPED);

    if (socket == INVALID_SOCKET) {
        printf("errore socket: %lu", WSAGetLastError());
        exit(1);
    }

    serve_client(socket, ip, port);
    closesocket(socket);
}
