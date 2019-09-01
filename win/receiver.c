#include <stdio.h>
#include <windows.h>
#include "win.h"

int main(int argc, char *argv[]) {
    CHAR chBuf[2048];
    WSAPROTOCOL_INFO protocolInfo;
    DWORD dwRead;
    HANDLE hStdin, hStdout;

    int port = atoi(argv[0]);
    char *ip = argv[1];

    //Open STDERR / STDOUT
    freopen("CONOUT$", "w", stderr);
    freopen("CONOUT$", "w", stdout);

    //Get handles
    hStdout = CreateFile("CONOUT$",  GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    if ((hStdout == INVALID_HANDLE_VALUE) || (hStdin == INVALID_HANDLE_VALUE)){
        perror("invalid handles");
        exit(1);
    }

    SetStdHandle(STD_OUTPUT_HANDLE, hStdout);
    SetStdHandle(STD_ERROR_HANDLE, hStdout);

    printf("receiver ON");

    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD( 2, 0 );
    WSAStartup( wVersionRequested, &wsaData );

    ReadFile(hStdin, &protocolInfo, sizeof(protocolInfo), &dwRead, NULL);
    SOCKET socket = WSASocket(protocolInfo.iAddressFamily, protocolInfo.iSocketType, protocolInfo.iProtocol, &protocolInfo, 0, WSA_FLAG_OVERLAPPED);

    //printf("family: %d - type: %d - protocol: %d", protocolInfo.iAddressFamily, protocolInfo.iSocketType, protocolInfo.iProtocol);

    if( socket == INVALID_SOCKET) {
        printf("errore socket: %lu", WSAGetLastError());
        exit(1);
    }
    serve_client(socket, ip, port);
    printf("-child=%d-", protocolInfo.iSocketType);
    
}
