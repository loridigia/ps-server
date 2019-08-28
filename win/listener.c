#include <stdio.h>
#include "win.h"


extern configuration *config;


int main(int argc, char *argv[]) {
    printf("listner ready");
    int port = atoi(argv[0]);

    HANDLE handle_mapped_file;
    LPCTSTR buffer;

    handle_mapped_file = OpenFileMapping(FILE_MAP_READ, FALSE, "Global\\Config");

    if (handle_mapped_file == NULL){
        perror("Errore nell'aprire memory object listener");
        exit(1);
    }

    buffer = (LPTSTR) MapViewOfFile(handle_mapped_file, FILE_MAP_READ, 0, 0, BUF_SIZE);
    if (buffer == NULL){
        perror("Errore nel mappare la view del file");
        CloseHandle(handle_mapped_file);
        exit(1);
    }

    printf("%s", buffer);
    //handle_requests(port, work_with_processes);
    sleep(5);

}
