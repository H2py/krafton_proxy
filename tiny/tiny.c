/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"
#include <signal.h>

// read & pares, 
void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void sigchld_handler(int sig);

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  signal(SIGCHLD, sigchld_handler); // SIGCHLD 호출 시, sigchld_handler 실행

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
  struct stat sbuf; // 파일의 메타데이터를 담는 sbuf 생성
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE]; // 필요한 인자들 선언
  char filename[MAXLINE], cgiargs[MAXLINE]; // cgiargs : ?이후 만들어지는 쿼리스트링
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
  
  is_static = parse_uri(uri, filename, cgiargs); // parsing을 통해서, is_static 여부를 확인
  if(stat(filename, &sbuf) < 0) {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }
  
  read_requesthdrs(&rio); // 헤더를 출력하고 버리는 용도

  if (is_static) { // 이후, stat 정적 파일일 때
    // 정규 파일이 아니고, 유저가 다룰 수 없다면 에러를 내뱉음, 그렇지 않으면 serve_static 정적 파일을 클라이언트에게 전달한다
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

  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body)); // 콘텐츠 길이에 대한 정보를 담는다
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

/* 헤더 내용을 읽고 버리는 함수이다 -> 헤더를 사용하지 않더라도 이를 깨끗하게 만들기 위함이다
 * 착각하기 쉬운 내용으로는 buf가 완전히 "\r\n"이랑 똑같아야 끝난다는 것이다 
 * strcmpy는 문자열을 비교하며, 완전히 같다면 0, 사전순으로 보았을 때 먼저 시작하면 >0을 리턴함
 */
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  while(strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

// http://example.com/cgi-bin/adder?arg1=1&arg2=2와 같이 생긴 문자열
// cgi가 포함되어 있지 않다면, ./uri 형태로 만듦, 루트면 ./home.html cgi-bin이 있으면 쿼리 스트링 저장한다
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  if(!strstr(uri, "cgi-bin")) { // 정적콘텐츠이면, cgiargs의 인자 공백으로 만듦
    strcpy(cgiargs, ""); // 현재 디렉토리에 uri를 더한다
    strcpy(filename, ".");
    strcat(filename, uri);
    if(uri[strlen(uri) - 1] == '/')
      strcat(filename, "home.html");
    return 1;
  }
  else {
    ptr = index(uri, '?'); // 포인터가 NULL이 아니면, cgiargs에, arg1=1&arg2=2를 저장한다, ?를 NULL처리 한다
    if (ptr) {
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';
    }
    else 
      strcpy(cgiargs, ""); // 없으면 공백을 담는다

    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

// 동영상 같은 큰 파일은 필요한 부분만 조금씩 다운로드해서 재생, 이떄 Range header를 포함하여 욫어한다(partial request)
void serve_static(int fd, char *filename, int filesize) // 클라이언트와 연결되어 있는 파일 디스크립터, 전송할 파일 경로 및 크기 인자로 받음
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  get_filetype(filename, filetype);
  // sprintf(buf, "HTTP/1.0 200 OK \r\n");
  // sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  // sprintf(buf, "%sConnection: close\r\n", buf);
  // sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  // sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  
  if (strstr(filetype, "video/")) {
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sAccept-Ranges: bytes\r\n", buf);  // Range 지원 명시
    sprintf(buf, "%sContent-Length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-Type: %s\r\n\r\n", buf, filetype);
  } else {
    // 기존 코드 유지
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  }
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers: \n");
  printf("%s", buf);

  srcfd = Open(filename, O_RDONLY, 0); // 요청한 파일 읽기 전용으로 연다, 파일 내용을 가상 메모리 공간에 매핑한다 -> srcp는 파일을 가리키는 포인터가 된다
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // 파일을 메모리에 매핑한다. -> 파일 내용을 한 번에 네트워크로 전송 가능, 데이터 변경 내용 공유 x 
  Close(srcfd); // 파일 디스크립터는 이제 필요 없으니까 닫음
  Rio_writen(fd, srcp, filesize);
  Munmap(srcp, filesize);
  
  // srcfd = Open(filename, O_RDONLY, 0);
  // Rio_readn(srcfd, srcp, filesize);
  // Close(srcfd);
  // Rio_writen(fd, srcp, filesize);
  // Free(srcp);
}

/*
 * mmap 사용법 : 파일을 메모리에 매핑하여, 파일 내용을 포인터로 메모리처럼 접근이 가능함
 * mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
 * addr : 0 (커널이 알아서 주소 정해줌), length : 매핑할 크기, prot : 메모리 보호 -> PROT_READ, flags : 매핑 방식 (MAP_PRIVATE : 쓰기 변경 시 복사 됨), fd 매핑 할 파일 디스크립터, offset : 파일의 시작 위치
 */

// filename이 해당하는 내용 .html .gif. png 등등을 찾아서 text/html, image/png와 같이 바꿔준다
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".mp4"))
    strcpy(filetype, "video/mp4");
  else
    strcpy(filetype, "text/plain");
}


