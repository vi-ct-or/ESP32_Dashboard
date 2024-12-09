#define ENABLE_GxEPD2_GFX 0

#include <GxEPD2_BW.h>
#include "time.h"
#include <string>
#include <list>
#include "strava.h"
#include "polyline.h"
#include "logo.h"

#include "displayEpaper.h"

#define SQUARE_SIZE 150
GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(GxEPD2_420_GDEY042T81(/*CS=5*/ 5, /*DC=*/4, /*RES=*/16, /*BUSY=*/15)); // 400x300, SSD1683

int getMaxLat();
int getMaxLng();
int getMinLat();
int getMinLng();
void drawDateStr(const void *pv);
void drawTimeStr(const void *pv);
void drawYearStr(const void *pv);
void drawStravaPolyline(const void *pv);
void drawFull(const void *pv);
void drawMonth(const void *pv);
void drawLastActivity(const void *pv);
void timestampToHour(time_t timestamp, std::string *out);

void initDisplay(void)
{
    // init display
    display.init(9600, true, 50, false);
    display.setRotation(1);
    display.setTextSize(2);
    // display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.drawPaged(drawFull, 0);
}

void displayTime(struct tm *now)
{
    display.drawPaged(drawTimeStr, (const void *)now);
    display.hibernate();
}
void displayDate(struct tm *now)
{
    display.drawPaged(drawDateStr, (const void *)now);
    display.hibernate();
}

void displayStravaAllYear()
{
    display.drawPaged(drawYearStr, 0);
    display.hibernate();
}

void displayStravaThisMonth(struct tm *now)
{
    display.drawPaged(drawMonth, (const void *)now);
    display.hibernate();
}
void displayLastActivity()
{
    display.drawPaged(drawLastActivity, 0);
    display.hibernate();
}

void displayStravaYTD()
{
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    display.setCursor(0, 20);
    display.println("2024    YTD   2023\n");
    display.print((int)getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DISTANCE, 1, 0, timeinfo.tm_yday));
    display.print("km        ");
    display.print((int)getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DISTANCE, 0, 0, timeinfo.tm_yday));
    display.println("km");
    // display.print(getTotal(ACTIVITY_TYPE_RUN, true, 1, true));
    // display.println("m d+");
    display.print((int)getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DISTANCE, 1, 0, timeinfo.tm_yday));
    display.print("km       ");
    display.print((int)getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DISTANCE, 0, 0, timeinfo.tm_yday));
    display.println("km");
    // display.print(getTotal(ACTIVITY_TYPE_BIKE, true, 1, true));
    // display.println("m d+");
    display.display();
}

void displayStravaCurrentWeek()
{
    struct tm timeinfo;
    getLocalTime(&timeinfo);

    uint16_t startDay = timeinfo.tm_yday - (timeinfo.tm_wday + 6) % 7;
    uint16_t endDay = timeinfo.tm_yday;
    display.setCursor(0, 20);
    display.println("This week\n");
    display.print(getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DISTANCE, 1, startDay, endDay));
    display.print("km   ");
    display.print((int)getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DENIV, 1, startDay, endDay));
    display.println("m d+");
    display.print(getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DISTANCE, 1, startDay, endDay));
    display.print("km   ");
    display.print((int)getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DENIV, 1, startDay, endDay));
    display.println("m d+");
    display.display();
}

void displayStravaToday()
{
    struct tm timeinfo;
    getLocalTime(&timeinfo);

    uint16_t startDay = timeinfo.tm_yday;
    uint16_t endDay = timeinfo.tm_yday;
    float distBike = getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DISTANCE, 1, startDay, endDay);
    float distRun = getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DISTANCE, 1, startDay, endDay);

    display.setCursor(0, 20);
    display.println("Today\n");
    if (distRun > 0.1)
    {
        display.print(distRun);
        display.print("km   ");
        display.print((int)getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DENIV, 1, startDay, endDay));
        display.println("m d+");
    }
    if (distBike > 0.1)
    {
        display.print(distBike);
        display.print("km   ");
        display.print((int)getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DENIV, 1, startDay, endDay));
        display.println("m d+");
    }
    display.display();
}

