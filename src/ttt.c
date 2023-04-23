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

    while(1){

        bytes = read(STDIN_FILENO , buffer, BUFSIZE);
        if(bytes >= BUFSIZE){
            printf("Message to long\n");
            break;
        }

        //work with whats been taken from user 
        char temp[BUFSIZE];
        memset(temp,0,BUFSIZE);
        temp[0] = buffer[0];
        temp[1] = buffer[1];
        temp[2] = buffer[2];
        temp[3] = buffer[3];
        temp[4] = '|';

        int index = 5;
        int size = bytes - 5;
        char num[4];
        sprintf(num,"%d",size);
        for(int i = 0;num[i] != '\0';i++){
            temp[index] = num[i];
            index++;
        }
        temp[index] = '|';
        index++;

        for(int i = 5;i < bytes -1 ;i++){
            if(buffer[i] == ' ' && buffer[0] != 'P'){
                temp[index++] = '|';
                index++;                
            }
            else{
                temp[index] = buffer[i];
                index++;
            }
        }

        if(size != 0){
            temp[index] = '|';
            index++;
        }

        // //print temp
        // for(int i = 0;i < index;i++){
        //     printf("%d: %c\n",i, temp[i]);
        // }

    

        //send message to server
        send(sock,temp,index,0);
        //clear temp
        memset(temp, 0, BUFSIZE);

        //get message from server
        memset(buffer,0,BUFSIZE);
        recv(sock, buffer, BUFSIZE,0);
        printf("Data received from the server: %s\n", buffer);
        memset(buffer, 0, BUFSIZE);  // clear buffer
    }

    close(sock);
    return EXIT_SUCCESS;
}
