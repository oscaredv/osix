#include <i386/io.h>
#include <i386/pit.h>
#include <i386/rtc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define CMOS_ADDR_REG 0x70
#define CMOS_DATA_REG 0x71

uint8_t cmos_read(uint8_t address) {
  outb(address, CMOS_ADDR_REG);
  io_wait();
  return inb(CMOS_DATA_REG);
}

static inline int rtc_bcd2bin(int bcd) { return (bcd & 0x0F) + (bcd >> 4) * 10; }

int rtc_read_clock(struct tm *tm) {
  if (tm == NULL)
    return -1;

  tm->tm_sec = rtc_bcd2bin(cmos_read(0x0));
  tm->tm_min = rtc_bcd2bin(cmos_read(0x2));
  tm->tm_hour = rtc_bcd2bin(cmos_read(0x4));

  tm->tm_mday = rtc_bcd2bin(cmos_read(0x7));
  tm->tm_mon = rtc_bcd2bin(cmos_read(0x8))-1;
  tm->tm_year = rtc_bcd2bin(cmos_read(0x9));
  tm->tm_year += rtc_bcd2bin(cmos_read(0x32)) * 100;
  tm->tm_year -= 1900;

  return 0;
}

void rtc_read() {
  struct tm tm;
  time_t time1, time2;

  do {
    // CMOS clock is updated once a second. Read it twice until both readings
    // are equal to make sure we didn't read while the clock was updating
    rtc_read_clock(&tm);
    time1 = mktime(&tm);
    rtc_read_clock(&tm);
    time2 = mktime(&tm);
  } while (time1 != time2);

  pit_settime(time1);

  printf("rtc0 at 0x%x: %02d:%02d:%02d %04d-%02d-%02d\n", CMOS_ADDR_REG,
         tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_year + 1900, tm.tm_mon+1,
         tm.tm_mday);
}
