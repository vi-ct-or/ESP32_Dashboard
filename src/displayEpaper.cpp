#define ENABLE_GxEPD2_GFX 0

#include <GxEPD2_BW.h>
#include "time.h"
#include <string>
#include <list>
#include "strava.h"
#include "polyline.h"
#include "logo.h"
#include <sys/time.h>
#include "network.h"
#include "Fonts/FreeSans9pt7b.h"
#include "Fonts/FreeSans12pt7b.h"
#include "Fonts/FreeSans18pt7b.h"
#include "Fonts/FreeSans24pt7b.h"

#include "displayEpaper.h"

#define SQUARE_SIZE 150
#define DAY_IN_SEC 3600 * 24
#define WEEK_IN_SEC 604800
#define WEEK_NB 53
GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(GxEPD2_420_GDEY042T81(/*CS=5*/ 5, /*DC=*/4, /*RES=*/19, /*BUSY=*/15)); // 400x300, SSD1683
// SCL(SCK)=18,SDA(MOSI)=23

int getMaxLat();
int getMaxLng();
int getMinLat();
int getMinLng();
void drawDateStr(const void *pv);
void drawStatus(const void *pv);
void drawTemperature(const void *pv);
void drawLoadingCircle(const void *pv);
void drawTimeStr(const void *pv);
void drawYearStr(const void *pv);
void drawYearDistance(const void *pv);
void drawYearTime(const void *pv);
void drawYearDeniv(const void *pv);
void drawYearTitle(const void *pv);
void drawStravaPolyline(const void *pv);
void drawFull(const void *pv);
void drawLastTwelveMonths(const void *pv);
void drawWeeks(const void *pv);
void drawLastActivity(const void *pv);
void secondsToHour(time_t timestamp, std::string *out);
void printSTDString(std::string str);
void drawUpdating(const void *pv);
void drawTimeSync(const void *pv);
bool isLeap(int year);
void getYearAndWeek(tm TM, int &YYYY, int &WW);
std::string speedToPace(double speedKmH);
std::string addNewLines(const std::string &input, int maxWidth, int maxLine, uint8_t *nbLine);
std::string replaceSpecialCharacters(const char *inputStr);
void printMultiLine(const char *str, int16_t x, int16_t y, uint16_t lineHeight, uint8_t lineNb);

RTC_DATA_ATTR bool prevGPSSync = false;
RTC_DATA_ATTR bool firstTime = true;
RTC_DATA_ATTR bool hasBeenRefreshed = false;
QueueHandle_t xQueueDisplay;

void initDisplay(void)
{
    // init display

    display.init(9600, false /*true*/, 50, false);
    display.setRotation(1);
    display.setTextSize(2);
    // display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.hibernate();
}

void displayTemplate()
{

    display.drawPaged(drawFull, 0);
    display.hibernate();
}

void displayTime(struct tm *now)
{
    display.drawPaged(drawTimeStr, (const void *)now);
    display.hibernate();
}
void displayStatus()
{
    display.drawPaged(drawStatus, 0);
    display.hibernate();
}
void displayDate(struct tm *now)
{
    display.drawPaged(drawDateStr, (const void *)now);
    display.hibernate();
}

void displayStravaAllYear(struct tm *now)
{
    initDB();
    // display.drawPaged(drawYearTitle, (const void *)now);
    //  display.drawPaged(drawYearStr, 0);
    display.drawPaged(drawYearDistance, 0);
    display.drawPaged(drawYearTime, 0);
    display.drawPaged(drawYearDeniv, 0);
    display.hibernate();
}

void displayStravaMonths(struct tm *now)
{
    initDB();
    display.drawPaged(drawLastTwelveMonths, (const void *)now);
    display.hibernate();
}

void displayStravaWeeks(struct tm *now)
{
    initDB();
    display.drawPaged(drawWeeks, (const void *)now);
    display.hibernate();
}

void displayLastActivity()
{
    display.drawPaged(drawLastActivity, 0);
    display.hibernate();
}

void displayUpdating(uint8_t state)
{
    if (state == 0)
    {
        display.clearScreen();
    }
    display.drawPaged(drawUpdating, (void *)&state);
    display.hibernate();
}

void displayTimeSync(bool gpsSync)
{
    if (gpsSync != prevGPSSync || firstTime)
    {
        display.drawPaged(drawTimeSync, (void *)&gpsSync);
        display.hibernate();
        prevGPSSync = gpsSync;
        firstTime = false;
    }
}

void displayStravaPolyline()
{
    display.drawPaged(drawStravaPolyline, 0);
    display.hibernate();
}

void displayTemperature()
{
    display.drawPaged(drawTemperature, 0);
    display.hibernate();
}

void displaySunsetSunrise()
{
    display.drawPaged(drawLoadingCircle, 0);
    display.hibernate();
}

int getMaxLat()
{
    int maxLat = -1800000;
    for (std::list<TsCoordinates>::iterator it = coordList.begin(); it != coordList.end(); ++it)
    {
        if (it->lat > maxLat)
        {
            maxLat = it->lat;
        }
    }
    return maxLat;
}
int getMaxLng()
{
    int maxLng = -1800000;
    for (std::list<TsCoordinates>::iterator it = coordList.begin(); it != coordList.end(); ++it)
    {
        if (it->lng > maxLng)
        {
            maxLng = it->lng;
        }
    }
    return maxLng;
}
int getMinLat()
{
    int minLat = 1800000;
    for (std::list<TsCoordinates>::iterator it = coordList.begin(); it != coordList.end(); ++it)
    {
        if (it->lat < minLat)
        {
            minLat = it->lat;
        }
    }
    return minLat;
}
int getMinLng()
{
    int minLng = 1800000;
    for (std::list<TsCoordinates>::iterator it = coordList.begin(); it != coordList.end(); ++it)
    {
        if (it->lng < minLng)
        {
            minLng = it->lng;
        }
    }
    return minLng;
}

void drawText(int16_t x, int16_t y, const char *text)
{
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
    Serial.printf("Text bounds: x1=%d, y1=%d, w=%d, h=%d\n", x1, y1, w, h);
    display.setPartialWindow(x1, y1, w, h);
    display.setCursor(x, y);
    display.print(text);
}

