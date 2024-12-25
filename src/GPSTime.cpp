#include <TinyGPSPlus.h>
#include "GPSTime.h"
#include <sys/time.h>

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