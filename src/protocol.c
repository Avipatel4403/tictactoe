#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int protocol_check(char *buf, int length) {

    char code[6];
    memcpy(code, buf, 5);
    code[5] = '\0';

    if (strcmp(code, "MOVE|") == 0) {   
        if (length != 13) {
            return 1;
        }
        if (buf[7] != 'X' && buf[7] != 'O') {
            return 1;
        }
        if (buf[5] != '6' || buf[6] != '|' || buf[8] != '|' || buf[12] != '|' || buf[10] != ',') {
            return 1;
        }
        for (int i = 9; i <= 11; i += 2) {
            if (buf[i] < '1' || buf[i] > '3') {
                return 1;
            }
        }
        return 0;
    } 
    else if (strcmp(code, "DRAW|") == 0) {
        if (length != 9) {
            return 1;
        }
        if (buf[7] != 'S' && buf[7] != 'A' || buf[7] != 'R') {
            return 1;
        }
        if (buf[5] != '2' || buf[6] != '|' || buf[8] != '|') {
            return 1;
        }
        return 0;
    }
    else if (strcmp(code, "RSGN|") == 0) {
        if (length != 7) {
            return 1;
        }
        if (buf[5] != '0' || buf[6] != '|' || length != 7) {
            return 1;
        }
        return 0;
    } else { 
        return 1; 
    }
}

int protocol_name(char *buf, int length, char **name) 
{
     // Check if code is PLAY
    if (strncmp(buf, "PLAY|", 5) != 0) {
        return 2;
    }

    //Find location of the 2nd bar
    int i = 5;
    while(buf[i] != '|' && i < length) {
        i++;
    }

    //2nd bar not found and has reached the end of the buffer
    if(i == length) {
        return 2;
    }

    // Convert numString into an int
    int nameLength = atoi(&buf[5]);

    //3rd bar is not at the end based on length field or length of the buffer
    if(buf[i + nameLength] != '|' || buf[length - 1] != '|') {
        return 2;
    }

    //Name is too long
    if(nameLength - 1 > 20) {
        return 1;
    }

    *name = malloc(nameLength);
    memcpy(*name, &buf[i + 1], nameLength - 1);
    (*name)[nameLength - 1] = '\0';
    
    return 0;
}

char *protocol_create_begin(char *name, char *piece) {

    const char* prefix = "BEGN|";
    const char* delimiter = "|";
    char* formatted_string;
    int length;

    // Calculate the length of the formatted string
    length = strlen(prefix) + strlen(piece) + strlen(name) + strlen(delimiter) * 3 + 1;

    // Allocate memory for the formatted string
    formatted_string = (char*) malloc(length * sizeof(char));

    // Build the formatted string
    sprintf(formatted_string, "%s%d%s%s%s%s%s", prefix, (int) strlen(piece) + (int) strlen(name) + (int) strlen(delimiter) * 2, delimiter, piece, delimiter, name, delimiter);

    return formatted_string;
}