#include <stdio.h>
#include "csapp.h"
#include "sbuf.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define NTHREADS. 4

static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

void doit(int fd);
int parse_uri(char *uri, char *hostname, char *port, char *path);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void read_requesthdrs(rio_t *rp, char *hostname, char *port, int serverfd);
void *thread(void *vargp);

int main(int argc, char *argv[]) {
  signal(SIGPIPE, SIG_IGN);
  pthread_t tid;
  int listenfd, *connfdp;
  char hostname[MAXLINE], port[MAXLINE];
  struct sockaddr_in clientaddr;
  socklen_t clientlen;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // Open_listenfd를 통해서, 클라이언트 측 요청을 받을 리스닝 소켓을 열어준다
    listenfd = Open_listenfd(argv[1]);

    while (1) {
        clientlen = sizeof(clientaddr);
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen); // 클라이언트와 연결된 connfd를 doit에 넘겨준다
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        Pthread_create(&tid, NULL, thread, connfdp); // 새로운 스레드를 생성하여, 클라이언트와 통신을 담당한다. 이때, 스레드가 실행할 함수는 thread();
    }

    return 0;
}


void doit(int clientfd) {
    int serverfd, n;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE], port[MAXLINE], path[MAXLINE];
    rio_t client_rio, server_rio;

    // 클라이언트와 연결된 clientfd에 Rio_readinitb를 연결시킴
    Rio_readinitb(&client_rio, clientfd);
    if (!Rio_readlineb(&client_rio, buf, MAXLINE)) return;
    printf("Request line: %s", buf);

    // sscanf는 buf에서, 띄어쓰기 문자열마다 나누어 method, uri, version을 기록함 GET / www.cmu.edu / HTTP/1.0(version)
    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET")) {
        clienterror(clientfd, method, "501", "Not Implemented", "Tiny does not implement this method");
        return;
    }

    parse_uri(uri, hostname, port, path); // parse_uri는 hostname / port / path 를 구분함, uri만 이용해서, 인자로 받은 hostname, port, path에 기록하면 됨

    serverfd = Open_clientfd(hostname, port); // hostname과 port를 이용해서 요청할 serverfd를 열어준다
    if (serverfd < 0) {
        clienterror(clientfd, method, "502", "Bad Gateway", "Failed to connect to end server");
        return;
    }

    Rio_readinitb(&server_rio, serverfd); // init을 사용하고

    snprintf(buf, MAXLINE, "%s %s HTTP/1.0\r\n", method, path); // snpirntf는, buf에 MAXLINE만큼 작성하는데, 세번째 인자가 buf에 들어가며, 그 다음 인자들은 %s에 들어갈 인자들임 
    Rio_writen(serverfd, buf, strlen(buf)); // serverfd에 요청을 보낸다

    read_requesthdrs(&client_rio, hostname, port, serverfd); // read_requesthdrs는 무엇을 하는 동작인거지?

    while ((n = Rio_readlineb(&server_rio, buf, MAXLINE)) > 0) { // server_rio로부터 읽어온 뒤, 0보다 크다면 Rio_wrtien을 clientfd에 buf만큼 출력함
        Rio_writen(clientfd, buf, n);
    }

    Close(serverfd);
}

int parse_uri(char *uri, char *hostname, char *port, char *path) {
    char *hostbegin, *hostend, *pathbegin, *portbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) return -1;
    hostbegin = uri + 7;
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin) {
        strcpy(path, pathbegin);
    } else {
        path[0] = '/';
        path[1] = '\0';
    }

    hostend = pathbegin ? pathbegin : hostbegin + strlen(hostbegin);
    portbegin = strchr(hostbegin, ':');

    if (portbegin && portbegin < hostend) {
        strncpy(hostname, hostbegin, portbegin - hostbegin);
        hostname[portbegin - hostbegin] = '\0';
        strncpy(port, portbegin + 1, hostend - portbegin - 1);
        port[hostend - portbegin - 1] = '\0';
    } else {
        strncpy(hostname, hostbegin, hostend - hostbegin);
        hostname[hostend - hostbegin] = '\0';
        strcpy(port, "80");
    }

    return 0;
}

void *thread(void *vargp)
{
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self()); // 스레드를 detach 상태로 만든다, 종료 시, 자동으로 리소스가 회수된다 -> 메모리 누수를 방지함
    Free(vargp);
    doit(connfd);
    Close(connfd);
    return NULL;
}


void read_requesthdrs(rio_t *rp, char *hostname, char *port, int serverfd) {
    char buf[MAXLINE], line[MAXLINE];
    int host = 0, conn = 0, proxy = 0, agent = 0;

    while (Rio_readlineb(rp, buf, MAXLINE) > 0 && strcmp(buf, "\r\n")) {
        if (!strncasecmp(buf, "Host:", 5)) host = 1;
        else if (!strncasecmp(buf, "Connection:", 11)) {
            snprintf(line, MAXLINE, "Connection: close\r\n");
            Rio_writen(serverfd, line, strlen(line));
            conn = 1;
            continue;
        } else if (!strncasecmp(buf, "Proxy-Connection:", 17)) {
            snprintf(line, MAXLINE, "Proxy-Connection: close\r\n");
            Rio_writen(serverfd, line, strlen(line));
            proxy = 1;
            continue;
        } else if (!strncasecmp(buf, "User-Agent:", 11)) {
            snprintf(line, MAXLINE, "%s", user_agent_hdr);
            Rio_writen(serverfd, line, strlen(line));
            agent = 1;
            continue;
        }
        Rio_writen(serverfd, buf, strlen(buf));
    }

    if (!host) {
        snprintf(line, MAXLINE, "Host: %s:%s\r\n", hostname, port);
        Rio_writen(serverfd, line, strlen(line));
    }
    if (!conn) {
        snprintf(line, MAXLINE, "Connection: close\r\n");
        Rio_writen(serverfd, line, strlen(line));
    }
    if (!proxy) {
        snprintf(line, MAXLINE, "Proxy-Connection: close\r\n");
        Rio_writen(serverfd, line, strlen(line));
    }
    if (!agent) {
        snprintf(line, MAXLINE, "%s", user_agent_hdr);
        Rio_writen(serverfd, line, strlen(line));
    }

    snprintf(line, MAXLINE, "\r\n");
    Rio_writen(serverfd, line, strlen(line));
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXBUF];

    snprintf(body, MAXBUF,
             "<html><title>Tiny Error</title>"
             "<body bgcolor=ffffff>\r\n"
             "%s: %s\r\n"
             "<p>%s: %s\r\n"
             "<hr><em>The Tiny Web server</em>\r\n</body></html>\r\n",
             errnum, shortmsg, longmsg, cause);

    snprintf(buf, MAXLINE, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    snprintf(buf, MAXLINE, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    snprintf(buf, MAXLINE, "Content-length: %lu\r\n\r\n", strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}