void drawStatus(const void *pv)
{
    display.setPartialWindow(270, 0, 30, 16);
    if (isWifiConnected())
    {
        display.drawBitmap(280, 0, networkBitmap, 16, 12, GxEPD_BLACK);
    }
    else
    {
        display.drawBitmap(280, 0, noNetworkBitmap, 16, 12, GxEPD_BLACK);
    }

    display.drawRect(5, 2, 20, 12, GxEPD_BLACK);

    display.fillRect(5, 3, 13, 10, GxEPD_BLACK);

    // display.setTextSize(1);
    // display.setCursor(245, 5);
    // display.print("3.65V");
}

void drawTemperature(const void *pv)
{
    uint8_t xOffsetFactor = 3;
    display.setPartialWindow(100, 0, 170, 16);

    // if (tempDataOldness > 3600)
    // {
    //     // data is old, display "N/A"
    //     display.setTextSize(1);
    //     display.setCursor(150, 0);
    //     display.print("N/A");
    //     return;
    // }

    display.setTextSize(2);

    if (airTemperature < 98.0)
    {

        // air Temperature
        display.setCursor(100, 0);
        display.print(airTemperature, 1);
        display.setTextSize(1);

        if (airTemperature < -9.9)
        {
            xOffsetFactor = 5;
        }
        else if (airTemperature < 0.0 || airTemperature > 9.9)
        {
            xOffsetFactor = 4;
        }

        display.setCursor(100 + xOffsetFactor * 12, -2);
        display.print("o");
        display.setCursor(100 + xOffsetFactor * 12 + 4, 6);
        display.print("C");
    }

    // data oldness
    // display.setTextSize(1);
    // display.setCursor(185, 0);
    // display.print(tempDataOldness);
    // display.print("s");

    // water Temperature
    xOffsetFactor = 3;

    if (waterTemperature < 98.0)
    {

        display.setTextSize(2);
        display.setCursor(200, 0);
        display.print(waterTemperature, 1);
        display.setTextSize(1);
        if (waterTemperature < -9.9)
        {
            xOffsetFactor = 5;
        }
        else if (waterTemperature < 0.0 || waterTemperature > 9.9)
        {
            xOffsetFactor = 4;
        }

        display.setCursor(200 + xOffsetFactor * 12, -2);
        display.print("o");
        display.setCursor(200 + +xOffsetFactor * 12 + 4, 6);
        display.print("C");
    }

    display.drawCircle(8, 8, 7, GxEPD_BLACK);
}

void drawLoadingCircle(const void *pv)
{
    // Draw the outer circle
    int x = 8;
    int y = 8;
    int radius = 7;

    uint32_t currentTimestamp = time(NULL);

    uint32_t fillPercent = ((float)(currentTimestamp - sunriseTimestamp) / (float)(sunsetTimestamp - sunriseTimestamp)) * 100.0;

    // fillPercent = 75;                    // for test
    int steps = 100 * fillPercent / 100; // 20 steps for a full circle

    // Serial.printf("Current timestamp: %u, Sunrise: %u, Sunset: %u, Fill percent: %u%%\n", currentTimestamp, sunriseTimestamp, sunsetTimestamp, fillPercent);

    display.setPartialWindow(0, 0, 16, 16);

    display.drawCircle(x, y, radius, GxEPD_BLACK);

    if (currentTimestamp < sunriseTimestamp)
    {
        return;
    }
    else if (currentTimestamp > sunsetTimestamp)
    {
        display.fillCircle(x, y, radius, GxEPD_BLACK);
    }
    else
    {

        // Simulate filling the circle step by step
        for (uint16_t i = 0; i <= steps; i++)
        {
            // Calculate the angle for the current step
            float angle = -PI / 2 + ((2 * PI * i) / 100);

            // Calculate the end point of the line for this step
            int16_t xEnd = static_cast<int>(std::round(x + radius * cos(angle)));
            int16_t yEnd = static_cast<int>(std::round(y + radius * sin(angle)));
            /*if (i == 0 || i == steps)
            {
                Serial.print("angle = ");
                Serial.print(angle);
                Serial.print(", radius * cos(angle) = ");
                Serial.print(radius * cos(angle));
                Serial.print(", radius * sin(angle) = ");
                Serial.print(radius * sin(angle));
                Serial.print(", xEnd = ");
                Serial.print(xEnd);
                Serial.print(", yEnd = ");
                Serial.println(yEnd);
            }*/

            // Draw a line from the center to the edge
            display.drawLine(x, y, xEnd, yEnd, GxEPD_BLACK);
        }
    }
}

void drawTimeStr(const void *pv)
{
    const struct tm *now = (const struct tm *)pv;
    display.setTextSize(2);
    display.setFont(&FreeSans18pt7b);
    display.setTextColor(GxEPD_BLACK);
    std::string timeStr;
    if (now->tm_hour < 10)
    {
        timeStr = "0";
    }
    timeStr += std::to_string(now->tm_hour) + ":";
    if (now->tm_min < 10)
    {
        timeStr += "0";
    }
    timeStr += std::to_string(now->tm_min);

    // drawText(120, 70, timeStr.c_str());
    display.setPartialWindow(120, 16, 300 - 120, 70);
    // display.drawRect(120, 16, 300 - 120, 70, GxEPD_BLACK);
    display.setCursor(124, 73);
    display.print(timeStr.c_str());
    display.setFont();
}

