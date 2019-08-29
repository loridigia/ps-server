#include <stdio.h>
#include "win.h"

extern configuration *config;

int main(int argc, char *argv[]) {
    HANDLE handle_mapped_file;

    printf("listener ON \n");
    int port = atoi(argv[0]);

    //READ_ONLY from shared mem config
    handle_mapped_file = OpenFileMapping(FILE_MAP_READ, FALSE, "Global\\Config");
    if (handle_mapped_file == NULL){
        perror("Errore nell'aprire memory object listener");
        exit(1);
    }
    config = (configuration *) MapViewOfFile(handle_mapped_file, FILE_MAP_READ, 0, 0, BUF_SIZE);
    if (config == NULL){
        perror("Errore nel mappare la view del file");
        CloseHandle(handle_mapped_file);
        exit(1);
    }
    
    handle_requests(port, work_with_processes);
    sleep(5);

}
