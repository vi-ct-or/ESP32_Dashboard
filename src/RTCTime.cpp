#include "RTCTime.h"
#include <sys/time.h>
#include "RTClib.h"
#include "network.h"

RTC_DS3231 rtc;
RTC_DATA_ATTR bool rtcAvailableFlag = false;

bool setRtcTime()
{
    struct tm tm;
    time_t now;
    bool l_ret = false;
    uint8_t prevSec;
    if (rtc.begin() && getLocalTime(&tm))
    {
        rtc.disable32K();
        prevSec = tm.tm_sec;
        while (prevSec == tm.tm_sec)
        {
            getLocalTime(&tm);
        }
        time(&now);
        tm = *gmtime(&now);
        rtc.adjust(DateTime(tm.tm_year + 1900, tm.tm_mon + 1,
                            tm.tm_mday, tm.tm_hour,
                            tm.tm_min, tm.tm_sec));
        l_ret = true;
        // Serial.println("OK setRtcTime");
        // Serial.print("RTC time :  ");
        // Serial.print(tm.tm_year + 1900);
        // Serial.print('/');
        // Serial.print(tm.tm_mon + 1);
        // Serial.print('/');
        // Serial.print(tm.tm_mday);
        // Serial.print("  ");
        // Serial.print(tm.tm_hour);
        // Serial.print(':');
        // Serial.print(tm.tm_min);
        // Serial.print(':');
        // Serial.print(tm.tm_sec);
        // Serial.println();
        rtcAvailableFlag = true;
    }
    else
    {
        Serial.println("error in setRtcTime");
    }
    return l_ret;
}

bool adjustLocalTimeFromRtc()
{
    Serial.println("Start function adjustLocalTimeFromRtc");
    uint8_t prevSec;
    if (rtc.begin())
    {
        setenv("TZ", "GMT0", 1);
        tzset();
        DateTime now = rtc.now();
        struct tm tmTmp;
        prevSec = now.second();
        timeval tv;
        while (prevSec == now.second())
        {
            now = rtc.now();
        }

        tv.tv_sec = now.unixtime();
        tv.tv_usec = 0; // set microseconds
        settimeofday(&tv, NULL);

        setenv("TZ", "CET-1CEST-2,M3.5.0/2,M10.5.0/3", 1);
        tzset();

        // Serial.print("RTC time :  ");
        // Serial.print(now.year(), DEC);
        // Serial.print('/');
        // Serial.print(now.month(), DEC);
        // Serial.print('/');
        // Serial.print(now.day(), DEC);
        // Serial.print("  ");
        // Serial.print(now.hour(), DEC);
        // Serial.print(':');
        // Serial.print(now.minute(), DEC);
        // Serial.print(':');
        // Serial.print(now.second(), DEC);
        // Serial.println();

        return true;
    }
    Serial.println("error in adjustLocalTimeFromRtc");
    return false;
}

bool rtcAvailable()
{
    bool ret = false;
    if (rtc.begin() && rtcAvailableFlag == true)
    {
        ret = true;
    }
    return ret;
}

void resetClock()
{
    if (rtc.begin())
    {
        rtcAvailableFlag = false;
        rtc.adjust(DateTime(2000, 1, 1, 0, 0, 0));
        DateTime now = rtc.now();
        struct tm tmTmp;
        timeval tv;

        tv.tv_sec = now.unixtime();
        tv.tv_usec = 0; // set microseconds
        // set local time to before 2016 to have getLocalTime() return false
        settimeofday(&tv, NULL);
    }
}