void drawDateStr(const void *pv)
{
    const struct tm *now = (const struct tm *)pv;
    std::string dateStr, day, nb, month;
    switch (now->tm_wday)
    {
    case 0:
        day = "Dimanche";
        break;
    case 1:
        day = "Lundi";
        break;
    case 2:
        day = "Mardi";
        break;
    case 3:
        day = "Mercredi";
        break;
    case 4:
        day = "Jeudi";
        break;
    case 5:
        day = "Vendredi";
        break;
    case 6:
        day = "Samedi";
        break;

    default:
        break;
    }

    switch (now->tm_mon)
    {
    case 0:
        month = "Janvier";
        break;
    case 1:
        month = "F";
        month += (char)0xE9;
        month += "vrier";
        break;
    case 2:
        month = "Mars";
        break;
    case 3:
        month = "Avril";
        break;
    case 4:
        month = "Mai";
        break;
    case 5:
        month = "Juin";
        break;
    case 6:
        month = "Juillet";
        break;
    case 7:
        month = "Ao";
        month += (char)0xFB;
        month += "t";
        break;
    case 8:
        month = "Septembre";
        break;
    case 9:
        month = "Octobre";
        break;
    case 10:
        month = "Novembre";
        break;
    case 11:
        month = "D";
        month += (char)0xE9;
        month += "cembre";
        break;

    default:
        break;
    }

    uint8_t maxLen = max(day.size(), month.size());

    nb = std::to_string(now->tm_mday);

    display.setTextSize(2);
    uint8_t Yoffset = 8;

    display.setPartialWindow(0, 8 + Yoffset, 120, 60);
    uint8_t x = 0;
    // day
    x = 113 / 2 - day.size() * 6;
    display.setCursor(x, 10 + Yoffset);
    display.print(day.c_str());
    // nb
    if (nb == "1")
    {
        x = 113 / 2 - nb.size() * 6 - 6;
        display.setCursor(55, 28 + Yoffset);
        display.setTextSize(1);
        display.print("er");
        display.setTextSize(2);
    }
    else
    {
        x = 113 / 2 - nb.size() * 6;
    }
    display.setCursor(x, 30 + Yoffset);
    display.print(nb.c_str());
    // month
    x = 113 / 2 - month.size() * 6;
    display.setCursor(x, 50 + Yoffset);
    display.print(month.c_str());
}

void drawYearStr(const void *pv)
{
    display.setTextSize(2);
    std::string yearStravaBike, yearStravaRun, dist, time, deniv;
    struct tm tm;
    getLocalTime(&tm);
    uint16_t currentDay = monthOffset[tm.tm_mon] + tm.tm_mday - 1;
    // bike
    dist = std::to_string(getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DISTANCE, 0, currentDay) / 10000);
    dist.insert(0, 5 - dist.size(), ' ');
    time = std::to_string(getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_TIME, 0, currentDay) / 3600);
    time.insert(0, 3 - time.size(), ' ');
    deniv = std::to_string(getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DENIV, 0, currentDay));
    deniv.insert(0, 6 - deniv.size(), ' ');
    yearStravaBike = dist;
    int16_t x, y;
    uint16_t w, h;
    // dist bike
    display.getTextBounds(dist.c_str(), 50, 105, &x, &y, &w, &h);
    display.setPartialWindow(x, y, w, h);
    display.setCursor(50, 113);
    display.print(dist.c_str());
    // time bike
    display.getTextBounds(time.c_str(), 130, 105, &x, &y, &w, &h);
    display.setPartialWindow(x, y, w, h);
    display.setCursor(120, 113);
    display.print(time.c_str());
    // // deniv bike

    // run
    dist = std::to_string(getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DISTANCE, 0, currentDay) / 10000);
    dist.insert(0, 5 - dist.size(), ' ');
    yearStravaRun = dist;
}

void drawYearDistance(const void *pv)
{
    display.setTextSize(2);
    std::string yearStravaBike, yearStravaRun, dist;
    struct tm tm;
    getLocalTime(&tm);
    uint16_t currentDay = monthOffset[tm.tm_mon] + tm.tm_mday - 1;
    // bike
    dist = std::to_string(getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DISTANCE, 0, currentDay) / 10000);
    dist.insert(0, 5 - dist.size(), ' ');
    yearStravaBike = dist;

    // run
    dist = std::to_string(getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DISTANCE, 0, currentDay) / 10000);
    dist.insert(0, 5 - dist.size(), ' ');
    yearStravaRun = dist;

    // dist bike
    int16_t x1b, y1b, x1r, y1r, x1, y1;
    uint16_t wb, hb, wr, hr, w, h;
    display.getTextBounds(yearStravaBike.c_str(), 58, 106, &x1b, &y1b, &wb, &hb);
    display.getTextBounds(yearStravaRun.c_str(), 58, 136, &x1r, &y1r, &wr, &hr);
    x1 = x1b; // bike is above run
    y1 = y1b; // bike is above run
    w = max(wb, wr);
    h = y1r + hr - y1b; // bike is above run
    display.setPartialWindow(x1, y1, w, h);
    display.setCursor(58, 106);
    display.print(yearStravaBike.c_str());
    display.setCursor(58, 136);
    display.print(yearStravaRun.c_str());
}

void drawYearTime(const void *pv)
{
    display.setTextSize(2);
    std::string yearStravaBike, yearStravaRun, time;
    struct tm tm;
    getLocalTime(&tm);
    uint16_t currentDay = monthOffset[tm.tm_mon] + tm.tm_mday - 1;
    // bike
    time = std::to_string(getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_TIME, 0, currentDay) / 3600);
    time.insert(0, 4 - time.size(), ' ');
    yearStravaBike = time;

    // run
    time = std::to_string(getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_TIME, 0, currentDay) / 3600);
    time.insert(0, 4 - time.size(), ' ');
    yearStravaRun = time;

    // dist bike
    int16_t x1b, y1b, x1r, y1r, x1, y1;
    uint16_t wb, hb, wr, hr, w, h;
    display.getTextBounds(yearStravaBike.c_str(), 140, 106, &x1b, &y1b, &wb, &hb);
    display.getTextBounds(yearStravaRun.c_str(), 140, 136, &x1r, &y1r, &wr, &hr);
    x1 = x1b; // bike is above run
    y1 = y1b; // bike is above run
    w = max(wb, wr);
    h = y1r + hr - y1b; // bike is above run
    display.setPartialWindow(x1, y1, w, h);
    display.setCursor(140, 106);
    display.print(yearStravaBike.c_str());
    display.setCursor(140, 136);
    display.print(yearStravaRun.c_str());
}
void drawYearDeniv(const void *pv)
{
    display.setTextSize(2);
    std::string yearStravaBike, yearStravaRun, deniv;
    struct tm tm;
    getLocalTime(&tm);
    uint16_t currentDay = monthOffset[tm.tm_mon] + tm.tm_mday - 1;
    // bike
    deniv = std::to_string(getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DENIV, 0, currentDay));
    deniv.insert(0, 6 - deniv.size(), ' ');
    yearStravaBike = deniv;

    // run
    deniv = std::to_string(getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DENIV, 0, currentDay));
    deniv.insert(0, 6 - deniv.size(), ' ');
    yearStravaRun = deniv;

    // dist bike
    int16_t x1b, y1b, x1r, y1r, x1, y1;
    uint16_t wb, hb, wr, hr, w, h;
    display.getTextBounds(yearStravaBike.c_str(), 220, 106, &x1b, &y1b, &wb, &hb);
    display.getTextBounds(yearStravaRun.c_str(), 220, 136, &x1r, &y1r, &wr, &hr);
    x1 = x1b; // bike is above run
    y1 = y1b; // bike is above run
    w = max(wb, wr);
    h = y1r + hr - y1b; // bike is above run
    display.setPartialWindow(x1, y1, w, h);
    display.setCursor(220, 106);
    display.print(yearStravaBike.c_str());
    display.setCursor(220, 136);
    display.print(yearStravaRun.c_str());
}

