#include <stdint.h>

static const char b64_table[] = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

// This password hashing algorithm is extremely insecure, and only appropriate for a toy OS
char *crypt(const char *key) {
  // Simple hashing using bitwise operations and a constant seed
  uint32_t hash = 0xDEADBEEF;
  const char *p = key;
  while (*p != 0) {
    hash ^= (*p++ * 16777619);
    hash = (hash << 5) | (hash >> 27);
  }

  static char output[7]; // 6 characters + null terminator
  // Convert hash to b64 format
  for (int i = 0; i < 6; i++) {
    output[i] = b64_table[hash & 63];
    hash >>= 6;
  }
  output[6] = 0;

  return output;
}
