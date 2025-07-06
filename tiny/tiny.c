/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"
// read & pares, 
void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}

/*
 * stat 구조체를 사용하는 이유 : 파일의 메타데이터를 저장하기 위해 사용한다 : 파일 크기 st_size, 파일 권한 : st_mode -> 파일의 존재 여부, 권한 등 확인
 * cgiargs(CGI에 전달할 인자들), http://example.com/cgi-bin/script?num1=5&num2=4에서 num1=5&num2=4 부분이 cgiargs에 저장됨
 * S_ISREG(sbuf.st_mode)는 일반 파일인지 확인 / S_IRUSR & sbuf.st_mode 소유자가 읽기 권한 가지는지 확인
 */
void doit(int fd)
{
  int is_static; // bool type처럼 사용할 int is_static
  struct stat sbuf; // sbuf를 stat 구조체로 선언한 이유는 무엇일까?
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE]; // 필요한 인자들 선언
  char filename[MAXLINE], cgiargs[MAXLINE]; // cgiargs란.. 무엇일까..?
  rio_t rio; // 내부 버퍼를 사용하는 rio 생성

  Rio_readinitb(&rio, fd); // rio와 fd를 연결한다
  Rio_readlineb(&rio, buf, MAXLINE); // MAXLINE만큼 한 줄을 읽어온다
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version); // Request의 첫 줄을 통해서, method, uri, version을 가져온다 GET / HTTP/1.0과 같은 형태
  if(strcasecmp(method, "GET")) { // strcasecmp는 대소문자를 구분하지 않는 문자열 비교함수이다
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  read_requesthdrs(&rio); // 헤더를 출력하고 버리는 용도

  is_static = parse_uri(uri, filename, cgiargs); // parsing을 통해서, is_static 여부를 확인
  if(stat(filename, &sbuf) < 0) {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }
  
  if (is_static) { // 이후, stat 구조체가 어떤 역할을 하는지, S_ISREG와 같은 플래그가 어떤 역할을 하는지 몰라서 막혔음
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
    }
    serve_static(fd, filename, sbuf.st_size);
  }
  else {
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs);
  }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXLINE];

  sprintf(body, "<html><title>Tiny Error</title>\n");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body); // 배경을 흰색으로 설정하고
  sprintf(body, "%s%s : %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s : %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web Server</em>\r\n", body); // 구분자를 넣고, Tiny Web Server를 강조구문으로 표시한다

  spirntf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body)); // 콘텐츠 길이에 대한 정보를 담는다
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, stlren(body));
}

/* 헤더 내용을 읽고 버리는 함수이다 -> 헤더를 사용하지 않더라도 이를 깨끗하게 만들기 위함이다
 * 착각하기 쉬운 내용으로는 buf가 완전히 "\r\n"이랑 똑같아야 끝난다는 것이다 
 * strcmpy는 문자열을 비교하며, 완전히 같다면 0, 사전순으로 보았을 때 먼저 시작하면 >0을 리턴함
 */
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while(strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  // text = "hello world"일 때, ststr(text, "world");는 text내에서 "특정 문자열이 처음으로 나오는 위치를 찾는다"
  if(!strstr(uri, "cgi-bin")) {
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri); 
    if (uri[strlen(uri) - 1] == '/')
      strcat(filename, "home.html");
    return 1;
  }
  else {
    ptr = index(uri, '?');
    if (ptr) {
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';
    }
    else
      strcpy(cgiargs, ""); // strcat(char *dest, const char *src) dest 뒤에다가 src
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}