void drawYearTitle(const void *pv)
{
    const struct tm *now = (const struct tm *)pv;
    display.setPartialWindow(0, 65, 100, 23);
    display.setCursor(10, 74);
    display.setTextSize(2);
    display.print(std::to_string(now->tm_year + 1900).c_str());
}

void drawFull(const void *pv)
{
    display.setFullWindow();

    display.setTextSize(1);
    display.setCursor(55, 90);
    std::string header = "   Distance      Temps        Denivel";
    header += (char)0xE9;
    display.print(header.c_str());
    display.drawBitmap(0, 100, bicycleBitMap, 42, 28, GxEPD_BLACK);
    display.drawBitmap(0, 100 + 30, runningShoeBitmap, 42, 28, GxEPD_BLACK);
    uint16_t y = 113;

    display.setTextSize(1);
    display.setCursor(120, y);
    display.print("km");
    display.setTextSize(1);
    display.setCursor(120, y + 30);
    display.print("km");

    display.setCursor(190, y);
    display.print("h");
    display.setTextSize(1);
    display.setCursor(190, y + 30);
    display.print("h");

    display.setCursor(294, y);
    display.print("m");
    display.setTextSize(1);
    display.setCursor(294, y + 30);
    display.print("m");

    // display.drawLine(0, 128, 300, 128, GxEPD_BLACK);
    // display.drawLine(0, 158, 300, 158, GxEPD_BLACK);
}

void drawStravaPolyline(const void *pv)
{
    TsActivity *lastAct = getStravaLastActivity();
    std::string *lastPolyline = getStravaLastPolyline();
    display.setPartialWindow(150, 250, SQUARE_SIZE, SQUARE_SIZE);
    if (lastAct == NULL || lastAct->isFilled == false)
    {
        Serial.println("lastAct NULL");
        return;
    }
    Serial.println("lastAct not NULL");
    if (!lastPolyline->empty())
    {
        decode(lastPolyline->c_str(), lastPolyline->size());
        int maxLat = getMaxLat();
        int maxLng = getMaxLng();
        int minLat = getMinLat();
        int minLng = getMinLng();

        int maxDiff = max(maxLat - minLat, maxLng - minLng);
        if (maxDiff == 0)
        {
            Serial.println("maxDiff is 0, no course to display");
            return;
        }
        int minDiff = min(maxLat - minLat, maxLng - minLng);
        int offsetV = 0;
        int offsetH = 0;
        int pixelLargeur;
        pixelLargeur = minDiff * SQUARE_SIZE / maxDiff;

        if (minDiff == maxLat - minLat)
        {
            // only apply to y
            offsetV = (SQUARE_SIZE - pixelLargeur) / 2;
        }
        else
        {
            // only apply to x
            offsetH = (SQUARE_SIZE - pixelLargeur) / 2;
        }

        int x, y, prevx = -1, prevy = -1;

        for (std::list<TsCoordinates>::iterator it = coordList.begin(); it != coordList.end(); ++it)
        {
            x = 149 + (int)(((float)((it->lng - minLng) * SQUARE_SIZE)) / (float)maxDiff) + offsetH;
            y = 249 + SQUARE_SIZE - (int)(((float)((it->lat - minLat) * SQUARE_SIZE)) / (float)maxDiff) - offsetV;
            if (prevx != -1 && prevy != -1)
            {
                display.drawLine(prevx, prevy, x, y, GxEPD_BLACK);
            }
            prevx = x;
            prevy = y;
        }
        coordList.clear();
    }
    else
    {
        Serial.println("No course to display");
        switch (lastAct->type)
        {
        case ACTIVITY_TYPE_RUN:
            display.drawBitmap(150, 250, runningShoeBitmap, 28, 28, GxEPD_BLACK);
            break;
        case ACTIVITY_TYPE_BIKE:
            display.drawBitmap(150, 250, bicycleBitMap, 28, 28, GxEPD_BLACK);
            break;
        case ACTIVITY_TYPE_SWIM:
            display.drawBitmap(150, 250, swimBitmapLarge, 150, 150, GxEPD_BLACK);
            break;
        case ACTIVITY_TYPE_RACKET:
            display.drawBitmap(150, 250, racketBitmapLarge, 150, 150, GxEPD_BLACK);
            break;
        default:
            break;
        }
    }
}

