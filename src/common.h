#ifndef CLIENT_SERVER_TCP_FILE_SENDER_COMMON_H
#define CLIENT_SERVER_TCP_FILE_SENDER_COMMON_H

#define PERROR_EXIT(s)      \
    {                       \
        perror((s));        \
        exit(EXIT_FAILURE); \
    }

enum SETTINGS {
    DOMAIN = AF_INET,
    DEFAULT_ADDR = INADDR_BROADCAST,
    DEFAULT_PORT = 8080,
    MAX_NO_CLIENTS = 5,
    KILOBYTE = (1 << 10),
    BUFFER_SIZE = 64 * KILOBYTE,
};

#endif  // CLIENT_SERVER_TCP_FILE_SENDER_COMMON_H
