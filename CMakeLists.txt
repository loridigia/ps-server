cmake_minimum_required(VERSION 3.14)
project(ps_server C)

set(CMAKE_C_STANDARD 99)

if (UNIX)
    add_executable(ps_server main.c shared/shared.h shared/shared.c unix/unix.h unix/unix.c)
endif (UNIX)

if (WIN32)
    add_executable(ps_server main.c shared/shared.h shared/shared.c win/win.h win/win.c)
endif (WIN32)