void drawLastActivity(const void *pv)
{
    TsActivity *lastActivity = getStravaLastActivity();
    display.setPartialWindow(1, 259, 149, 140);
    if (lastActivity == NULL || lastActivity->isFilled == false)
    {
        Serial.println("lastActivity NULL");
        return;
    }
    Serial.println("lastlastActivityAct not NULL");
    std::string displStr, dist, name, time, duration, deniv, speedOrPace;
    uint8_t lineNbTitle = 1;
    uint8_t heightLetter2 = 16;
    float speed = 0.0;
    name = replaceSpecialCharacters(lastActivity->name);
    name = addNewLines(name, 11, 3, &lineNbTitle);

    dist = std::to_string((float)lastActivity->dist / 100.0); // dist
    uint8_t dotIdx = dist.find('.');
    dist.resize(dotIdx + 3);
    dist.insert(0, 8 - dist.size(), ' ');
    secondsToHour(lastActivity->time, &duration); // duration
    duration.insert(0, 8 - duration.size(), ' ');
    deniv = std::to_string(lastActivity->deniv); // deniv
    deniv.insert(0, 8 - deniv.size(), ' ');

    speed = ((float)(((float)lastActivity->dist / 100.0)) / (float)lastActivity->time * 3600.0);

    display.setTextSize(1);
    if (lastActivity->type == ACTIVITY_TYPE_RUN)
    {
        speedOrPace = speedToPace(speed);
        if (lineNbTitle == 3)
        {
            display.setCursor(86, 267 + (lineNbTitle + 4) * heightLetter2 - heightLetter2 / 2);
        }
        else
        {
            display.setCursor(86, 267 + (lineNbTitle + 4) * heightLetter2);
        }
        display.print("min/km");
        speedOrPace.insert(0, 7 - speedOrPace.size(), ' ');
    }
    else
    {
        speedOrPace = std::to_string(speed);
        dotIdx = speedOrPace.find('.');
        speedOrPace.resize(dotIdx + 3);
        if (lineNbTitle == 3)
        {
            display.setCursor(98, 267 + (lineNbTitle + 4) * heightLetter2 - heightLetter2 / 2);
        }
        else
        {
            display.setCursor(98, 267 + (lineNbTitle + 4) * heightLetter2);
        }
        display.print("km/h");
        speedOrPace.insert(0, 8 - speedOrPace.size(), ' ');
    }

    displStr = name;
    displStr += "\n\n";
    displStr += dist;
    displStr += "\n";
    displStr += duration;
    displStr += "\n";
    displStr += deniv;
    displStr += "\n";
    displStr += speedOrPace;

    display.setCursor(1, 260);
    display.setTextSize(2);
    printMultiLine(displStr.c_str(), 1, 260, 16, lineNbTitle);
    // display.print(displStr.c_str());

    display.setTextSize(1);
    if (lineNbTitle == 3)
    {
        display.setCursor(98, 267 + (lineNbTitle + 1) * heightLetter2 - heightLetter2 / 2);
    }
    else
    {
        display.setCursor(98, 267 + (lineNbTitle + 1) * heightLetter2);
    }
    display.print("km");

    if (lineNbTitle == 3)
    {
        display.setCursor(98, 267 + (lineNbTitle + 3) * heightLetter2 - heightLetter2 / 2);
    }
    else
    {
        display.setCursor(98, 267 + (lineNbTitle + 3) * heightLetter2);
    }
    display.print("m d+");

    display.setCursor(83, 267);

    // kudos or lock
    if (lastActivity->isVisible == false)
    {
        display.drawBitmap(1, 400 - 18, lock, 17, 17, GxEPD_BLACK);
    }
    else if (lastActivity->kudos > 0)
    {
        display.drawBitmap(1, 400 - 18, myKudosBitmap, 17, 17, GxEPD_BLACK);
        display.setTextSize(1);
        display.setCursor(22, 400 - 11);
        display.print(lastActivity->kudos);

        if (lastActivity->kudos > prevKudos && prevKudos != 0)
        {
            // new kudos !
            display.print("(+");
            display.print(lastActivity->kudos - prevKudos);
            display.print(")");
        }
    }
    prevKudos = lastActivity->kudos;

    // streak
    uint32_t streak = getCurrentStreakDays();
    Serial.print("Current streak : ");
    Serial.println(streak);
    if (streak >= 2)
    {
        if (isLastActivityFromToday())
        {
            display.drawBitmap(90, 400 - 18, flame, 17, 17, GxEPD_BLACK);
        }
        else
        {
            display.drawBitmap(90, 400 - 14, flame_little, 12, 12, GxEPD_BLACK);
        }
        display.setTextSize(1);
        display.setCursor(108, 400 - 11);
        display.print(streak);
    }
}

void printMultiLine(const char *str, int16_t x, int16_t y, uint16_t lineHeight, uint8_t lineNb)
{
    int16_t cursorX = x;
    int16_t cursorY = y;
    const char *ptr = str;

    char lineBuffer[100];
    while (*ptr != '\0')
    {
        int lineLength = 0;
        // Read a line until newline or end of string
        while (*ptr != '\0' && *ptr != '\n' && lineLength < sizeof(lineBuffer) - 1)
        {
            lineBuffer[lineLength++] = *ptr++;
        }
        lineBuffer[lineLength] = '\0'; // Null-terminate the line

        // Print the line
        display.setCursor(cursorX, cursorY);
        display.print(lineBuffer);

        // Move to next line
        if (lineLength == 0 && lineNb == 3)
        {
            // Empty line => double new line =>
            cursorY += lineHeight / 4;
        }
        else
        {
            cursorY += lineHeight;
        }

        // If we stopped at a newline, skip it
        if (*ptr == '\n')
        {
            ptr++;
        }
    }
}

void secondsToHour(time_t timestamp, std::string *out)
{
    time_t remainingTime = timestamp;
    if (remainingTime / 3600 > 0)
    {
        out->append(std::to_string(remainingTime / 3600));
        out->append(":");
        remainingTime %= 3600;
        if (remainingTime / 60 < 10)
        {
            out->append("0");
        }
    }
    out->append(std::to_string(remainingTime / 60));
    out->append(":");
    remainingTime %= 60;
    if (remainingTime < 10)
    {
        out->append("0");
    }
    out->append(std::to_string(remainingTime));
}

void printSTDString(std::string str)
{
    for (uint8_t i = 0; i < str.size(); i++)
    {
        Serial.print(str[i]);
    }
    Serial.print("\n");
}

