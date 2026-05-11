#ifndef CONNECTION_H
#define CONNECTION_H

#define BUFF_SIZE 4096

typedef struct {
    int fd;

    char read_buf[BUFF_SIZE];

    int read_len;
} connection_t;

#endif