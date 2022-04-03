#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "common.h"

int server_socket = -1;
int peer_socket = -1;

void cleanup() {
    if (server_socket != -1) {
        close(server_socket);
    }
    if (peer_socket != -1) {
        close(peer_socket);
    }
}

int main(int argc, char *argv[]) {
    if (atexit(cleanup)) {
        PERROR_EXIT("setup() error")
        perror("Error while setup");
        exit(1);
    }

    server_socket = socket(DOMAIN, SOCK_STREAM, 0);
    if (server_socket == -1) {
        PERROR_EXIT("socket() error")
    }

    struct sockaddr_in server_address = {.sin_family = DOMAIN,
                                         .sin_addr.s_addr = htonl(INADDR_ANY),
                                         .sin_port = htons(DEFAULT_PORT)};

    if (bind(server_socket, (struct sockaddr *)&server_address,
             sizeof(server_address))) {
        PERROR_EXIT("bind() error")
    }

    if (listen(server_socket, MAX_NO_CLIENTS)) {
        PERROR_EXIT("listen() error")
    }

    printf("Listening...\n");

    // client handler
    // exit -- forbidden
    while (1) {
        struct sockaddr_in peer_addr;

        socklen_t peer_addr_len = sizeof(peer_addr);
        if ((peer_socket = accept(server_socket, (struct sockaddr *)&peer_addr,
                                  &peer_addr_len)) == -1) {
            perror("Can't accept client");
            continue;
        }

        printf("Client accepted\n");

        ssize_t len;
        // ----- File size -----

        off_t file_size;
        len = recv(peer_socket, &file_size, sizeof(file_size), 0);
        if (len == -1) {
            perror("recv error");
            continue;
        } else if (len != sizeof(file_size)) {
            fprintf(stderr,
                    "Didn't receive full package, expected: %zd, got: %zd\n",
                    sizeof(file_size), len);
            continue;
        }

        // ----- File name -----

        char file_name[NAME_MAX];
        len = recv(peer_socket, file_name, sizeof(file_name), 0);
        if (len == -1) {
            perror("recv error");
            continue;
        } else if (len != sizeof(file_name)) {
            fprintf(stderr,
                    "Didn't receive full package, expected: %zd, got: %zd\n",
                    sizeof(file_name), len);
            continue;
        }

        // ----- File data -----
        int fd_out = open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd_out == -1) {
            perror("open error");
            continue;
        }
        size_t size = file_size;
        ssize_t write_len;
        ssize_t read_len;
        char buffer[BUFFER_SIZE];
        do {
            read_len = recv(peer_socket, buffer, sizeof(buffer), 0);
            if (read_len == -1) {
                perror("recv error");
                exit(EXIT_FAILURE);
            }
            write_len = write(fd_out, buffer, read_len);
            if (write_len == -1) {
                perror("write error");
                exit(EXIT_FAILURE);
            }
            size -= write_len;
        } while (size > 0 && write_len > 0);
        if (size != 0) {
            fprintf(stderr,
                    "Didn't receive full package, expected: %zd, got: %zd\n",
                    file_size, file_size - size);
            continue;
        }
        // -----

        printf("File size: %ld\n", file_size);
        printf("File name: %s\n", file_name);

        close(peer_socket);
    }
    //

    exit(EXIT_SUCCESS);
}