void drawLastTwelveMonths(const void *pv)
{
    uint16_t yearMon[12] = {0};
    const struct tm *now = (const struct tm *)pv;
    struct tm tmp;
    if (now->tm_mon == 11) // december
    {
        tmp.tm_year = now->tm_year;
        tmp.tm_mon = 0;
    }
    else
    {
        tmp.tm_year = now->tm_year - 1;
        tmp.tm_mon = now->tm_mon + 1;
    }

    tmp.tm_hour = 8;
    tmp.tm_min = 0;
    tmp.tm_sec = 0;
    uint16_t maxMonth = 1;

    for (uint8_t i = 0; i < 12; i++)
    {
        uint16_t startMonDay, endMonDay;
        uint32_t dist;
        tmp.tm_mon = (now->tm_mon + i + 1) % 12;
        if (tmp.tm_mon == 0 && i != 0)
        {
            tmp.tm_year++;
        }
        tmp.tm_mday = 1;
        mktime(&tmp);
        // Serial.print("start : ");
        // Serial.println(tmp.tm_yday);
        startMonDay = monthOffset[tmp.tm_mon];
        endMonDay = monthOffset[tmp.tm_mon + 1] - 1;
        // Serial.println(i);
        // Serial.print(startMonDay);
        // Serial.print(" - ");
        // Serial.println(endMonDay);
        // Serial.print(" : velo : ");
        dist = getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DISTANCE, startMonDay, endMonDay);
        // Serial.print(dist / 10000);
        // Serial.print(" ; run : ");
        // Serial.print(getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DISTANCE, startMonDay, endMonDay) / 10000);
        dist += getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DISTANCE, startMonDay, endMonDay);
        yearMon[i] = dist / 10000;
        // Serial.print(" ; total : ");
        // Serial.println(dist / 10000);
        if (dist / 10000 > maxMonth)
        {
            maxMonth = dist / 10000;
        }
    }

    uint16_t x = 50, w = 18, h, space = 3, hMax = 68, y = 235;

    display.setPartialWindow(0, y - hMax, 300, hMax + 12);
    for (uint8_t i = 0; i < 12; i++)
    {
        if (yearMon[i] > 0)
        {
            display.drawRect(x, y, w, -(yearMon[i] * hMax / maxMonth), GxEPD_BLACK);
        }
        display.setTextSize(1);
        display.setCursor(x + 6, y + 2);
        char monthLetter;
        uint8_t mon = (now->tm_mon + 1 + i) % 12;
        switch (mon)
        {
        case 0:
        case 5:
        case 6:
            monthLetter = 'J';
            break;
        case 1:
            monthLetter = 'F';
            break;
        case 2:
        case 4:
            monthLetter = 'M';
            break;
        case 3:
        case 7:
            monthLetter = 'A';
            break;
        case 8:
            monthLetter = 'S';
            break;
        case 9:
            monthLetter = 'O';
            break;
        case 10:
            monthLetter = 'N';
            break;
        case 11:
            monthLetter = 'D';
            break;

        default:
            monthLetter = '?';
            break;
        }
        display.print(monthLetter);
        x += w + space;
    }

    display.drawLine(30, y - hMax - 1, 300, y - hMax - 1, GxEPD_BLACK);
    display.drawLine(30, y - hMax / 2, 300, y - hMax / 2, GxEPD_BLACK);
    display.drawLine(30, y, 300, y, GxEPD_BLACK);
    display.setCursor(1, y - 3 - hMax);
    display.print(maxMonth);
    display.setCursor(1, y - 3 - hMax / 2);
    display.print(maxMonth / 2);
    display.setCursor(1, y - 3);
    display.print(0);
    display.print(" km");
}

void drawUpdating(const void *pv)
{
    uint8_t state = *(uint8_t *)pv;
    display.setTextSize(2);
    switch (state)
    {
    case 0:
        display.setFullWindow();
        drawText(1, 5, "New version available");
        break;
    case 1:
        drawText(1, 40, "Downloading new version");
        break;
    case 2:
        drawText(1, 80, "Download complete");
        break;
    case 3:
        drawText(1, 120, "Starting update");
        break;
    case 4:
        drawText(1, 160, "Update error, restarting");
        break;
    case 5:
        drawText(1, 160, "Update successfully done, restarting");
        break;
    default:
        break;
    }
}

void drawTimeSync(const void *pv)
{
    bool gpsSync = *(bool *)pv;
    display.setTextSize(1);
    if (gpsSync)
    {
        Serial.println("draw gps");
        drawText(250, 70, "GPS");
    }
    else
    {
        Serial.println("draw ntp");
        drawText(250, 70, "NTP");
    }
}

void drawWeeks(const void *pv)
{
    uint16_t weeks[WEEK_NB];
    uint8_t weeksToHide[5] = {UINT8_MAX, UINT8_MAX, UINT8_MAX, UINT8_MAX, UINT8_MAX};
    const struct tm *now = (const struct tm *)pv;
    struct tm tmTmp = *now;
    time_t timeTmp;
    time_t nowTime = mktime(&tmTmp);

    timeTmp = nowTime - (now->tm_wday + 6) % 7 * DAY_IN_SEC;
    tmTmp = *localtime(&timeTmp);

    // fill array
    uint16_t startWeekDay, endWeekDay;
    startWeekDay = monthOffset[tmTmp.tm_mon] + tmTmp.tm_mday - 1;
    endWeekDay = monthOffset[now->tm_mon] + now->tm_mday - 1;
    uint16_t maxWeek = 0;
    uint32_t dist;
    for (int8_t i = WEEK_NB - 1; i > -1; i--)
    {
        if (tmTmp.tm_mon == now->tm_mon && tmTmp.tm_year == now->tm_year - 1)
        {
            for (uint8_t j = 0; j < 5; j++)
            {
                if (weeksToHide[j] == UINT8_MAX)
                {
                    weeksToHide[j] = i;
                    Serial.print("week to hide : ");
                    Serial.println(i);
                    break;
                }
            }
        }

        if (startWeekDay > endWeekDay)
        {
            dist = getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DISTANCE, startWeekDay, 365);
            dist += getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DISTANCE, 0, endWeekDay);

            dist += getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DISTANCE, startWeekDay, 365);
            dist += getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DISTANCE, 0, endWeekDay);
        }
        else
        {
            dist = getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DISTANCE, startWeekDay, endWeekDay);
            dist += getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DISTANCE, startWeekDay, endWeekDay);
        }
        weeks[i] = dist / 10000;
        // Serial.print(startWeekDay);
        // Serial.print(" - ");
        // Serial.print(endWeekDay);
        // Serial.print("  ");
        // Serial.println(dist / 10000);

        if (weeks[i] > maxWeek)
        {
            maxWeek = weeks[i];
        }
        timeTmp -= DAY_IN_SEC;
        tmTmp = *localtime(&timeTmp);
        endWeekDay = monthOffset[tmTmp.tm_mon] + tmTmp.tm_mday - 1;

        timeTmp += DAY_IN_SEC;
        timeTmp -= WEEK_IN_SEC;
        tmTmp = *localtime(&timeTmp);
        startWeekDay = monthOffset[tmTmp.tm_mon] + tmTmp.tm_mday - 1;
    }

    // draw array
    int currentWeek, year;
    uint16_t w = 3, h, space = 2, hMax = 68, y = 235, x = 300 - w - space;
    display.setPartialWindow(0, y - hMax, 300, hMax + 12);
    uint8_t weekNb;
    timeTmp = nowTime;
    display.setTextSize(1);
    Serial.println("begin drawing weeks");
    for (int8_t i = WEEK_NB - 1; i > -1; i--)
    {
        bool skipThisWeek = false;
        for (uint8_t j = 0; j < 5; j++)
        {
            if (weeksToHide[j] == i)
            {
                skipThisWeek = true;
                break;
            }
        }
        if (skipThisWeek)
        {
            continue;
        }
        if (maxWeek == 0)
        {
            maxWeek = 100; // avoid division by 0
        }

        display.drawRect(x, y, w, -(weeks[i] * hMax / maxWeek), GxEPD_BLACK);

        tmTmp = *localtime(&timeTmp);
        getYearAndWeek(tmTmp, year, currentWeek);

        if ((currentWeek % 5 == 0 || currentWeek == 1) && i != WEEK_NB - 1)
        {
            Serial.print("current week print : ");
            Serial.println(currentWeek);
            display.drawLine(x + w / 2, y, x + w / 2, y - (weeks[i] * hMax / maxWeek), GxEPD_BLACK);
            if (currentWeek < 10)
            {
                display.setCursor(x, y + 2);
            }
            else
            {
                display.setCursor(x - 4, y + 2);
            }
            display.print(currentWeek);
        }
        timeTmp -= WEEK_IN_SEC;
        x -= (w + space);
    }

    display.drawLine(30, y - hMax - 1, 300, y - hMax - 1, GxEPD_BLACK);
    display.drawLine(30, y - hMax / 2, 300, y - hMax / 2, GxEPD_BLACK);
    display.drawLine(30, y, 300, y, GxEPD_BLACK);
    display.setCursor(0, y - 3 - hMax);
    display.print(maxWeek);
    display.setCursor(0, y - 3 - hMax / 2);
    display.print(maxWeek / 2);
    display.setCursor(0, y - 3);
    display.print(0);
    display.print(" km");
}

