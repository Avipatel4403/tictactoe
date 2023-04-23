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

//check board
char* protocol(char* message);
int checkBoard(char** board);
int checkMove();

pthread_mutex_t queueLock;
int curPlayers;
Client *clientList[2];

void *play_game(void *arg) 
{   
    Game *game = (Game *) arg;

    char buf[BUFSIZE];
    int bytes;

    Client *playerOne = game->one;
    Client *playerTwo = game->two;

    Client* playerTurn = playerOne;
    char result;

    printf("Game running!\n");



    //request
    //0 = none
    //1 = resign
    //2 = draw 
    int request = 0;

    while(result != 'X' || result != 'O'){
        //check request
        if(request == 1){

        }
        else if(request == 2){

        }

        bytes = read(playerOne->con->fd, buf, BUFSIZE);
        protocol_name(buf,BUFSIZE);
        printf("Read %s from Player One\n", buf);

        if(checkBoard(game->board == 'X' || checkBoard(game->board == 'O'))){
            break;
        }

        // make move, resign, or ask draw

        // check request
         bytes = read(playerTwo->con->fd, buf, BUFSIZE);
        protocol_name(buf, BUFSIZE);
        printf("Read %s from Player Two\n", buf);
        checkBoard(game->board);
        if (checkBoard(game->board == 'X' || checkBoard(game->board == 'O'))) {
            break;
        }


    }









    close(playerOne->con->fd);
    close(playerTwo->con->fd);
    free(playerOne->NAME);
    free(playerTwo->NAME);
    free(playerOne->con);
    free(playerTwo->con);
    free(playerOne);
    free(playerTwo);
    free(game);
    return NULL;
}

void *create_client(void *arg) 
{
    pthread_t tid;
    int error;

    char buffer[BUFSIZE];
    int bytes;

    ConnectionData *con = arg;

    bytes = read(con->fd, buffer, BUFSIZE);
    if(!protocol_name(&buffer[0], bytes)) {
        close(con->fd);
        free(con);
        return NULL;
    }
    
    //check if read message is a valid name/protocol
    printf("New Player: %s\n", buffer);

    // char* str = "WAIT|0|";
    // write(con->fd, str, 8);

    pthread_mutex_lock(&queueLock);

    //Create new client
    Client *client = malloc(sizeof(Client*));
    client->con = con;
    client->NAME = malloc(strlen(buffer) + 1);
    strcpy(client->NAME, buffer);

    //Add client to list
    clientList[curPlayers] = client;
    curPlayers++;
    printf("Cur Players: %d\n", curPlayers);

    if (curPlayers == 2) {

        curPlayers = 0;

        error = pthread_sigmask(SIG_BLOCK, &mask, NULL);
        if (error != 0) {
            fprintf(stderr, "sigmask: %s\n", strerror(error));
            exit(EXIT_FAILURE);
        }

        Game *game = (Game *) malloc(sizeof(Game*));
        //Could maybe add checker here if malloc worked?
        game->one = clientList[0];
        game->two = clientList[1];

        error = pthread_create(&tid, NULL, play_game, (void*) game);
        if (error != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(error));
            close(clientList[0]->con->fd);
            close(clientList[1]->con->fd);
            free(clientList[0]);
            free(clientList[1]);
            pthread_mutex_unlock(&queueLock);
            return NULL;
        }

        // automatically clean up child threads once they terminate
        pthread_detach(tid);

        // unblock handled signals
        error = pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
        if (error != 0) {
            fprintf(stderr, "sigmask: %s\n", strerror(error));
            exit(EXIT_FAILURE);
        }
    }
    pthread_mutex_unlock(&queueLock);
    printf("Done creating Client for %s", buffer);
    return NULL;
}

int main(int argc, char **argv) 
{
    ConnectionData *con;
    int error;
    pthread_t tid;

    install_handlers(&mask);

    char* service = argc == 2 ? argv[1] : "15000";

    int listener = open_listener(service, QUEUE_SIZE);
    if (listener < 0) exit(EXIT_FAILURE);

    printf("Listening for incoming connections on %s\n", service);

    while (active) {

        con = (ConnectionData*)malloc(sizeof(ConnectionData));
        con->addr_len = sizeof(struct sockaddr_storage);
        con->fd = accept(listener,
                         (struct sockaddr*)&con->addr,
                         &con->addr_len);
        if (con->fd < 0) 
        {
            perror("accept");
            free(con);
            // TODO check for specific error conditions
            continue;
        }

        pthread_create(&tid, NULL, create_client, (void *) con);
        pthread_detach(tid);
        
    }

    puts("Shutting down");
    close(listener);

    // returning from main() (or calling exit()) immediately terminates all
    // remaining threads

    // to allow threads to run to completion, we can terminate the primary thread
    // without calling exit() or returning from main:
    //   pthread_exit(NULL);
    // child threads will terminate once they check the value of active, but
    // there is a risk that read() will block indefinitely, preventing the
    // thread (and process) from terminating

    // to get a timely shut-down of all threads, including those blocked by
    // read(), we will could maintain a global list of all active thread IDs
    // and use pthread_cancel() or pthread_kill() to wake each one

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

int isValidMove(char** board,int row, int column)
{
    if(board[row][column] != EMPTY){
        return 1;
    }
    return 0;
}

int checkBoard(char** board)
{
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
