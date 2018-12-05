#ifndef _RTC_H
#define _RTC_H

#include "stm32f4xx.h"


#define   RTC_CTL1		0x00
#define   RTC_CTL2		0x01
#define   RTC_SEC		0x02
#define   RTC_MIN		0x03
#define   RTC_HOUR		0x04
#define   RTC_DAY		0x05
#define   RTC_WEEK		0x06
#define   RTC_MON		0x07
#define   RTC_YEAR		0x08
#define   RTC_A_MIN 	0x09
#define   RTC_A_HOUR	0x0A
#define   RTC_A_DAY 	0x0B
#define   RTC_A_WEEK	0x0C
#define   RTC_CLKOUT	0x0D
#define   RTC_T_CTL 	0x0E
#define   RTC_T_COUNT	0x0F
#define   CENTURY	0x80
#define   TI		0x10
#define   AF		0x08
#define   TF		0x04
#define   AIE		0x02
#define   TIE		0x01
#define   FE		0x80
#define   TE		0x80
#define   FD1		0x02
#define   FD0		0x01
#define   TD1		0x02
#define   TD0		0x01
#define   VL		0x80

#define HYM8563_REG_LEN 	0x10
#define HYM8563_RTC_SECTION_LEN	0x07

#define   TIME_DEBUG		0


struct rtc_time {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

uint8_t bin2bcd(uint8_t val);
uint8_t bin2bcd(uint8_t val);
uint8_t hym8563_set_time(u8 ucDevAddr, struct rtc_time *tm);
uint8_t hym8563_read_datetime(u8 ucDevAddr, struct rtc_time *tm);

uint8_t hym8563_i2c_set_regs(u8 ucDevAddr, u8 reg, u8 *buf, u8 len);
uint8_t hym8563_i2c_read_regs(u8 ucDevAddr, u8 reg, u8 *buf, u8 len);

#endif