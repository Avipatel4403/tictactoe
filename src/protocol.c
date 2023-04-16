#include <assert.h>
#include <fcntl.h>
#include <glob.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/*
format the message being sent from server to client and vice versa 
into something that is readable for the server
Ex:
Play Avi ---> PLAY|3|Avi
*/

