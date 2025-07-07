#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080

void echo(int conn_fd);

int main() {
    int listen_fd, conn_fd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen = sizeof(cliaddr);

    char buf[1024];

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    if(bind(listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        exit(1);
    }

    if(listen(listen_fd, 1) < 0) {
        perror("listen");
        exit(1);
    }

    printf("서버 시작: 포트 %d\n", PORT);


    while(1) {
        fd_set readfds;
        struct timeval timeout;

        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(listen_fd, &readfds);

        int maxfd = listen_fd > STDIN_FILENO ? listen_fd : STDIN_FILENO;

        timeout.tv_sec = 10;
        timeout.tv_usec = 0;

        int ready = select(maxfd + 1, &readfds, NULL, NULL, &timeout);

        if (ready == -1) {
            perror("select");
            exit(1);
        } else if (ready == 0) {
            printf("There's any connection or input doesn't exist\n");
            exit(1);
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char buf[1024];
            fgets(buf, sizeof(buf), stdin);
            printf("입력한 내용: %s", buf);
        }

        if (FD_ISSET(listen_fd, &readfds)) {
            conn_fd = accept(listen_fd, (struct sockaddr *)&cliaddr, &clilen);
            if (conn_fd >= 0) {
                printf("클라이언트 연결 : %s:%d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
                echo(conn_fd);
                close(conn_fd);
            }   
        }
    }

    return 0;
}

void echo(int conn_fd)
{
    char buf[1024];
    size_t n;
    
    while((n = read(conn_fd, buf, sizeof(buf))) > 0) {
        printf("Server recieved %d bytes from Client", (int)n);
        write(conn_fd, buf, n);
    }
}