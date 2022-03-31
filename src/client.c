#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int file_ds;

void cleanup() {
    if (file_ds != -1) {
        close(file_ds);
    }
}

int main(int argc, char *argv[]) {
    if (atexit(cleanup)) {
        perror("Error while setup");
        exit(1);
    }

    char *filename;
    char *address;
    char *result_name;

    int opt;
    while ((opt = getopt(argc, argv, "a:n:")) != -1) {
        switch (opt) {
            case 'a':
                address = optarg;
                break;
            case 'n':
                result_name = optarg;
                break;
            default:
                fprintf(stderr,
                        "Usage: %s [-a server_address] [-n "
                        "result_name] filename\n",
                        argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Expected argument after options\n");
        exit(EXIT_FAILURE);
    }

    filename = argv[optind];

    if ((file_ds = open(filename, O_RDONLY)) == -1) {
        fprintf(stdout, "Error while opening '%s': %s\n", filename,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct stat stat_buffer;
    if (fstat(file_ds, &stat_buffer)) {
        perror("fstat error");
        exit(EXIT_FAILURE);
    }


    exit(EXIT_SUCCESS);
}