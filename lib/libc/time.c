#include <stddef.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

// Time syscall
time_t time(time_t *t) { return syscall(SYS_time, t); }

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

struct tm *gmtime(const time_t *timep) {
  static struct tm result;
  if (timep == NULL) {
    return NULL;
  }

  time_t t = *timep;
  result.tm_sec = t % 60;
  t /= 60;
  result.tm_min = t % 60;
  t /= 60;
  result.tm_hour = t % 24;
  t /= 24;

  // Calculate the calendar date starting from the epoch (1970-01-01)
  int year = 1970;

  while (t >= days_in_year(year)) {
    t -= days_in_year(year);
    year++;
  }
  result.tm_year = year - 1900;

  // Determine the month and day of the month
  int leap = is_leap_year(year);
  int month = 0;
  while (t >= (days_in_month[month] + (leap && month == 1 ? 1 : 0))) {
    t -= days_in_month[month] + (leap && month == 1 ? 1 : 0);
    month++;
  }
  result.tm_mon = month;
  result.tm_mday = t;

  result.tm_isdst = 0;
  result.tm_wday = (t + 5) % 7;
  result.tm_yday = t - 1;
  for (int m = 0; m < month; m++) {
    result.tm_yday += days_in_month[m];
  }
  if (month >= 2 && is_leap_year(year))
    result.tm_yday++;

  return &result;
}

char *asctime(const struct tm *tm) {
  static const char *dname[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  static const char *mname[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  static char buf[32];
  snprintf(buf, sizeof(buf), "%s %d. %s %02d:%02d:%02d %04d", dname[tm->tm_wday], tm->tm_mday, mname[tm->tm_mon],
           tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_year + 1900);
  return buf;
}

char *ctime(const time_t *t) { return asctime(gmtime(t)); }
