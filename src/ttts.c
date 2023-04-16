#define _POSIX_C_SOURCE 200809L
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

volatile int active = 1;

void handler(int signum) {
    active = 0;
}

// set up signal handlers for primary thread
// return a mask blocking those signals for worker threads
// FIXME should check whether any of these actually succeeded
void install_handlers(sigset_t *mask) {
    struct sigaction act;
    act.sa_handler = handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigemptyset(mask);
    sigaddset(mask, SIGINT);
    sigaddset(mask, SIGTERM);
}
// data to be sent to worker threads
struct connection_data {
    struct sockaddr_storage addr;
    socklen_t addr_len;
    int fd;
};

int main(int argc, char **argv) {
    //need to create a single socket for server
    if(argc < 2){
        perror("missing port or socket");
    }

    //server socket
    struct addrinfo server;
    int error,sock;

    memset(&server,0,sizeof(struct addrinfo));
    server.ai_family = AF_UNSPEC;
    server.ai_socktype = SOCK_STREAM;
    server.ai_flags = AI_PASSIVE;

    return EXIT_SUCCESS;

}