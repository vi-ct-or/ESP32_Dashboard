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
void drawLastTwelveMonths(const void *pv);
void drawLastActivity(const void *pv);
void secondsToHour(time_t timestamp, std::string *out);
void printSTDString(std::string str);
void drawUpdating(const void *pv);

RTC_DATA_ATTR bool refresh = true;

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
    if (refresh)
    {
        display.drawPaged(drawFull, 0);
        refresh = false;
    }
    display.hibernate();
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

void displayStravaMonths(struct tm *now)
{
    display.drawPaged(drawLastTwelveMonths, (const void *)now);
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

void displayUpdating(uint8_t state)
{
    if (state == 0)
    {
        display.clearScreen();
    }
    display.drawPaged(drawUpdating, (void *)&state);
    display.hibernate();
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
    display.setTextSize(6);
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
    struct tm tm;
    getLocalTime(&tm);
    uint16_t currentDay = monthOffset[tm.tm_mon] + tm.tm_mday - 1;
    // bike
    dist = std::to_string(getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DISTANCE, 1, 0, currentDay) / 10000);
    dist.insert(0, 5 - dist.size(), ' ');
    time = std::to_string(getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_TIME, 1, 0, currentDay) / 3600);
    time.insert(0, 3 - time.size(), ' ');
    deniv = std::to_string(getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DENIV, 1, 0, currentDay));
    deniv.insert(0, 6 - deniv.size(), ' ');
    yearStravaBike = dist;
    yearStravaBike += "km ";
    yearStravaBike += time;
    yearStravaBike += "h ";
    yearStravaBike += deniv;
    yearStravaBike += "m";

    // run
    dist = std::to_string(getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DISTANCE, 1, 0, currentDay) / 10000);
    dist.insert(0, 5 - dist.size(), ' ');
    time = time = std::to_string(getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_TIME, 1, 0, currentDay) / 3600);
    time.insert(0, 3 - time.size(), ' ');
    deniv = std::to_string(getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DENIV, 1, 0, currentDay));
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
        int offsetV = (SQUARE_SIZE - (int)(((float)((maxLat - minLat) * SQUARE_SIZE)) / (float)maxDiff) - 1) / 2;
        int offsetH = (int)(((float)((maxLng - minLng) * SQUARE_SIZE)) / (float)maxDiff) / 2;
        int x, y, prevx = -1, prevy = -1;
        //      display.drawRect(150, 250, SQUARE_SIZE, SQUARE_SIZE, GxEPD_BLACK);
        for (std::list<TsCoordinates>::iterator it = coordList.begin(); it != coordList.end(); ++it)
        {
            x = 150 + (int)(((float)((it->lng - minLng) * SQUARE_SIZE)) / (float)maxDiff) /*+ offsetH*/;
            y = 249 + SQUARE_SIZE - (int)(((float)((it->lat - minLat) * SQUARE_SIZE)) / (float)maxDiff) /*- offsetV*/;
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

void drawLastActivity(const void *pv)
{
    TsActivity *lastActivity = getStravaLastActivity();
    std::string displStr, dist, name, time, duration, deniv, speed;
    name = lastActivity->name; // name
    if (name.size() > 10)
    {
        name.insert(10, 1, '\n');

        if (name[11] == ' ')
        {
            name.erase(11, 1);
        }
    }
    if (name.size() > 20)
    {
        name.insert(20, 1, '\n');

        if (name[21] == ' ')
        {
            name.erase(21, 1);
        }
    }
    if (name.size() > 30)
    {
        name.resize(30);
    }
    dist = std::to_string((float)lastActivity->dist / 100.0); // dist
    uint8_t dotIdx = dist.find('.');
    dist.resize(dotIdx + 3);
    dist += "km";
    dist.insert(0, 10 - dist.size(), ' ');
    secondsToHour(lastActivity->time, &duration); // duration
    duration.insert(0, 10 - duration.size(), ' ');
    deniv = std::to_string(lastActivity->deniv); // deniv
    deniv += "m d+";
    deniv.insert(0, 10 - deniv.size(), ' ');

    speed = std::to_string(((float)(((float)lastActivity->dist / 100.0)) / (float)lastActivity->time * 3600.0));
    dotIdx = speed.find('.');
    speed.resize(dotIdx + 3);
    speed += "km/h";
    speed.insert(0, 10 - speed.size(), ' ');

    displStr = name;
    displStr += "\n\n";
    displStr += dist;
    displStr += "\n";
    displStr += duration;
    displStr += "\n";
    displStr += deniv;
    displStr += "\n";
    displStr += speed;
    display.setTextSize(2);
    drawText(0, 260, displStr.c_str());
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
    uint16_t maxMonth = 0;

    for (uint8_t i = 0; i < 12; i++)
    {
        uint16_t startMonDay, endMonDay;
        uint32_t dist;
        tmp.tm_mon = (now->tm_mon + 1 + i) % 12;
        if (tmp.tm_mon == 0 && i != 0)
        {
            tmp.tm_year++;
        }
        tmp.tm_mday = 1;
        mktime(&tmp);
        // Serial.print("start : ");
        // Serial.println(tmp.tm_yday);
        startMonDay = monthOffset[tmp.tm_mon];
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
        // Serial.print("endday : ");
        // Serial.println(tmp.tm_yday);
        endMonDay = monthOffset[tmp.tm_mon + 1] - 1;
        bool isCurrentYear = tmp.tm_year == now->tm_year;
        Serial.print(startMonDay);
        Serial.print(" - ");
        Serial.println(endMonDay);
        dist = getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DISTANCE, isCurrentYear, startMonDay, endMonDay);
        dist += getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DISTANCE, isCurrentYear, startMonDay, endMonDay);
        yearMon[i] = dist / 10000;
        Serial.println(dist);
        if (dist / 10000 > maxMonth)
        {
            maxMonth = dist / 10000;
        }
    }

    uint16_t x = 50, w = 18, h, space = 3, hMax = 68, y = 235;

    display.setPartialWindow(0, y - hMax, 300, hMax + 12);
    for (uint8_t i = 0; i < 12; i++)
    {
        display.drawRect(x, y, w, -(yearMon[i] * hMax / maxMonth), GxEPD_BLACK);
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
        Serial.print(monthLetter);
        Serial.print(" : ");
        Serial.println(yearMon[i]);
        x += w + space;
    }

    display.drawLine(30, y - hMax - 1, 300, y - hMax - 1, GxEPD_BLACK);
    display.drawLine(30, y - hMax / 2, 300, y - hMax / 2, GxEPD_BLACK);
    display.drawLine(30, y, 300, y, GxEPD_BLACK);
    display.setCursor(0, y - 3 - hMax);
    display.print(maxMonth);
    display.setCursor(0, y - 3 - hMax / 2);
    display.print(maxMonth / 2);
    display.setCursor(0, y - 3);
    display.print(0);
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