void displayText(const char msg[])
{
    display.println(msg);
    display.display();
}

void displayStravaPolyline()
{
    display.drawPaged(drawStravaPolyline, 0);
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
    display.setPartialWindow(x1, y1, w, h);
    display.setCursor(x, y);
    display.print(text);
}

void drawTimeStr(const void *pv)
{
    const struct tm *now = (const struct tm *)pv;
    std::string timeStr = std::to_string(now->tm_hour) + ":";
    display.setTextSize(6);
    if (now->tm_min < 10)
    {
        timeStr += "0";
    }
    timeStr += std::to_string(now->tm_min);

    drawText(120, 5, timeStr.c_str());
}

void drawDateStr(const void *pv)
{
    const struct tm *now = (const struct tm *)pv;
    display.setTextSize(2);
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
        month = "Fevrier";
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
        month = "Aout";
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
        month = "Decembre";
        break;

    default:
        break;
    }

    uint8_t maxLen = max(day.size(), month.size());
    if (day.size() < month.size())
    {
        day.insert(0, (month.size() - day.size()) / 2, ' ');
        day.insert(day.size(), (month.size() - day.size()) / 2, ' ');
    }
    else if (day.size() > month.size())
    {
        month.insert(0, (day.size() - month.size()) / 2, ' ');
        month.insert(month.size(), (day.size() - month.size()) / 2, ' ');
    }

    nb = std::to_string(now->tm_mday);
    if (now->tm_mday < 10)
    {
        nb.insert(0, 1, '0');
    }
    nb.insert(0, (maxLen - 2) / 2, ' ');
    nb.insert(nb.size(), (maxLen - 2) / 2, ' ');

    dateStr = day;
    dateStr += "\n";
    dateStr += nb;
    dateStr += "\n";
    dateStr += month;
    drawText(1, 1, dateStr.c_str());
}

void drawYearStr(const void *pv)
{
    display.setTextSize(2);
    std::string yearStravaBike, yearStravaRun, dist, time, deniv;

    // bike
    dist = std::to_string((int)getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DISTANCE, 1, 0, DAYS_BY_YEAR - 1));
    dist.insert(0, 5 - dist.size(), ' ');
    time = "000";
    time.insert(0, 3 - time.size(), ' ');
    deniv = std::to_string((int)getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DENIV, 1, 0, DAYS_BY_YEAR - 1));
    deniv.insert(0, 6 - deniv.size(), ' ');
    yearStravaBike = dist;
    yearStravaBike += "km ";
    yearStravaBike += time;
    yearStravaBike += "h ";
    yearStravaBike += deniv;
    yearStravaBike += "m";

    // run
    dist = std::to_string((int)getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DISTANCE, 1, 0, DAYS_BY_YEAR - 1));
    dist.insert(0, 5 - dist.size(), ' ');
    time = "000";
    time.insert(0, 3 - time.size(), ' ');
    deniv = std::to_string((int)getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DENIV, 1, 0, DAYS_BY_YEAR - 1));
    deniv.insert(0, 6 - deniv.size(), ' ');
    yearStravaRun = dist;
    yearStravaRun += "km ";
    yearStravaRun += time;
    yearStravaRun += "h ";
    yearStravaRun += deniv;
    yearStravaRun += "m";

    int16_t x1b, y1b, x1r, y1r, x1, y1;
    uint16_t wb, hb, wr, hr, w, h;
    display.getTextBounds(yearStravaBike.c_str(), 50, 105, &x1b, &y1b, &wb, &hb);
    display.getTextBounds(yearStravaRun.c_str(), 50, 135, &x1r, &y1r, &wr, &hr);
    x1 = x1b; // bike is above run
    y1 = y1b; // bike is above run
    w = max(wb, wr);
    h = y1r + hr - y1b; // bike is above run
    display.setPartialWindow(x1, y1, w, h);
    display.setCursor(50, 105);
    display.print(yearStravaBike.c_str());
    display.setCursor(50, 135);
    display.print(yearStravaRun.c_str());
}

