#include <stddef.h>
#include <time.h>

int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static inline int is_leap_year(int year) {
  return (year % 4) == 0 && ((year % 100) != 0 || ((year + 1900) % 400) == 0);
}

static inline int days_in_year(int year) { return is_leap_year(year) ? 366 : 365; }

time_t mktime(struct tm *tm) {
  if (tm == NULL)
    return 0;

  // Days
  time_t result = tm->tm_mday - 1;

  // Days in previous months
  int m = tm->tm_mon;
  while (m-- > 0) {
    result += days_in_month[m];
  }
  // Extra day in february in leap years
  if (tm->tm_mon > 1 && days_in_year(tm->tm_year) == 366) {
    result++;
  }

  // Days in years
  for (int y = 70; y < tm->tm_year; y++) {
    result += days_in_year(y);
  }

  // Convert days to seconds
  result *= 60 * 60 * 24;

  // Add seconds, minutes in seconds and hours in seconds
  result += tm->tm_sec + (tm->tm_min * 60) + (tm->tm_hour * 60 * 60);

  return result;
}
