#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080

int main() {
    int sockfd;
    struct sockaddr_in servaddr;
    fd_set readfds;
    char sendbuf[1024], recvbuf[1024];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("📡 서버에 연결됨. 메시지를 입력하세요 (Ctrl+D로 종료)\n");

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds); // 키보드 입력 감지
        FD_SET(sockfd, &readfds);       // 서버 응답 감지

        int maxfd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;

        int ready = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (ready < 0) {
            perror("select");
            exit(1);
        }

        // 키보드 입력 → 서버로 전송
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (fgets(sendbuf, sizeof(sendbuf), stdin) == NULL) {
                printf("❌ 입력 종료됨 (Ctrl+D)\n");
                break;
            }
            write(sockfd, sendbuf, strlen(sendbuf));
        }

        // 서버 메시지 → 화면 출력
        if (FD_ISSET(sockfd, &readfds)) {
            int n = read(sockfd, recvbuf, sizeof(recvbuf));
            if (n <= 0) {
                printf("❌ 서버가 연결을 종료했습니다.\n");
                break;
            }
            recvbuf[n] = '\0';
            printf("📨 서버 응답: %s", recvbuf);
        }
    }

    close(sockfd);
    return 0;
}
