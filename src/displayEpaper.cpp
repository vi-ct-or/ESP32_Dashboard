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

RTC_DATA_ATTR bool refresh = true;
RTC_DATA_ATTR bool isRefreshed = false;
RTC_DATA_ATTR bool prevGPSSync = false;
RTC_DATA_ATTR bool firstTime = true;

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
        isRefreshed = true;
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

void displayStravaAllYear(struct tm *now)
{
    display.drawPaged(drawYearTitle, (const void *)now);
    // display.drawPaged(drawYearStr, 0);
    display.drawPaged(drawYearDistance, 0);
    display.drawPaged(drawYearTime, 0);
    display.drawPaged(drawYearDeniv, 0);
    display.hibernate();
}

void displayStravaMonths(struct tm *now)
{
    display.drawPaged(drawLastTwelveMonths, (const void *)now);
    display.hibernate();
}

void displayStravaWeeks(struct tm *now)
{
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

    drawText(120, 10, timeStr.c_str());
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
    drawText(1, 5, dateStr.c_str());
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
    // yearStravaBike += "km ";
    // yearStravaBike += time;
    // yearStravaBike += "h ";
    // yearStravaBike += deniv;
    // yearStravaBike += "m";
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
    // display.getTextBounds(deniv.c_str(), 220, 105, &x, &y, &w, &h);
    // display.setPartialWindow(x, y, w, h);
    // display.print(deniv.c_str());

    // run
    dist = std::to_string(getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DISTANCE, 0, currentDay) / 10000);
    dist.insert(0, 5 - dist.size(), ' ');
    // time = time = std::to_string(getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_TIME, 0, currentDay) / 3600);
    // time.insert(0, 3 - time.size(), ' ');
    // deniv = std::to_string(getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DENIV, 0, currentDay));
    // deniv.insert(0, 6 - deniv.size(), ' ');
    yearStravaRun = dist;
    // yearStravaRun += "km ";
    // yearStravaRun += time;
    // yearStravaRun += "h ";
    // yearStravaRun += deniv;
    // yearStravaRun += "m";

    // int16_t x1b, y1b, x1r, y1r, x1, y1;
    // uint16_t wb, hb, wr, hr, w, h;
    // display.getTextBounds(yearStravaBike.c_str(), 50, 105, &x1b, &y1b, &wb, &hb);
    // display.getTextBounds(yearStravaRun.c_str(), 50, 135, &x1r, &y1r, &wr, &hr);
    // x1 = x1b; // bike is above run
    // y1 = y1b; // bike is above run
    // w = max(wb, wr);
    // h = y1r + hr - y1b; // bike is above run
    // display.setPartialWindow(x1, y1, w, h);
    // display.setCursor(50, 105);
    // display.print(yearStravaBike.c_str());
    // display.setCursor(50, 135);
    // display.print(yearStravaRun.c_str());
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
    display.setPartialWindow(0, 60, 100, 23);
    display.setCursor(10, 70);
    display.setTextSize(2);
    display.print(std::to_string(now->tm_year + 1900).c_str());
}

