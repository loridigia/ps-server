#include <stdio.h>
#include <windows.h>
#include "win.h"

#define pipename "\\\\.\\pipe\\LogPipe"

extern configuration *config;

int main(int argc, char *argv[]) {
    HANDLE handle_mapped_file;
    WSAPROTOCOL_INFO protocolInfo;
    DWORD dwRead;
    HANDLE hStdin, hStdout;

    // Load argv
    int port = atoi(argv[0]);
    char *ip = argv[1];

    //READ_ONLY from shared mem config
    handle_mapped_file = OpenFileMapping(FILE_MAP_READ, FALSE, "Global\\Config");
    if (handle_mapped_file == NULL){
        perror("Errore nell'aprire memory object receiver");
        exit(1);
    }
    config = (configuration *) MapViewOfFile(handle_mapped_file, FILE_MAP_READ, 0, 0, BUF_SIZE);
    if (config == NULL){
        perror("Errore nel mappare la view del file");
        CloseHandle(handle_mapped_file);
        exit(1);
    }

    //get handles of mutex and pipe
    mutex = CreateMutex(NULL,FALSE,"Global\\Mutex");

    h_pipe = CreateFile(pipename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h_pipe == INVALID_HANDLE_VALUE && GetLastError() != ERROR_PIPE_BUSY){
        perror("Errore nella creazione della pipe");
        exit(1);
    }

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

    if( socket == INVALID_SOCKET) {
        printf("errore socket: %lu", WSAGetLastError());
        exit(1);
    }

    serve_client(socket, ip, port);
    closesocket(socket);

}
