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
#define _POSIX_C_SOURCE 200809L
#include <unistd.h>

#define QUEUE_SIZE 8

//Defintions
#define EMPTY 0 //Empty Space on the board
#define X 1 //Game Status: X is the winner
#define O 2 //Games Status: O is the winner
#define DRAW 3 //Self Explanatory

//Global
sigset_t mask;

typedef struct Client{
    int SOCK; //Players Connection
    char PIECE; // NULL X or O
    char *NAME; //players name
}Client;

typedef struct Game{
    Client *one;
    Client *two;
    char board[3][3];
    int status;
}Game;

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

// data to be sent to worker threads
struct connection_data {
    struct sockaddr_storage addr;
    socklen_t addr_len;
    int fd;
};

#define BUFSIZE 256
#define HOSTSIZE 100
#define PORTSIZE 10
void* read_data(void* arg) {
    struct connection_data* con = arg;
    char buf[BUFSIZE + 1], host[HOSTSIZE], port[PORTSIZE];
    int bytes, error;

    error = getnameinfo(
        (struct sockaddr*)&con->addr, con->addr_len,
        host, HOSTSIZE,
        port, PORTSIZE,
        NI_NUMERICSERV);
    if (error) {
        fprintf(stderr, "getnameinfo: %s\n", gai_strerror(error));
        strcpy(host, "??");
        strcpy(port, "??");
    }

    printf("Connection from %s:%s\n", host, port);

    while (active && (bytes = read(con->fd, buf, BUFSIZE)) > 0) {
        buf[bytes] = '\0';
        printf("[%s:%s] read %d bytes |%s|\n", host, port, bytes, buf);
    }

    if (bytes == 0) {
        printf("[%s:%s] got EOF\n", host, port);
    } else if (bytes == -1) {
        printf("[%s:%s] terminating: %s\n", host, port, strerror(errno));
    } else {
        printf("[%s:%s] terminating\n", host, port);
    }

    close(con->fd);
    free(con);

    return NULL;
}

//check board
char* protocol(char* message);
int checkBoard(char** board);
int checkMove();

pthread_mutex_t queueLock;
int curPlayers;
Client *clientList[2];

void play_game(Game *game) 
{
    char buf[BUFSIZE + 1];
    int bytes;

    Client *playerOne = game->one;
    Client *playerTwo = game->two;

    printf("Game running!");
    bytes = read(playerOne->SOCK, buf, BUFSIZE);
    printf("Read %s bytes from Player One", bytes);
    bytes = read(playerTwo->SOCK, buf, BUFSIZE);
    printf("Read %s bytes from Player Two", bytes);

    close(playerOne->SOCK);
    close(playerTwo->SOCK);
    free(playerOne);
    free(playerTwo);
    free(game);
}

void create_client(struct connection_data* con) 
{
    int tid;
    int error;

    char buf[BUFSIZE + 1];
    int bytes;
    while (active && (bytes = read(con->fd, buf, BUFSIZE)) > 0) {
        buf[bytes] = '\0';
    }
    
    //check if read message is a valid name/protocol

    char* str = "WAIT|0|";
    write(con->fd, str, 8);

    pthread_mutex_lock(&queueLock);

    //Create new client
    Client *client = malloc(sizeof(Client*));
    client->SOCK = con->fd;
    //need to read in client name somehow
    client->NAME = buf[0];

    //Add client to list
    clientList[curPlayers] = client;
    curPlayers++;

    if (curPlayers == 2) {

        error = pthread_sigmask(SIG_BLOCK, &mask, NULL);
        if (error != 0) {
            fprintf(stderr, "sigmask: %s\n", strerror(error));
            exit(EXIT_FAILURE);
        }

        Game *game = (Game *) malloc(sizeof(Game*));
        //Could maybe add checker here if malloc worked?
        game->one = clientList[0];
        game->two = clientList[1];

        error = pthread_create(&tid, NULL, play_game, game);
        if (error != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(error));
            close(clientList[0]->SOCK);
            close(clientList[1]->SOCK);
            free(clientList[0]);
            free(clientList[1]);
            pthread_mutex_unlock(&queueLock);
            return;
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
}

int main(int argc, char **argv) 
{
    struct connection_data* con;
    int error;
    pthread_t tid;

    install_handlers(&mask);

    char* service = argc == 2 ? argv[1] : "15000";

    int listener = open_listener(service, QUEUE_SIZE);
    if (listener < 0) exit(EXIT_FAILURE);

    printf("Listening for incoming connections on %s\n", service);

    while (active) {

        con = (struct connection_data*)malloc(sizeof(struct connection_data));
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

        pthread_create(&tid, NULL, create_client, con);
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

int open_listener(char* service, int queue_size) {
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
int isValidMove(char** board,int row, int column){
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
