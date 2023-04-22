#include <string.h>

int protocol_name(char *buf, int length) 
{
    char code[5];
    strncpy(code, buf, 4);
    code[4] = '\0';

    if(strcmp(code, "PLAY") != 0) {
        return 0;
    }
    
}