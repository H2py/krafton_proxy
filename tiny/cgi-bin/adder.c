/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1 = 0, n2 = 0;

  // QUERY_STRING=15000&213
  if((buf = getenv("QUERY_STRING")) != NULL) {
    p = strchr(buf, '&'); // 검색 대상 문자열 strchr(const char *ch, int c)에서 맨 처음 찾은 포인터 반환
    *p = '\0'; // &를 찾아서 개행문자로 바꾸면 2개로 쪼개짐
    strcpy(arg1, buf); // strcpy(char *dest, const char *src) dest로 src의 문자열을 복사해넣는 함수
    strcpy(arg2, p+1);
    n1 = atoi(arg1);
    n2 = atoi(arg2);
  }

  sprintf(content, "QUERY_STRING=%s", buf);
  sprintf(content, "Welcome to add.com: ");
  sprintf(content, "%sThe Internet addition portal.\r\n<p>", content);
  sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>", content, n1, n2, n1 + n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);

  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  printf("%s", content);
  fflush(stdout);

  exit(0);
}
/* $end adder */