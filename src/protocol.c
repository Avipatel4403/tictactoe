#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int protocol_name(char *buf, int length, char **name) 
{
    char code[6];
    memcpy(code, buf, 5);
    code[5] = '\0';
    printf("Code: %s\n", code);
    if(strcmp(code, "PLAY|") != 0) {
        return 0;
    }

    //Find location of the 2nd bar
    int i = 5;
    while(buf[i] != '|' && i < length) {
        i++;
    }
    printf("i: %d\n", i);

    //2nd bar not found and has reached the end of the buffer
    if(i == length) {
        return 0;
    }

    //Copy the substring into numString
    int sizeOfLength = i - 4;
    char numString[sizeOfLength];
    memcpy(numString, buf + 5, sizeOfLength - 1);
    numString[sizeOfLength - 1] = '\0';
    printf("numString: %s\n", numString);

    //Convert numString into an int
    int nameLength = atoi(numString);
    printf("nameLength: %d\n", nameLength);

    //3rd bar is not at the end based on length field or length of the buffer
    if(buf[i + nameLength] != '|' || buf[length - 1] != '|') {
        return 0;
    }

    //Name is too long
    if(nameLength - 1 > 20) {
        return 1;
    }

    *name = malloc(nameLength);
    memcpy(*name, &buf[i + 1], nameLength - 1);
    *name[nameLength - 1] = '\0';
    
    return 2;
}