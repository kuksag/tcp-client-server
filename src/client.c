#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "common.h"

int fd = -1;
int server_socket = -1;

void cleanup() {
    if (fd != -1) {
        close(fd);
    }
    if (server_socket != -1) {
        close(server_socket);
    }
}

char file_name_to_save[NAME_MAX];

void send_wrapper(void *data, size_t size) {
    ssize_t sent_bytes = send(server_socket, data, size, 0);
    if (sent_bytes == -1) {
        perror("send");
        exit(EXIT_FAILURE);
    }
    if ((size_t)sent_bytes != size) {
        fprintf(stderr, "Bad, sent %zd out of %zu bytes", sent_bytes, size);
        exit(EXIT_FAILURE);
    }
    printf("Ok, sent %zd bytes\n", sent_bytes);
}

void send_file_wrapper(size_t size) {
    size_t len = size;
    ssize_t ret;
    off_t offset = 0;
    do {
        ret = sendfile(server_socket, fd, &offset, len);
        if (ret == -1) {
            PERROR_EXIT("sendfile")
        }
        len -= ret;
    } while (len > 0 && ret > 0);
    if (len != 0) {
        fprintf(stderr, "Bad, sent %zu out of %zu bytes", size - len, size);
        exit(EXIT_FAILURE);
    }
    printf("Ok, sent %zu bytes\n", size);
}

void parse_args(int argc, char *argv[], struct sockaddr_in *server_address) {
    int opt;
    while ((opt = getopt(argc, argv, "a:p:n:")) != -1) {
        int code;
        char *endptr;
        switch (opt) {
            case 'a':
                code = inet_pton(DOMAIN, optarg, &server_address->sin_addr);
                if (code == 0) {
                    fprintf(stderr, "Bad address: %s\n", optarg);
                    exit(EXIT_FAILURE);
                }
                if (code == -1) {
                    fprintf(stderr, "Bad domain: %s\n", optarg);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'p':
                server_address->sin_port =
                    htons(strtol(optarg, &endptr, DECIMAL_BASE));
                if (*endptr != '\0') {
                    fprintf(stderr, "Bad port: %s\n", optarg);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'n':
                snprintf(file_name_to_save, sizeof(file_name_to_save), "%s",
                         optarg);
                break;
            default:
                fprintf(stderr,
                        "Usage: %s [-a address] [-p port] [-n "
                        "result_name] filename\n",
                        argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char *argv[]) {
    if (atexit(cleanup)) {
        perror("Error while setup");
        exit(1);
    }

    struct sockaddr_in server_address = {
        .sin_family = DOMAIN,
        .sin_addr.s_addr = DEFAULT_ADDR,
        .sin_port = htons(DEFAULT_PORT),
    };

    parse_args(argc, argv, &server_address);
    if (optind >= argc) {
        fprintf(stderr, "Expected argument after options\n");
        exit(EXIT_FAILURE);
    }

    char *filename = argv[optind];
    if (strlen(file_name_to_save) == 0) {
        snprintf(file_name_to_save, sizeof(file_name_to_save), "%s",
                 argv[optind]);
    }

    if ((fd = open(filename, O_RDONLY)) == -1) {
        fprintf(stdout, "open(%s) error: %s\n", filename, strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct stat stat_buffer;
    if (fstat(fd, &stat_buffer)) {
        PERROR_EXIT("fstat")
    }

    server_socket = socket(DOMAIN, SOCK_STREAM, 0);
    if (server_socket == -1) {
        PERROR_EXIT("socket")
    }

    int socket_opt = -1;
    if (setsockopt(server_socket, IPPROTO_TCP, TCP_CORK, &socket_opt,
                   sizeof(socket_opt))) {
        PERROR_EXIT("setsockopt")
    }

    if (connect(server_socket, (struct sockaddr *)&server_address,
                sizeof(server_address))) {
        PERROR_EXIT("connect")
    }

    // ----- File size -----

    printf("Sending file size...\n");
    send_wrapper(&stat_buffer.st_size, sizeof(stat_buffer.st_size));

    // ----- File name -----

    printf("Sending file name...\n");
    send_wrapper(file_name_to_save, sizeof(file_name_to_save));

    // ----- File data -----

    printf("Sending file data...\n");
    send_file_wrapper(stat_buffer.st_size);

    // -----
    printf("Closing socket...\n");
    exit(EXIT_SUCCESS);
}