// 클라이언트가 요청한 CGI Program 실행 & 전송, 클라이언트와 연결된 소켓 파일 디스크립터와, cgi program의 경로(filename)을 받음
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = { NULL };
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (Fork() == 0) { // 자식 프로세스
    setenv("QUERY_STRING", cgiargs, 1); // 마지막 인자 : overwrite on (0이 아닌 경우)
    Dup2(fd, STDOUT_FILENO); // fd가 가리키는 소켓으로 표준 출력이 바뀐다 -> CGI program의 출력이 클라이언트에게 전송됨
    Execve(filename, emptylist, environ); // 새 프로그램을 메모리에 올리고 0부터 시작, 현재 프로세스 이미지를 완전히 교체함, 실패시 - 1 반환, 2번째 인자에는 char *const argv[] 새 프로그램에 전달할 인자 배열들이 들어간다, char *const evp[] 또한 환경 변수 배열이다
  } 
  // Wait(NULL); // 자식 프로세스가 종료될때까지 부모 프로세스 블록 시킴 -> NULL은 자식의 종료상태가 필요없지 않다는 의미를 뜻함
}

void sigchld_handler(int sig)
{
  int status;
  pid_t pid;
  char buf[] = "Helloo\n"; 
  char err_buf[] = "There's no child process";

  pid = waitpid(-1, &status, WNOHANG);
  // waitpid(pid_t pid, int *status, int options) pid -1의 의미는 "종료된 자식 프로세스 아무거나 기다리기", 일반적인 값은 기다릴 자식 프로세스, &status는 자식 프로세스의 상태를 저장할 포인터, WNOHANG : 자식 기다리지 않고, 즉시 반환(NON BLOCKING)
  while(pid > 0) {  
    if (pid < 0)
      write(STDERR_FILENO, err_buf, strlen(err_buf));

    write(STDOUT_FILENO, buf, strlen(buf));
  }
}

// Q. wait 사용 이유? A. 부모가 먼저 종료될 수 있으니, wait 함수를 사용한다 
/*
  pid_t pid = fork()  
  if (pid == 0) {
    sleep(3);
    printf("Child Done\n");
  } else {
    sleep(1);
    printf("Parent Done\n"); 
  }
  이때, 부모가 먼저 종료된 뒤, 자식이 종료됨
  자식은 종료된 상태값을 부모에게 전달해야하는데, 부모가 종료되어있기에, 커널은 자식의 종료 상태를 전달하지 못함
  좀비가 많아지면 PID가 고갈되고, 시스템이 불안정해진다
*/


// Q. fork를 사용하지 않고, Execve를 사용하면 어떤 일이 일어나는가?
/*
  프로세스 이미지가 통째로 CGI Program으로 바뀐다
  따라서, CGI 실행을 자식에게 위임하고, 서버는 계속 살아있도록 만든다
*/

/*
 * 1. 왜 localhost, 51614 
 * 뒤에 인자가 들어오지 않아서 무한 루프에 빠지는거임 인자가 없으면
 */