void drawFull(const void *pv)
{
    display.setFullWindow();
    display.setCursor(1, 70);
    display.setTextSize(2);
    display.print("Cette annee :\n");
    display.setTextSize(1);
    display.setCursor(55, 90);
    display.print("Distance       Temps       Denivele");
    display.drawBitmap(0, 100, bicycleBitMap, 42, 28, GxEPD_BLACK);
    display.drawBitmap(0, 100 + 30, runningShoeBitmap, 42, 28, GxEPD_BLACK);
}

void drawStravaPolyline(const void *pv)
{
    display.setPartialWindow(150, 250, SQUARE_SIZE, SQUARE_SIZE);
    TsActivity *lastAct = getStravaLastActivity();
    if (!lastAct->polyline.empty())
    {
        decode(lastAct->polyline.c_str(), lastAct->polyline.size());
        int maxLat = getMaxLat();
        int maxLng = getMaxLng();
        int minLat = getMinLat();
        int minLng = getMinLng();

        int maxDiff = max(maxLat - minLat, maxLng - minLng);
        int x, y, prevx = -1, prevy = -1;
        for (std::list<TsCoordinates>::iterator it = coordList.begin(); it != coordList.end(); ++it)
        {
            x = 150 + (int)(((float)((it->lng - minLng) * SQUARE_SIZE)) / (float)maxDiff);
            y = 249 + SQUARE_SIZE - (int)(((float)((it->lat - minLat) * SQUARE_SIZE)) / (float)maxDiff);
            // display.drawPixel(x, y, GxEPD_BLACK);
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
    }
}

void drawMonth(const void *pv)
{
    // display.setPartialWindow(1, 150, 300, 100);
    uint16_t yearMon[12] = {0};
    const struct tm *now = (const struct tm *)pv;
    uint8_t monNb = 0;
    struct tm tmp;
    tmp.tm_year = now->tm_year;
    tmp.tm_hour = 8;

    for (uint8_t i = 0; i < now->tm_mon + 1; i++)
    {
        uint16_t startMonDay, endMonDay, dist;
        tmp.tm_mon = i;
        tmp.tm_mday = 1;
        mktime(&tmp);
        startMonDay = tmp.tm_yday - 1;
        if (i == 0 || i == 2 || i == 4 || i == 6 || i == 7 || i == 9 || i == 11)
        {
            tmp.tm_mday = 31;
        }
        else if (i == 3 || i == 5 || i == 8 || i == 10)
        {
            tmp.tm_mday = 30;
        }
        else
        {                                                                          // fevrier
            if ((tmp.tm_year + 1900) % 4 == 0 and (tmp.tm_year + 1900) % 100 != 0) // bissextile
            {
                tmp.tm_mday = 29;
            }
            else
            {
                tmp.tm_mday = 28;
            }
        }
        mktime(&tmp);
        endMonDay = tmp.tm_yday - 1;
        dist = (int)getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DISTANCE, 1, startMonDay, endMonDay);
        dist += (int)getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DISTANCE, 1, startMonDay, endMonDay);
        yearMon[i] = dist;
    }
}
void drawLastActivity(const void *pv)
{
    TsActivity *lastActivity = getStravaLastActivity();
    std::string displStr, dist;
    displStr = lastActivity->name;
    displStr += "\n";
    dist = std::to_string((float)lastActivity->dist / 100.0);
    uint8_t dotIdx = dist.find('.');
    dist.resize(dotIdx + 3);
    displStr += dist;
    displStr += "km\n";
    timestampToHour(lastActivity->time, &displStr);
    displStr += std::to_string(lastActivity->deniv);
    displStr += "m d+";
    display.setTextSize(2);
    drawText(0, 250, displStr.c_str());
}

void timestampToHour(time_t timestamp, std::string *out)
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