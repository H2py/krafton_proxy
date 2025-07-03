#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>

// Write a program that converts its 16-bit network byte order to a 16-bit hex number and prints the result.
// linux > ./hex2dd 0x400 -> 1024

int main(int argc, char** argv)
{

    unsigned long hex;

    if (argc != 2) {
        printf("Usage : %s <hexadecimal value>\n", argv[0]);
        exit(1);
    }

    if (sscanf(argv[1], "%lx", &addr) != 1) {
        fprintf(stderr, "Error : misuage of input '%s'\n", argv[1]);
        exit(1);
    }

    printf("%lu\n", addr);

    exit(0);
}