void drawFull(const void *pv)
{
    display.setFullWindow();
    display.setTextSize(1);
    display.setCursor(55, 90);
    display.print("   Distance      Temps        Denivele");
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
    display.setPartialWindow(150, 250, SQUARE_SIZE, SQUARE_SIZE);
    TsActivity *lastAct = getStravaLastActivity();

    if (lastAct == NULL || lastAct->isFilled == false)
    {
        Serial.println("lastAct NULL");
        return;
    }
    Serial.println("lastAct not NULL");
    if (!lastAct->polyline.empty())
    {
        decode(lastAct->polyline.c_str(), lastAct->polyline.size());
        int maxLat = getMaxLat();
        int maxLng = getMaxLng();
        int minLat = getMinLat();
        int minLng = getMinLng();

        int maxDiff = max(maxLat - minLat, maxLng - minLng);
        int minDiff = min(maxLat - minLat, maxLng - minLng);
        int offsetV = 0;
        int offsetH = 0;
        int pixelLargeur;
        pixelLargeur = minDiff * SQUARE_SIZE / maxDiff;

        if (minDiff == maxLat - minLat)
        {
            // only apply to y
            offsetV = (SQUARE_SIZE - pixelLargeur) / 2;
            // offsetV = (SQUARE_SIZE - (int)(((float)((maxLat - minLat) * SQUARE_SIZE)) / (float)maxDiff) - 1) / 2;
        }
        else
        {
            // only apply to x
            offsetH = (SQUARE_SIZE - pixelLargeur) / 2;
            // offsetH = (int)(((float)((maxLng - minLng) * SQUARE_SIZE)) / (float)maxDiff) / 2;
        }

        Serial.print("offsetV = ");
        Serial.println(offsetV);
        Serial.print("offsetH = ");
        Serial.println(offsetH);
        int x, y, prevx = -1, prevy = -1;
        display.drawRect(150, 249, SQUARE_SIZE, SQUARE_SIZE + 1, GxEPD_BLACK);
        for (std::list<TsCoordinates>::iterator it = coordList.begin(); it != coordList.end(); ++it)
        {
            x = 150 + (int)(((float)((it->lng - minLng) * SQUARE_SIZE)) / (float)maxDiff) + offsetH;
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
    }
}

void drawLastActivity(const void *pv)
{
    TsActivity *lastActivity = getStravaLastActivity();
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

    name = addNewLines(lastActivity->name, 11, 3, &lineNbTitle);
    // name = lastActivity->name; // name
    //  if (name.size() > 10)
    //  {
    //      name.insert(10, 1, '\n');
    //      lineNbTitle = 2;
    //      if (name[11] == ' ')
    //      {
    //          name.erase(11, 1);
    //      }
    //  }
    //  if (name.size() > 20)
    //  {
    //      name.insert(20, 1, '\n');
    //      lineNbTitle = 3;
    //      if (name[21] == ' ')
    //      {
    //          name.erase(21, 1);
    //      }
    //  }
    //  if (name.size() > 30)
    //  {
    //      name.resize(30);
    //  }

    dist = std::to_string((float)lastActivity->dist / 100.0); // dist
    uint8_t dotIdx = dist.find('.');
    dist.resize(dotIdx + 3);
    // dist += "km";
    dist.insert(0, 8 - dist.size(), ' ');
    secondsToHour(lastActivity->time, &duration); // duration
    duration.insert(0, 8 - duration.size(), ' ');
    deniv = std::to_string(lastActivity->deniv); // deniv
    // deniv += "m d+";
    deniv.insert(0, 8 - deniv.size(), ' ');

    speed = ((float)(((float)lastActivity->dist / 100.0)) / (float)lastActivity->time * 3600.0);

    display.setPartialWindow(1, 259, 149, 140);
    display.setTextSize(1);
    if (lastActivity->type == ACTIVITY_TYPE_RUN)
    {
        speedOrPace = speedToPace(speed);
        // speedOrPace += "min/km";
        display.setCursor(86, 267 + (lineNbTitle + 4) * heightLetter2);
        display.print("min/km");
        speedOrPace.insert(0, 7 - speedOrPace.size(), ' ');
    }
    else
    {
        speedOrPace = std::to_string(speed);
        dotIdx = speedOrPace.find('.');
        speedOrPace.resize(dotIdx + 3);
        // speedOrPace += "km/h";
        display.setCursor(98, 267 + (lineNbTitle + 4) * heightLetter2);
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
    display.print(displStr.c_str());
    // for (uint16_t i = 259; i < 400; i = i + heightLetter2)
    // {
    //     display.drawLine(0, i, 140, i, GxEPD_BLACK);
    // }

    display.setTextSize(1);
    display.setCursor(98, 267 + (lineNbTitle + 1) * heightLetter2);
    display.print("km");
    display.setCursor(98, 267 + (lineNbTitle + 3) * heightLetter2);
    display.print("m d+");

    display.setCursor(83, 267);
    // display.print("test");

    if (lastActivity->kudos > 0)
    {
        display.drawBitmap(1, 400 - 18, myKudosBitmap, 17, 17, GxEPD_BLACK);
        display.setTextSize(1);
        display.setCursor(22, 400 - 10);
        display.print(lastActivity->kudos);
    }

    // drawText(1, 260, displStr.c_str());
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
        display.drawRect(x, y, w, -(weeks[i] * hMax / maxWeek), GxEPD_BLACK);

        tmTmp = *localtime(&timeTmp);
        getYearAndWeek(tmTmp, year, currentWeek);
        // display.setCursor(x, y + 2 + (i % 3) * 5);
        if ((currentWeek % 5 == 0 || currentWeek == 1) && x < 300 - 15)
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

            if (currentWidth + word.length() > maxWidth)
            {
                currentNbLine++;
                if (currentNbLine > maxLine)
                {
                    currentNbLine--;
                    break;
                }
                result += '\n';
                currentWidth = 0;
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
