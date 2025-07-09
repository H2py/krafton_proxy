#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CLIENTS 100

void *client_handler(void *arg);

int main()
{
    int listen_fd, conn_fd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("listen");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    if((bind(listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr))) < 0)
    {
        perror("bind");
        exit(1);
    }

    if((listen(listen_fd, 10)) < 0)
    {
        perror("listen");
        exit(1);
    }

    printf("Now server is running Port : %d", PORT);

    while(1) {
        clilen = sizeof(cliaddr);
        conn_fd = accept(listen_fd, (struct sockaddr *)&cliaddr, &clilen);
        if (conn_fd < 0) {
            perror("accept");
            continue;
        }

        printf("Client connected from %s:%d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

        int *pclient = malloc(sizeof(int));
        *pclient = conn_fd;

        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, pclient);
        pthread_detach(tid);  // 쓰레드 종료 후 자동 정리
    }

    close(listen_fd);
    return 0;
}


void *client_handler(void *arg)
{
    int client_fd = *((int *)arg);
    free(arg);

    char *message = "Hello from server\n";
    write(client_fd, message, strlen(message));

    printf("💬 클라이언트(fd=%d)에게 메시지 전송 완료\n", client_fd);
    close(client_fd);
    return NULL;
}