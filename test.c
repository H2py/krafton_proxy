#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAXLINE 1024

// 클라이언트가 접속하면 연결마다 새로운 스레드 생성 -> 각 스레드는 에코를 수행함

pthread_once_t once_control = PTHREAD_ONCE_INIT;

void initialze_server() {
    printf("Server is created\n");
}

void *handle_client(void *arg) {
    int connfd = *((int *)arg);
    free(arg);
    pthread_detach(pthread_self());

    char buf[MAXLINE];
    ssize_t n;

    while((n = read(connfd, buf, MAXLINE)) > 0) {
        printf("message received from client is : %s", buf);
        write(connfd, buf, n); //echo
    }

    printf("Client is terminated (TID : %lu)\n", pthread_self());
    close(connfd);
    pthread_exit(NULL);
}

int main() {
    
}