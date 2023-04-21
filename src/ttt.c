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
#define BUFLEN 256
#define BUFSIZE 512

char *lineBuffer;
int linePos, lineSize;

void append(char *buf, int len) {
    int newPos = linePos + len;

    if (newPos > lineSize) {
        lineSize *= 2;

        assert(lineSize >= newPos);
        lineBuffer = realloc(lineBuffer, lineSize);

        if (lineBuffer == NULL) {
            perror("line buffer");
            exit(EXIT_FAILURE);
        }
    }

    memcpy(lineBuffer + linePos, buf, len);
    linePos = newPos;
}

int main(int argc, char **argv) {
    int sock, bytes;
    char buf[BUFLEN];
    if (argc != 3) {
        printf("Specify host and service\n");
        exit(EXIT_FAILURE);
    }

    // sock = 0;
    sock = connect_inet(argv[1], argv[2]);
    if (sock < 0) exit(EXIT_FAILURE);



    int pos, lstart;
    char buffer[BUFSIZE];
    while(1){
        //get the linebuffer to turn into
        while ((bytes = read(STDIN_FILENO, buf, BUFLEN)) > 0) {

            lstart = 0;

            // search for newlines
            for (pos = 0; pos < bytes; ++pos) {
                if (buffer[pos] == '\n') {
                    int thisLen = pos - lstart + 1;
                    append(buffer + lstart, thisLen);
                    linePos = 0;
                    lstart = pos + 1;
                }
            }

        }
        assert(lineBuffer[linePos - 1] == '\n');

        //work with whats been taken from user 
        char temp[BUFSIZE];
        memset(temp,0,BUFSIZE);
        temp[0] = lineBuffer[0];
        temp[1] = lineBuffer[1];
        temp[2] = lineBuffer[2];
        temp[3] = lineBuffer[3];
        temp[4] = '|';
        temp[5] = linePos - 5;
        for(int i = 5;i < linePos;i++){
            if(lineBuffer[i] == ' ' && lineBuffer[0] != 'P'){
                temp[i+1] = '|';                
            }
            else{
                temp[i+1] = lineBuffer[i];
            }
        }

        write(sock, buf, bytes);

        read(sock, buffer, BUFLEN);
        printf("Data received from the server: %s\n", buffer);
        memset(buffer, 0, BUFLEN);  // clear buffer
    }
    close(sock);
    free(lineBuffer);
    return EXIT_SUCCESS;
}
