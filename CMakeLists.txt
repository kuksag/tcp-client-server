cmake_minimum_required(VERSION 3.16)
project(client-server-tcp-file-sender C)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_program(clang-tidy clang-tidy)
if (clang-tidy)
    set(CMAKE_C_CLANG_TIDY clang-tidy -checks=-*,readability-*)
endif ()

find_program(cppcheck cppcheck)
if (cppcheck)
    set(CMAKE_C_CPPCHECK cppcheck --enable=all)
endif ()

add_executable(server src/server.c)
add_executable(client src/client.c)
target_link_libraries(server PRIVATE Threads::Threads)
