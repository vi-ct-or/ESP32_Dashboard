#include <TinyGPSPlus.h>
#include "GPSTime.h"
#include <sys/time.h>
#include "RTClib.h"
#include "network.h"

#define RXD2 16
#define TXD2 17

TinyGPSPlus gps;
RTC_DS3231 rtc;

void initGpsTime()
{
    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
}

bool setGPSTime()
{

    time_t start = millis();
    while (millis() - start < 5000)
    {
        if (Serial2.available() > 0 && gps.encode(Serial2.read()))
        {
            Serial.println("available");

            if (gps.time.isValid() && gps.date.isValid() && gps.date.year() > 2023)
            {
                struct tm tm;
                time_t now;
                struct timeval tv;

                tm.tm_min = gps.time.minute();
                tm.tm_hour = gps.time.hour();
                tm.tm_mday = gps.date.day();
                tm.tm_mon = gps.date.month() - 1;
                tm.tm_year = gps.date.year() - 1900;
                tm.tm_sec = gps.time.second();

                now = mktime(&tm);
                tv.tv_sec = now;
                tv.tv_usec = 0;
                Serial.print("settimeofday : ");
                Serial.println(settimeofday(&tv, NULL));

                Serial.println("Time set");
                return true;
            }
        }
    }
    return false;
}

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
        Serial.println("OK setRtcTime");
        Serial.print("RTC time :  ");
        Serial.print(tm.tm_year + 1900);
        Serial.print('/');
        Serial.print(tm.tm_mon + 1);
        Serial.print('/');
        Serial.print(tm.tm_mday);
        Serial.print("  ");
        Serial.print(tm.tm_hour);
        Serial.print(':');
        Serial.print(tm.tm_min);
        Serial.print(':');
        Serial.print(tm.tm_sec);
        Serial.println();
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

        // tmTmp.tm_year = now.year() - 1900;
        // tmTmp.tm_mon = now.month() - 1;
        // tmTmp.tm_mday = now.day();
        // tmTmp.tm_hour = now.hour();
        // tmTmp.tm_min = now.minute();
        // tmTmp.tm_sec = now.second();
        // tmTmp.tm_isdst = 0;        // DST is not used
        // time_t t = mktime(&tmTmp); // convert to time_t
        tv.tv_sec = now.unixtime();
        tv.tv_usec = 0; // set microseconds
        settimeofday(&tv, NULL);
        Serial.print("setenv() =  ");

        Serial.println(setenv("TZ", "CET-1CEST-2,M3.5.0/2,M10.5.0/3", 1));
        tzset();

        Serial.print("RTC time :  ");
        Serial.print(now.year(), DEC);
        Serial.print('/');
        Serial.print(now.month(), DEC);
        Serial.print('/');
        Serial.print(now.day(), DEC);
        Serial.print("  ");
        Serial.print(now.hour(), DEC);
        Serial.print(':');
        Serial.print(now.minute(), DEC);
        Serial.print(':');
        Serial.print(now.second(), DEC);
        Serial.println();

        return true;
    }
    Serial.println("error in adjustLocalTimeFromRtc");
    return false;
}
