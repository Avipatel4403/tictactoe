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
#define DRAW 3

//this is needed to handle signals
volatile int active = 1;

void handle(int signum) {
    active = 0;
}

//check board


char* protocol(char* message);



int checkBoard(char** board);
int checkMove();

int main(int argc, char **argv) {

    //signals 
    struct sigaction act;
    struct sigaction old;
   
   
    //need to create a single socket for server
    if(argc < 2){
        perror("missing port or socket");
    }

    //server socket
    struct sockaddr_storage server;

    socklen_t servlen = sizeof(server);

    return EXIT_SUCCESS;
}
int checkMove(char** board,int row, int column){
    if(board[row][column] != EMPTY){
        return 1;
    }
    return 0;
}

int checkBoard(char** board){
    //check rows 
    for(int i = 0;i < 3;i++){
        if (board[i][0] == board[i][1] && board[i][1] == board[i][2]) {
            return board[i][0];
        }
    }

    //check diagonal
    if (board[0][0] == board[1][1] && board[1][1] == board[2][2]) {
        return board[0][0];
    } 
    else if (board[0][2] == board[1][1] && board[1][1] == board[2][0]) {
        return board[0][2];
    }

    //check columns
    for (int i = 0; i < 3; i++) {
        if (board[0][i] == board[1][i] && board[0][i] == board[i][2]) {
            return board[0][i];
        }
    }
    //check if there is no empty space
    for (int i = 0; i < 3; i++) {
        for(int j = 0; j < 3;j++){
            if (board[i][j] == EMPTY) {
                return EMPTY;
            }
        }
    }
    return DRAW;
}
