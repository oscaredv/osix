#ifndef __INCLUDE_CTYPE_H__
#define __INCLUDE_CTYPE_H__

static inline int isprint(int c) { return c >= 32 && c < 127; }
static inline int isdigit(int c) { return c >= '0' && c <= '9'; }
static inline int isspace(int c) { return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r'; }
static inline int ispunct(int c) {
  return c == '!' || c == '"' || c == '#' || c == '$' || c == '%' || c == '&' || c == '\'' || c == '(' || c == ')' ||
         c == '*' || c == '+' || c == ',' || c == '-' || c == '.' || c == '/' || c == ':' || c == ';' || c == '<' ||
         c == '=' || c == '>' || c == '?' || c == '@' || c == '[' || c == '\\' || c == ']' || c == '^' || c == '_' ||
         c == '`' || c == '{' || c == '|' || c == '}' || c == '~';
}

#endif
