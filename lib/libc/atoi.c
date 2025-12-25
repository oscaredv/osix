// Convert string to integer
int atoi(const char *s) {
  int i = 0;

  char neg = 0;
  while (*s != 0) {
    if (*s >= '0' && *s <= '9') {
      i *= 10;
      i += *s - '0';
    } else if (*s == '-' && i == 0) {
      neg = 1;
    } else {
      break;
    }

    ++s;
  }

  if (neg)
    return -i;

  return i;
}
