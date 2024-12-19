#include <TinyGPSPlus.h>
#include "GPSTime.h"

#define RXD2 16
#define TXD2 17

TinyGPSPlus gps;

void initGpsTime()
{
    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
}

bool setGPSTime()
{

    time_t start = millis();
    while (millis() - start < 1000)
    {
        if (Serial2.available() > 0 && gps.encode(Serial2.read()))
        {
            Serial.println("available");

            if (gps.time.isValid() && gps.date.isValid())
            {
                struct tm tm;
                time_t now;
                struct timeval tv;

                tm.tm_sec = gps.time.second();
                tm.tm_min = gps.time.minute();
                tm.tm_hour = gps.time.hour();
                tm.tm_mday = gps.date.day();
                tm.tm_mon = gps.date.month() - 1;
                tm.tm_year = gps.date.year() - 1900;

                now = mktime(&tm);
                tv.tv_sec = now;
                tv.tv_usec = 0;

                settimeofday(&tv, NULL);

                Serial.println("Time set");
                return true;
            }
        }
    }
    return false;
}