bool isLeap(int year)
{
    if (year % 4 == 0)
    {
        if (year % 100 == 0 && year % 400 != 0)
            return false;
        else
            return true;
    }
    return false;
}

void getYearAndWeek(tm TM, int &YYYY, int &WW) // Reference: https://en.wikipedia.org/wiki/ISO_8601
{
    YYYY = TM.tm_year + 1900;
    int day = TM.tm_yday;

    int Monday = day - (TM.tm_wday + 6) % 7;                       // Monday this week: may be negative down to 1-6 = -5;
    int MondayYear = 1 + (Monday + 6) % 7;                         // First Monday of the year
    int Monday01 = (MondayYear > 4) ? MondayYear - 7 : MondayYear; // Monday of week 1: should lie between -2 and 4 inclusive
    WW = 1 + (Monday - Monday01) / 7;                              // Nominal week ... but see below

    // In ISO-8601 there is no week 0 ... it will be week 52 or 53 of the previous year
    if (WW == 0)
    {
        YYYY--;
        WW = 52;
        if (MondayYear == 3 || MondayYear == 4 || (isLeap(YYYY) && MondayYear == 2))
            WW = 53;
    }

    // Similar issues at the end of the calendar year
    if (WW == 53)
    {
        int daysInYear = isLeap(YYYY) ? 366 : 365;
        if (daysInYear - Monday < 3)
        {
            YYYY++;
            WW = 1;
        }
    }
}

std::string speedToPace(double speedKmH)
{
    std::string out;
    if (speedKmH <= 0)
    {
        return "Invalid speed"; // Error message for invalid speed
    }

    // Calculate pace in minutes per kilometer
    double paceInMinutes = 60.0 / speedKmH;

    // Extract minutes and seconds from the pace
    int minutes = static_cast<int>(paceInMinutes);
    int seconds = static_cast<int>((paceInMinutes - minutes) * 60); // Convert fractional part to seconds

    // Create the formatted string "x:xx"
    out = std::to_string(minutes) + ":";
    if (seconds < 10)
    {
        out += "0";
    }
    out += std::to_string(seconds);

    return out;
}

std::string addNewLines(const std::string &input, int maxWidth, int maxLine, uint8_t *nbLine)
{
    std::string result;
    std::string word;
    int currentWidth = 0;
    int currentNbLine = 1;

    for (size_t i = 0; i < input.size(); ++i)
    {
        char c = input[i];
        if (c == ' ' || c == '\n' || i == input.size() - 1)
        {
            if (i == input.size() - 1 && c != ' ')
            {
                word += c; // Add the last character if it's not a space
            }

            // Check if the word exceeds the maxWidth
            if (currentWidth + word.length() > maxWidth)
            {
                if (word.length() > maxWidth)
                {
                    if (currentWidth > 0)
                    {
                        result += ' ';
                        currentWidth++;
                    }

                    if (currentWidth > maxWidth - 4)
                    {
                        result += "\n";
                        currentWidth = 0;
                        currentNbLine++;
                        if (currentNbLine > maxLine)
                        {
                            currentNbLine--;
                            break;
                        }
                    }
                    // If the word itself is longer than maxWidth, we need to break it
                    while (word.length() > maxWidth)
                    {
                        result += word.substr(0, std::max(maxWidth - currentWidth, 0));
                        result += '\n';
                        currentNbLine++;
                        if (currentNbLine > maxLine)
                        {
                            currentNbLine--;
                            *nbLine = currentNbLine;
                            return result; // Stop if we exceed the max number of lines
                        }
                        word = word.substr(std::max(maxWidth - currentWidth, 0));
                    }
                }
                else
                {
                    currentNbLine++;
                    if (currentNbLine > maxLine)
                    {
                        currentNbLine--;
                        break;
                    }
                    // Only add a newline if the result is not empty
                    if (!result.empty())
                    {
                        result += '\n';
                    }
                    currentWidth = 0;
                }
            }
            else if (currentWidth > 0)
            {
                result += ' ';
                currentWidth++;
            }

            result += word;
            currentWidth += word.length();
            word.clear();
        }
        else
        {
            word += c;
        }
    }

    *nbLine = currentNbLine;
    return result;
}

