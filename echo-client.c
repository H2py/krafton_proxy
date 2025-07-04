#include <stdio.h>
#include <stdlib.h>
#include "csapp.h"

int main(int argc, char **argv)
{
    rio_t rio;
    int clientfd;
    char *host, *port, buf[MAXLINE];

    if (argc != 3) {
        fprintf(stderr, "usage : %s <host> <port> \n", argv[0]);
        exit(0);
    }

    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);

    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        Rio_writen(clientfd, buf, strlen(buf));
        Rio_readlineb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);
    }
    Close(clientfd);
    exit(0);
}