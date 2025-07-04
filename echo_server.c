#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/wait.h>

#include "csapp.h"

#define PORT 8080
#define BUF_SIZE 1024

void echo(int connfd) {
    char buf[BUF_SIZE];
    int n;
    while ((n = read(connfd, buf, BUF_SIZE) > 0)) {
        write(connfd, buf, n);
    }
}

void reap_zombie(int sig) {
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    int listenfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen = sizeof(cliaddr);
    
    signal(SIGCHLD, reap_zombie);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY; 

    bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    listen(listenfd, 1024);

    printf("Server is running on... Wating for client in PORT %d...", PORT);

    while(1) {
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
    }

    if (fork() == 0) {
        close(listenfd);
        echo(connfd);
        close(connfd);
        exit(0);
    }
    close(connfd);
    
    return 0;

}