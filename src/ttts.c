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

#define EMPTY 0
#define X 1
#define O 2

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

int checkBoard(char** board);

int main(int argc, char **argv) {
   
   
    //need to create a single socket for server
    if(argc < 2){
        perror("missing port or socket");
    }

    //server socket








    return EXIT_SUCCESS;

}

int checkBoard(char** board){
    //check rows 
    int result;
    for(int i = 0;i < 3;i++){
        if (board[i][0] == board[i][1] && board[i][1] == board[i][2]) {
            result = board[i][0];
            break;
        }
    }

    //check diagonal
    if (board[0][0] == board[1][1] && board[1][1] == board[2][2]) {
        result = board[0][0];
    } 
    else if (board[0][2] == board[1][1] && board[1][1] == board[2][0]) {
        result = board[0][2];
    }

    //check columns
    for (int i = 0; i < 3; i++) {
        if (board[0][i] == board[1][i] && board[0][i] == board[i][2]) {
            result = board[0][i];
        }
    }

    if(result != EMPTY){
        return result;
    }
}