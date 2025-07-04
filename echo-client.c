#include <stdio.h>
#include <stdlib.h>
#include "csapp.h"

// 어떤게 필요할까?
// 어떤 동작을 하는 걸 만들어야할까?
// 일단, 연결할 수 있는 서버를 찾아야한다 (open_clientfd는 클라이언트 소켓을 생성하고, 서버에 연결하는 함수이다)

// echo-client.c

int main(int argc, char **argv) {
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    if (argc != 3) {
        fprintf(stderr, "usage : ~~~ \n");
        exit(0);
    }
    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);

    while(Fgets(buf, MAXLINE, stdin) != NULL) {
        Rio_writen(clientfd, buf, strlen(buf));
        Rio_readlineb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);
    }

    Close(clientfd);
    exit(0);
}