std::string replaceSpecialCharacters(const char *inputStr)
{
    Serial.println("replaceSpecialCharacters");
    String out = "";
    uint8_t i = 0;

    while (inputStr[i] != '\0' && i < MAX_NAME_LENGTH)
    {
        out += inputStr[i];
        i++;
    }
    char myChar;
    myChar = (char)0xE9;
    out.replace("é", String(myChar));
    myChar = (char)0xE8;
    out.replace("è", String(myChar));
    myChar = (char)0xEA;
    out.replace("ê", String(myChar));
    myChar = (char)0xE0;
    out.replace("à", String(myChar));
    myChar = (char)0xE7;
    out.replace("ç", String(myChar));
    myChar = (char)0xFB;
    out.replace("û", String(myChar));
    myChar = (char)0xF4;
    out.replace("ô", String(myChar));
    out.replace("`", "'");
    out.replace("‘", "'");
    out.replace("’", "'");
    out.replace("“", "\"");
    out.replace("”", "\"");
    out.replace("É", "E");
    out.replace("À", "A");
    out.replace("Â", "A");
    out.replace("Ê", "E");
    out.replace("Ô", "O");
    out.replace("Î", "I");
    out.replace("Û", "U");
    out.replace("È", "E");
    out.replace("Ç", "C");

    // print char and hex value
    for (i = 0; i < out.length(); i++)
    {
        Serial.print(out[i]);
        Serial.print(" : ");
        Serial.println((uint8_t)out[i], HEX);
    }

    // // remove emojis
    // i = 0;
    // while (i < out.length())
    // {
    //     if ((uint8_t)out[i] >= 0x80)
    //     {
    //         out.remove(i, 1);
    //     }
    //     else
    //     {
    //         i++;
    //     }
    // }

    std::string outStr = out.c_str();

    return outStr;
}

void displayTaskFunction(void *parameter)
{
    TeDisplayMessage msg;
    struct tm tm;
    uint8_t *taskCnt = (uint8_t *)parameter;
    while (true)
    {
        msg = DISPLAY_MESSAGE_NONE;

        if (xQueueReceive(xQueueDisplay, &(msg), 0) == pdPASS)
        {
            (*taskCnt)++;
            Serial.print("Queue display message received : ");
            switch (msg)
            {
            case DISPLAY_MESSAGE_TIME:
                Serial.println("time");
                if (getLocalTime(&tm))
                {
                    displayTime(&tm);
                }
                break;
            case DISPLAY_MESSAGE_DATE:
                Serial.println("date");
                if (getLocalTime(&tm))
                {
                    displayDate(&tm);
                }
                break;
            case DISPLAY_MESSAGE_POLYLINE:
                Serial.println("polyline");
                if (xSemaphoreTake(polylineMutex, (TickType_t)10))
                {
                    displayStravaPolyline();

                    Serial.println("end DISPLAY_MESSAGE_POLYLINE");
                    xSemaphoreGive(polylineMutex);
                }
                else
                {
                    // if can't get mutex, retry later, so send the message again
                    msg = DISPLAY_MESSAGE_POLYLINE;
                    xQueueSend(xQueueDisplay, &msg, 0);
                }

                break;
            case DISPLAY_MESSAGE_LAST_ACTIVITY:
                Serial.println("last act");
                displayLastActivity();
                break;
            case DISPLAY_MESSAGE_MONTHS:
                Serial.println("month");
                if (getLocalTime(&tm))
                {
                    displayStravaMonths(&tm);
                }
                break;
            case DISPLAY_MESSAGE_WEEKS:
                Serial.println("week");
                if (getLocalTime(&tm))
                {
                    displayStravaWeeks(&tm);
                }
                break;
            case DISPLAY_MESSAGE_TOTAL_YEAR:
                Serial.println("year");
                if (getLocalTime(&tm))
                {
                    displayStravaAllYear(&tm);
                }
                Serial.println("end DISPLAY_MESSAGE_TOTAL_YEAR");
                break;
            case DISPLAY_MESSAGE_REFRESH:
                Serial.println("refresh");
                xQueueReset(xQueueDisplay);
                msg = DISPLAY_MESSAGE_TEMPLATE;
                xQueueSend(xQueueDisplay, &msg, 0);
                msg = DISPLAY_MESSAGE_TIME;
                xQueueSend(xQueueDisplay, &msg, 0);
                msg = DISPLAY_MESSAGE_DATE;
                xQueueSend(xQueueDisplay, &msg, 0);
                msg = DISPLAY_MESSAGE_TOTAL_YEAR;
                xQueueSend(xQueueDisplay, &msg, 0);
                if (getLocalTime(&tm) && tm.tm_min % 2 == 0)
                {
                    msg = DISPLAY_MESSAGE_MONTHS;
                }
                else
                {
                    msg = DISPLAY_MESSAGE_WEEKS;
                }
                xQueueSend(xQueueDisplay, &msg, 0);
                msg = DISPLAY_MESSAGE_LAST_ACTIVITY;
                xQueueSend(xQueueDisplay, &msg, 0);
                msg = DISPLAY_MESSAGE_POLYLINE;
                xQueueSend(xQueueDisplay, &msg, 0);
                hasBeenRefreshed = true;
                break;
            case DISPLAY_MESSAGE_TEMPLATE:
                Serial.println("template");
                displayTemplate();
                break;
            case DISPLAY_MESSAGE_NEW_ACTIVITY:
                Serial.println("new activity");
                msg = DISPLAY_MESSAGE_TOTAL_YEAR;
                xQueueSend(xQueueDisplay, &msg, 0);
                msg = DISPLAY_MESSAGE_LAST_ACTIVITY;
                xQueueSend(xQueueDisplay, &msg, 0);
                msg = DISPLAY_MESSAGE_POLYLINE;
                xQueueSend(xQueueDisplay, &msg, 0);
                break;

            case DISPLAY_MESSAGE_STATUS:
                Serial.println("status");
                displayStatus();
                break;

            case DISPLAY_MESSAGE_TEMPERATURE:
                Serial.println("temperature");
                displayTemperature();
                break;
            case DISPLAY_MESSAGE_SUNSET_SUNRISE:
                Serial.println("sunset sunrise");
                displaySunsetSunrise();
                break;
            default:
                Serial.println("unknown msg");
                break;
            }
            (*taskCnt)--;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(NULL);
}