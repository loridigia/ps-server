#if defined(__linux__) || defined(__APPLE__)
    #include "unix/unix.h"
#elif defined(_WIN32)
    #include "win/win.h"
#endif

//#include "core/debug.c"


int main(int argc, char *argv[]) {
    init(argc, argv);
}