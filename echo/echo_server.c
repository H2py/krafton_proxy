#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "csapp.h"

void echo(int connfd);

int main(int argc, char **argv) {
    int listenfd, connfd; // listenfd는 클라이언트의 연결을 받기 위해서 필요하며, connfd는 연결된 클라이언트와 데이터를 주고 받기 위해 필요하다
    socklen_t clientlen; // clientlen은 클라이언트 소켓 구조체의 길이를 나타낼 것이다
    struct sockaddr_storage clientaddr; // clientaddr은 클라이언트 주소 : IP address를 담는 구조체이다
    char client_hostname[MAXLINE], client_port[MAXLINE]; // client host name과 port를 받을 변수 선언

    if (argc != 2) {
        fprintf(stderr, "usage : %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = open_listenfd(argv[1]); // 지정된 포트에서 클라이언트 연결 요청을 기다리는 소켓 생성
    while (1) {
        clientlen = sizeof(struct sockaddr_storage); // 클라이언트 길이를 만든다
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // clientaddr에는 연결된 클라이언트의 주소 정보가 저장됨
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0); // 클라이언트 주소 및 호스트 이름을 사용하기 쉽게 만들어준다
        echo(connfd); // 메아리를 출력한다
        Close(connfd);
    }
    exit(0);
}

void echo(int connfd) {
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, connfd); // connfd와 rio를 연결한다
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) { // rio에서 읽은만큼, Rio_writen을 통해서, 클라이언트에게 보낸다
        printf("server received %d bytes \n", (int)n);
        Rio_writen(connfd, buf, n);
    }
}