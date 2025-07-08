#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CLIENTS FD_SETSIZE

int main() {
    int listen_fd, conn_fd, sockfd;
    int maxfd, maxi = -1;
    int client[MAX_CLIENTS];
    fd_set allset, rset;

    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;
    char buf[1024];
    ssize_t n;

    // 🛠️ 소켓 생성
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        exit(1);
    }

    // 🛠️ 주소 설정
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    // 🛠️ 바인드
    if (bind(listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        exit(1);
    }

    // 🛠️ 리슨
    if (listen(listen_fd, 10) < 0) {
        perror("listen");
        exit(1);
    }

    printf("📡 Broadcasting server is running (Port %d)\n", PORT);

    // 초기화
    maxfd = listen_fd;
    for (int i = 0; i < MAX_CLIENTS; i++)
        client[i] = -1;

    FD_ZERO(&allset);
    FD_SET(listen_fd, &allset);

    // 📡 메인 루프
    while (1) {
        rset = allset;

        int ready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (ready < 0) {
            perror("select");
            exit(1);
        }

        // 🧲 새 클라이언트 접속 감지
        if (FD_ISSET(listen_fd, &rset)) {
            clilen = sizeof(cliaddr);
            conn_fd = accept(listen_fd, (struct sockaddr *)&cliaddr, &clilen);
            if (conn_fd < 0) {
                perror("accept");
                continue;
            }

            printf("🔗 New connection: %s:%d\n",
                   inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

            int i;
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client[i] < 0) {
                    client[i] = conn_fd;
                    break;
                }
            }

            if (i == MAX_CLIENTS) {
                fprintf(stderr, "❌ Client count exceeded\n");
                close(conn_fd);
                continue;
            }

            FD_SET(conn_fd, &allset);
            if (conn_fd > maxfd) maxfd = conn_fd;
            if (i > maxi) maxi = i;

            if (--ready <= 0) continue;
        }

        // 💬 클라이언트 메시지 처리
        for (int i = 0; i <= maxi; i++) {
            sockfd = client[i];
            if (sockfd < 0) continue;

            if (FD_ISSET(sockfd, &rset)) {
                n = read(sockfd, buf, sizeof(buf));
                if (n <= 0) {
                    printf("❌ Client disconnected (fd=%d)\n", sockfd);
                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[i] = -1;
                } else {
                    // 브로드캐스트
                    for (int j = 0; j <= maxi; j++) {
                        int otherfd = client[j];
                        if (otherfd >= 0 && otherfd != sockfd) {
                            write(otherfd, buf, n);
                        }
                    }
                    printf("📨 Client[%d] → broadcast: %.*s\n", sockfd, (int)n, buf);
                }

                if (--ready <= 0) break;
            }
        }
    }

    return 0;
}
