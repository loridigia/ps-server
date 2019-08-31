#ifdef DEVELOPMENT
    #define CONFIG_PATH "../config.txt"
    #define INFO_PATH   "../info.txt"
    #define LOG_PATH    "../log.txt"
    #define PUBLIC_PATH "../public"
    #define CORE_PATH   "../core/core.h"
#else
    #define CONFIG_PATH "config.txt"
    #define INFO_PATH   "info.txt"
    #define LOG_PATH    "log.txt"
    #define PUBLIC_PATH "public"
    #define CORE_PATH   "core/core.h"
#endif

#define BACKLOG 128
#define COMPLETE 0
#define PORT_ONLY 1
#define CHUNK 32
#define MAX_TYPE_LENGTH 10
#define MAX_IP_SIZE 20
#define MAX_EXT_LENGTH 3
#define MAX_PORT_LENGTH 6
#define MAX_FILENAME_LENGTH 256
#define MIN_PORT 1024
#define MAX_PORT 65535

#define PORT_FLAG "--port="
#define TYPE_FLAG "--type="
#define HELP_FLAG "--help"