cmake_minimum_required(VERSION 3.26)
project(Ass1 C)

set(CMAKE_C_STANDARD 11)

# Add debug flags to the CMake configuration
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

add_executable(Ass1
        shell.c)
