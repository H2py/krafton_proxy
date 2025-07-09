#include <stdio.h>
#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

void doit(int fd);
int parse_uri(char *uri, char *hostname, char *port, char *path);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void read_requesthdrs(rio_t *rp, char *hostname, char *port, int serverfd);

int main(int argc, char *argv[]) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  struct sockaddr_in clientaddr;
  socklen_t clientlen;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);

    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd);
        Close(connfd);
    }

    return 0;
}

void doit(int clientfd) {
    int serverfd, n;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE], port[MAXLINE], path[MAXLINE];
    char response_buf[MAX_OBJECT_SIZE];
    rio_t client_rio, server_rio;

    Rio_readinitb(&client_rio, clientfd);
    if (!Rio_readlineb(&client_rio, buf, MAXLINE)) return;
    printf("Request line: %s", buf);

    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET")) {
        clienterror(clientfd, method, "501", "Not Implemented", "Tiny does not implement this method");
        return;
    }

    parse_uri(uri, hostname, port, path);

    serverfd = Open_clientfd(hostname, port);
    if (serverfd < 0) {
        clienterror(clientfd, method, "502", "Bad Gateway", "Failed to connect to end server");
        return;
    }

    Rio_readinitb(&server_rio, serverfd);

    snprintf(buf, MAXLINE, "%s %s HTTP/1.0\r\n", method, path);
    Rio_writen(serverfd, buf, strlen(buf));

    read_requesthdrs(&client_rio, hostname, port, serverfd);

    while ((n = Rio_readlineb(&server_rio, buf, MAXLINE)) > 0) {
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
