// Compile:  gcc -pthread -Wall -Wextra ttts.c -o ttts

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "protocol.h"

//Defintions
#define QUEUE_SIZE 8
#define EMPTY 0 //Empty Space on the board
#define X 1 //Game Status: X is the winner
#define O 2 //Games Status: O is the winner
#define DRAW 3 //Self Explanatory
#define BUFSIZE 256

// data to be sent to worker threads
typedef struct ConnectionData {
    struct sockaddr_storage addr;
    socklen_t addr_len;
    int fd;
} ConnectionData;

typedef struct Client {
    ConnectionData *con; //Players Connection
    char PIECE; // NULL X or O
    char *NAME; //players name
    struct Client *opp;
} Client;

typedef struct Game {
    Client *one;
    Client *two;
    char board[3][3];
    int status;
} Game;

//Global
sigset_t mask;
int open_listener(char* service, int queue_size);

volatile int active = 1;

void handler(int signum) {
    active = 0;
}




// set up signal handlers for primary thread
// return a mask blocking those signals for worker threads
// FIXME should check whether any of these actually succeeded
void install_handlers(sigset_t* mask) {
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

//CHECKS
char** checkDUPLICATES;
char* protocol(char* message);
char checkBoard(char board[3][3]);
int makeMove(char board[3][3], int row, int column, char piece);

pthread_mutex_t queueLock;
int curPlayers;
Client *clientList[2];
char* protocol(char* message);
char checkBoard(char board[3][3]);
int makeMove(char board[3][3], int row, int column, char piece);

int drawHandle(Game* game,Client* currentPlayer){
    int bytes;
    char buf[BUFSIZE];
    write(currentPlayer->opp->con->fd,"DRAW|2|S|",9); 
    while(1){
        
        if((bytes = read(currentPlayer->opp->con->fd, buf,BUFSIZE)) <= 0){
            return 1;
        }
        if(protocol_check(buf,bytes) == 1){
            write(currentPlayer->opp->con->fd, "INVL|23|Invalid Message Format|", 32);
            continue;
        }
        if (strncmp(buf,"DRAW|",5)) {
            if (buf[7] == 'A') {

                return 1;
            } 
            else if (buf[7] == 'R') {
                return 0;
            } else {
                write(currentPlayer->opp->con->fd, "INVL|23|Invalid Message Format|", 32);
                continue;
            }

        } else {
            if (strncmp(buf, "RSGN|", 5)){
                return 1;
            }
            write(currentPlayer->opp->con->fd, "INVL|23|Invalid Message Format|", 32);

            if ((bytes = read(currentPlayer->opp->con->fd,buf,BUFSIZE )) <= 0) {
                return 1;
            }
            if (protocol_check(buf, bytes) == 1) {
                write(currentPlayer->opp->con->fd, "INVL|23|Invalid Message Format|", 32);
                continue;
            }

        }
    }
}

void CloseGAME(Game* game, Client* winner, int resigned) {
    //send Game
    //get protocol
    char* win;
    char* loser;


    //PLAY ONE WON
    if(winner == game->one){
        write(game->one->con->fd,win,strlen(win));
        write(game->two->con->fd, loser, strlen(loser));
    }
    else if(winner == game->two){
        write(game->one->con->fd, loser, strlen(loser));
        write(game->two->con->fd, win, strlen(win));
    }
    else{
        write(game->one->con->fd, closing, strlen(closing));
        write(game->two->con->fd, closing, strlen(closing));
    }



    // Close file descriptors
    close(game->one->con->fd);
    close(game->two->con->fd);
    // Free dynamically allocated memory
    free(game->one->NAME);
    free(game->two->NAME);
    free(game->one->con);
    free(game->two->con);
    free(game->one);
    free(game->two);
    free(game);
    printf("Game ended\n");


}
//     // Free dynamically allocated memory
//     free(playerOne->NAME);
//     free(playerTwo->NAME);
//     free(playerOne->con);
//     free(playerTwo->con);
//     free(playerOne);
//     free(playerTwo);
//     free(game);
//     printf("Game ended\n");
// }

void *play_game(void *arg) 
{   
    Game *game = (Game *) arg;
    char buf[BUFSIZE];
    int bytes;
    Client *playerOne = game->one;
    Client *playerTwo = game->two;
    playerOne->PIECE = 'X';
    playerOne->opp = playerTwo;
    playerTwo->PIECE = 'O';
    playerTwo->opp = playerOne;


    //player one goes first
    Client* playerTurn = playerOne;

    printf("Game running!\n");

    //Sends Begin
    char *begin;
    begin = NULL;
    begin = protocol_create_begin(playerOne->NAME, &playerOne->PIECE);
    write(playerOne->con->fd, begin, strlen(begin));
    begin = protocol_create_begin(playerTwo->NAME, &playerTwo->PIECE);
    write(playerTwo->con->fd, begin, strlen(begin));
    free(begin);

    //Game Piece who won
    char result;
    Client* winner;
    int gameEnded = 0;
    int turnComplete = 0;
    
    //Player moves
    while(!gameEnded){

        //
        if((bytes = read(playerTurn->con->fd, buf, BUFSIZE)) <= 0){
            gameEnded = 1;
            CloseGAME(game,playerTurn->opp,1);
            continue;
        }

        //someway to check 
        printf("Read %s from Player %c \n", buf,playerTurn->PIECE);

        //Grab Message
        if(protocol_check(buf,bytes) == 1){
            gameEnded = 1;
            CloseGAME(game, playerTurn->opp, 1);
            continue;
        }

        if(strncmp(buf,"DRAW",4) == 0){
            if(buf[7] == 'S'){
                    
                if(drawHandle(game,playerTurn) == 1){
                    gameEnded = 1;
                    CloseGAME(game,winner,0);
                    continue;
                }
                write(playerTurn->con->fd,"DRAW|2|R|",10);
                continue;
            
            }
            else{
                write(playerTurn->con->fd,"INVL|18|No Draw Initiated|",27);
                continue;
            }
        }
        else if(strncmp(buf,"RSGN",4) == 0){
            gameEnded = 1;
            if(playerTurn == playerOne){
                winner = playerTwo;
            }
            else{
                winner = playerOne;
            }
            continue;
        }
        else if(strncmp(buf,"MOVE",4) == 0){
            if(makeMove(game->board,buf[9] - 48,buf[11] - 48,playerTurn->PIECE) != 1){
                write(playerTurn->con->fd, "INVL | 24 | That space is occupied.|", 37);
                continue;
            }
            result = checkBoard(game->board);
            if (result != DRAW || result != EMPTY) {
                gameEnded = 1;
            }
        }
        //INVALID COMMAND
        else{
            write(playerTurn->con->fd, "INVL|14|Wrong Command",22);
            continue;
        }
    }

    // //change player
    // if(playerTurn == playerOne){
    //     playerTurn = playerTwo;
    // }
    // else{
    //     playerTurn = playerOne;
    // }

    // }

    //game
    CloseGAME(game,winner,1);


    return NULL;
}

void *create_client(void *arg) 
{
    pthread_t tid;
    int error;

    char buffer[BUFSIZE];
    char *name;
    int bytes_read;
    char* response;

    ConnectionData *con = arg;

    while (1) {
        bytes_read = read(con->fd, buffer, BUFSIZE);
        if (bytes_read <= 0) {
            goto close_socket;
        }
        printf("Read: %s\n", buffer);
        int error = protocol_name(&buffer[0], bytes_read, &name);
        if (error == 0) {
            break;
        } else if (error == 1) {
            response = "INVL|34|Player name exceeds 20 characters|";
            write(con->fd, response, strlen(response));
        } else if (error == 2) {
            response = "INVL|23|Invalid Message Format|";
            write(con->fd, response, strlen(response));
            goto close_socket;
        }
    }

    char *wait = "WAIT|0|";
    write(con->fd, wait, strlen(wait));

    pthread_mutex_lock(&queueLock);

    //Create new client
    Client *client = malloc(sizeof(Client));
    client->con = con;
    client->NAME = name;

    if (curPlayers == 0) {
        //If there are no players waiting, add the client to the list
        clientList[curPlayers++] = client;
        printf("Player %s is waiting for an opponent\n", client->NAME);
    } else {

        // If there is another player waiting, create a new game
        Client *other = clientList[0];
        curPlayers = 0;

        pthread_sigmask(SIG_BLOCK, &mask, NULL);

        Game *game = malloc(sizeof(Game));  
        game->one = client;
        game->two = other;

        //Create and run thread for playing the game
        pthread_create(&tid, NULL, play_game, (void*) game);

        //Automatically clean up child threads once they terminate
        pthread_detach(tid);

        //Unblock handled signals
        pthread_sigmask(SIG_UNBLOCK, &mask, NULL);

        printf("Game started between %s and %s\n", client->NAME, other->NAME);
    }

    pthread_mutex_unlock(&queueLock);
    printf("Done creating Client for %s\n", client->NAME);
    return NULL;

    close_socket:
    close(con->fd);
    free(con);
    return NULL;
}

int main(int argc, char **argv) 
{
    ConnectionData *con;
    int error;
    pthread_t tid;
    char* service = argc == 2 ? argv[1] : "15000";

    install_handlers(&mask);
    int listener = open_listener(service, QUEUE_SIZE);
    if (listener < 0) exit(EXIT_FAILURE);

    printf("Listening for incoming connections on %s\n", service);

    while (active) {
        //Accept a new connection
        con = (ConnectionData*) malloc(sizeof(ConnectionData));
        con->addr_len = sizeof(struct sockaddr_storage);
        con->fd = accept(listener,(struct sockaddr*)&con->addr,&con->addr_len);

        //Check for errors in connection acceptance
        if (con->fd < 0) 
        {
            perror("accept");
            free(con);
            continue;
        }

        //Create a new thread to handle the connection
        pthread_create(&tid, NULL, create_client, (void *) con);
        pthread_detach(tid);
    }

    puts("Shutting down");
    close(listener);

    return EXIT_SUCCESS;
}

int open_listener(char* service, int queue_size) 
{
    struct addrinfo hint, *info_list, *info;
    int error, sock;

    // initialize hints
    memset(&hint, 0, sizeof(struct addrinfo));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_PASSIVE;

    // obtain information for listening socket
    error = getaddrinfo(NULL, service, &hint, &info_list);
    if (error) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        return -1;
    }

    // attempt to create socket
    for (info = info_list; info != NULL; info = info->ai_next) {
        sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

        // if we could not create the socket, try the next method
        if (sock == -1) continue;

        // bind socket to requested port
        error = bind(sock, info->ai_addr, info->ai_addrlen);
        if (error) {
            close(sock);
            continue;
        }

        // enable listening for incoming connection requests
        error = listen(sock, queue_size);
        if (error) {
            close(sock);
            continue;
        }

        // if we got this far, we have opened the socket
        break;
    }

    freeaddrinfo(info_list);

    // info will be NULL if no method succeeded
    if (info == NULL) {
        fprintf(stderr, "Could not bind\n");
        return -1;
    }

    return sock;
}

int makeMove(char board[3][3],int row, int column,char piece) {
    if(board[row][column] == EMPTY){
        board[row][column] = piece;
        return 1;
    }
    return 0;
}

char checkBoard(char board[3][3]) {
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
