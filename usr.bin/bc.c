#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

static const char *p;

static void skip_ws(void) {
  while (*p && isspace(*p))
    p++;
}

static int parse_expr();

static int parse_number() {
  skip_ws();
  int v = atoi(p);
  while (*p && isdigit(*p))
    p++;
  return v;
}

static int parse_factor() {
  skip_ws();

  if (*p == '(') {
    p++;
    int v = parse_expr();
    skip_ws();
    if (*p == ')')
      p++;
    return v;
  }

  return parse_number();
}

static int parse_term() {
  int v = parse_factor();

  while (1) {
    skip_ws();

    if (*p == '*') {
      p++;
      v *= parse_factor();
    } else if (*p == '/') {
      p++;
      v /= parse_factor();
    } else
      break;
  }

  return v;
}

static int parse_expr() {
  int v = parse_term();

  while (1) {
    skip_ws();

    if (*p == '+') {
      p++;
      v += parse_term();
    } else if (*p == '-') {
      p++;
      v -= parse_term();
    } else
      break;
  }

  return v;
}

int main(void) {
  char buf[256];

  while (fgets(buf, sizeof(buf), stdin)) {
    p = buf;
    int result = parse_expr();
    printf("%d\n", result);
  }

  return 0;
}