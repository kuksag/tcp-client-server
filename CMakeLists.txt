cmake_minimum_required(VERSION 3.16)

project(client-server-tcp-file-sender C)

#add_executable(backend-test)
#add_executable(server-test)

add_executable(server src/server.c)
add_executable(client src/client.c)