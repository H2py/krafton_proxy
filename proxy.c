#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

// 요구사항 : 클라이언트 요청, 웹 서버에게 전달하고 서버 응답 잃고 해당 클라이언트에게 응답을 전달한다
// 원래 요청이 HTTP/1.1인 경우에도 모든 요청은 HTTP/1.0으로 전달되어야 한다

// How to test? :   curl --proxy http://localhost:7000/ http://localhost:8000/ arg1 : proxy port, arg2 : tiny port

int main() {
  printf("%s", user_agent_hdr);
  return 0;
}
