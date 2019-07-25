cmake_minimum_required(VERSION 3.14)
project(ps_server C)

set(CMAKE_C_STANDARD 99)

add_executable(ps_server main.c commons/commons.h commons/commons.c)