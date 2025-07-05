#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>

#define MAXLINE 8192
#define LISTENQ 1024

void handle_request(int connfd);
void serve_cgi(int connfd, char *filename, char *cgiargs);
void parse_uri(char *uri, char *filename, char *cgiargs);
int is_cgi(char *filename);

int main(int argc, char **argv) {
    int listenfd, connfd;
    struct sockaddr_in clientaddr;
    socklen_t clientlen;
    int port = 8000;
    
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    // Create socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket");
        exit(1);
    }
    
    // Allow socket reuse
    int optval = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    
    // Bind socket
    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(port);
    
    if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("bind");
        exit(1);
    }
    
    // Listen for connections
    if (listen(listenfd, LISTENQ) < 0) {
        perror("listen");
        exit(1);
    }
    
    printf("Server listening on port %d\n", port);
    
    // Handle zombie processes
    signal(SIGCHLD, SIG_IGN);
    
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
        if (connfd < 0) {
            perror("accept");
            continue;
        }
        
        if (fork() == 0) {
            close(listenfd);
            handle_request(connfd);
            close(connfd);
            exit(0);
        }
        close(connfd);
    }
    
    return 0;
}

void handle_request(int connfd) {
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    
    // Read request line
    if (recv(connfd, buf, MAXLINE, 0) <= 0) {
        return;
    }
    
    sscanf(buf, "%s %s %s", method, uri, version);
    
    if (strcmp(method, "GET") != 0) {
        char *response = "HTTP/1.0 501 Not Implemented\r\n\r\n";
        send(connfd, response, strlen(response), 0);
        return;
    }
    
    parse_uri(uri, filename, cgiargs);
    
    if (is_cgi(filename)) {
        serve_cgi(connfd, filename, cgiargs);
    } else {
        char *response = "HTTP/1.0 404 Not Found\r\n\r\n";
        send(connfd, response, strlen(response), 0);
    }
}

void parse_uri(char *uri, char *filename, char *cgiargs) {
    char *ptr;
    
    if (strstr(uri, "cgi-bin")) {
        ptr = index(uri, '?');
        if (ptr) {
            strcpy(cgiargs, ptr + 1);
            *ptr = '\0';
        } else {
            strcpy(cgiargs, "");
        }
        strcpy(filename, ".");
        strcat(filename, uri);
    } else {
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
    }
}

int is_cgi(char *filename) {
    return strstr(filename, "cgi-bin") != NULL;
}

void serve_cgi(int connfd, char *filename, char *cgiargs) {
    char buf[MAXLINE];
    int pid;
    
    // Send HTTP response header
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    send(connfd, buf, strlen(buf), 0);
    sprintf(buf, "Server: Simple CGI Server\r\n");
    send(connfd, buf, strlen(buf), 0);
    
    if ((pid = fork()) == 0) {
        // Child process
        
        // Set environment variables
        setenv("QUERY_STRING", cgiargs, 1);
        setenv("REQUEST_METHOD", "GET", 1);
        setenv("SERVER_PORT", "8000", 1);
        
        // Redirect stdout to client
        dup2(connfd, STDOUT_FILENO);
        
        // Execute CGI program
        execl(filename, filename, (char *)0);
        
        // If execl fails
        perror("execl");
        exit(1);
    }
    
    // Parent process waits for child
    waitpid(pid, NULL, 0);
}