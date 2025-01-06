#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "vectorparse.h"


int main() {
    //char input[] = "[ (1 1) \"example\" +456 -789 \"hello world\" 1ab]";
    char input[] = "(1 1";
    char output[200] = "";
    char *next = input;

    // Extract vector
    next = extract_vector(next, output);

    if (output[0] != '\0') {
        printf("Vector: %s\n", output);
    } else {
        printf("No vector found or error in vector extraction.\n");
    }

    return 0;
}

