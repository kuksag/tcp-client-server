#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "common.h"

int server_socket = -1;

void cleanup() {
    if (server_socket != -1) {
        close(server_socket);
    }
}

int recv_wrapper(int peer_socket, void *data, size_t size) {
    ssize_t len = recv(peer_socket, data, size, 0);
    if (len == -1) {
        perror("recv");
        return 1;
    } else if (len != size) {
        fprintf(stderr,
                "Didn't receive full package, expected: %zd, got: %zd\n", size,
                len);
        return 1;
    }
    return 0;
}

int recv_file_wrapper(int peer_socket,
                      int fd_out,
                      size_t file_size,
                      void *buffer,
                      size_t buffer_size) {
    size_t size = file_size;
    ssize_t write_len;
    ssize_t read_len;
    do {
        read_len = recv(peer_socket, buffer, sizeof(buffer_size), 0);
        if (read_len == -1) {
            perror("recv error");
            return 1;
        }
        write_len = write(fd_out, buffer, read_len);
        if (write_len == -1) {
            perror("write error");
            return 1;
        }
        size -= write_len;
    } while (size > 0 && write_len > 0);
    if (size != 0) {
        fprintf(stderr,
                "Didn't receive full package, expected: %zd, got: %zd\n",
                file_size, file_size - size);
        return 1;
    }
    return 0;
}

void *handler(void *data) {
    int peer_socket = *(int *)data;
    printf("Client accepted\n");

    // ----- File size -----

    off_t file_size;
    if (recv_wrapper(peer_socket, &file_size, sizeof(file_size))) {
        goto release_and_exit;
    }

    // ----- File name -----

    char file_name[NAME_MAX];
    if (recv_wrapper(peer_socket, file_name, sizeof(file_name))) {
        goto release_and_exit;
    }

    // -----

    printf("File size: %ld\n", file_size);
    printf("Saving %s...\n", file_name);

    // ----- File data -----
    int fd_out = open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd_out == -1) {
        perror("open");
        goto release_and_exit;
    }
    char buffer[BUFFER_SIZE];
    if (recv_file_wrapper(peer_socket, fd_out, file_size, buffer,
                          sizeof(buffer))) {
        goto release_and_exit;
    }

    printf("%s saved\n", file_name);
    // -----

release_and_exit:
    free(data);
    close(peer_socket);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (atexit(cleanup)) {
        PERROR_EXIT("setup() error")
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
    while (1) {
        struct sockaddr_in peer_addr;

        pthread_t tid;
        int peer_socket;
        socklen_t peer_addr_len = sizeof(peer_addr);
        if ((peer_socket = accept(server_socket, (struct sockaddr *)&peer_addr,
                                  &peer_addr_len)) == -1) {
            perror("Can't accept client");
            continue;
        }
        int *arg = malloc(sizeof(peer_socket));
        *arg = peer_socket;
        pthread_create(&tid, NULL, handler, arg);
        pthread_detach(tid);
    }
}