#include <stdio.h>

#include "tree.h"

int main(int argc, char** argv) {
    int counter = 1;

    if (argc == 1) listdir(".");

    char* seen;

    while (++counter <= argc) {
        printf("\nListing %s...\n", argv[counter - 1]);
        listdir(argv[counter - 1]);
    }

    return 0;
}
