/**
* @Author: Emrah Duatepe
*/

#include "M254SE3AE.h"
#include "time.h"

time_t time(time_t *t)
{
    S_RTC_TIME_DATA_T rtc;
    RTC_GetDateAndTime(&rtc);  // Nuvoton surucusu ile RTC oku

    struct tm tm_time;
    tm_time.tm_year = rtc.u32Year - 1900;
    tm_time.tm_mon  = rtc.u32Month - 1;
    tm_time.tm_mday = rtc.u32Day;
    tm_time.tm_hour = rtc.u32Hour;
    tm_time.tm_min  = rtc.u32Minute;
    tm_time.tm_sec  = rtc.u32Second;
	tm_time.tm_wday = rtc.u32DayOfWeek;
	tm_time.tm_isdst = 0;  // yaz saati yok,sadece metadata var,islevsiz

    time_t now = mktime(&tm_time);  // epoch’a çevir
	
    if (t) 
		*t = now;
	
    return now;
}
