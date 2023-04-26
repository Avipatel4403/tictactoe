#define _POSIX_C_SOURCE 200809L
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <assert.h>



int connect_inet(char *host, char *service) {
    struct addrinfo hints, *info_list, *info;
    int sock, error;
    // look up remote host
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;      // in practice, this means give us IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;  // indicate we want a streaming socket
    error = getaddrinfo(host, service, &hints, &info_list);
    if (error) {
        fprintf(stderr, "error looking up %s:%s: %s\n", host, service,
                gai_strerror(error));
        return -1;
    }
    for (info = info_list; info != NULL; info = info->ai_next) {
        sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
        if (sock < 0) continue;
        error = connect(sock, info->ai_addr, info->ai_addrlen);
        if (error) {
            close(sock);
            continue;
        }
        break;
    }
    freeaddrinfo(info_list);
    if (info == NULL) {
        fprintf(stderr, "Unable to connect to %s:%s\n", host, service);
        return -1;
    }
    return sock;
}


#define BUFSIZE 512



int main(int argc, char **argv) {

    int sock;
    if (argc != 3) {
        printf("Specify host and service\n");
        exit(EXIT_FAILURE);
    }

    sock = connect_inet(argv[1], argv[2]);
    if (sock < 0) exit(EXIT_FAILURE);

    char buffer[BUFSIZE];
    int bytes;

    printf("Enter Play Message:\n");

    // Reads Play from terminal
    bytes = read(STDIN_FILENO, buffer, BUFSIZE);
    // Sends Play to Server
    write(sock, buffer, bytes-1);
    memset(buffer, 0, BUFSIZE);

    // Read Wait
    bytes = read(sock, buffer, BUFSIZE);
    printf("Data Received from Server: %s\n", buffer);
    memset(buffer, 0, BUFSIZE);

    // Read Begin
    bytes = read(sock, buffer, BUFSIZE);
    printf("Data Received from Server: %s\n", buffer);

    if(buffer[7] == 'X') {
        printf("You are first, write a message\n");
        memset(buffer, 0, BUFSIZE);
        goto Write;
    } else { 
        printf("You are second, waiting for opponent\n");
        memset(buffer, 0, BUFSIZE);
        goto Read; }

    while(1) {
        Read:
        bytes = read(sock, buffer, BUFSIZE);
        if(bytes == 0) {
            break;`
        }
        printf("Data Received from Server: %s\n", buffer);
        memset(buffer, 0, BUFSIZE);

        Write:
        bytes = read(STDIN_FILENO, buffer, BUFSIZE);
        write(sock, buffer, bytes-1);
        memset(buffer, 0, BUFSIZE);
    }

    close(sock);
    return EXIT_SUCCESS;
}
