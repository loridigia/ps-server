#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#define pipename "\\\\.\\pipe\\LogPipe"

int main(int argc, _TCHAR *argv[]) {
    printf("logger process");
    while (1)
    {
        HANDLE pipe = CreateNamedPipe(pipename, PIPE_ACCESS_INBOUND | PIPE_ACCESS_OUTBOUND , PIPE_WAIT, 1, 1024, 1024, 120 * 1000, NULL);
        if (pipe == INVALID_HANDLE_VALUE)
        {
            printf("Error: %d", GetLastError());
        }
        char data[1024];
        DWORD numRead;
        ConnectNamedPipe(pipe, NULL);
        ReadFile(pipe, data, 1024, &numRead, NULL);
        if (numRead > 0)
            printf("%s\n", data);
        CloseHandle(pipe);
    }
}
