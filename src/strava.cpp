#include <HTTPClient.h>
#include "ArduinoJson.h"
#include "credentials.h"
#include "time.h"
#include <string.h>
#include <Preferences.h>
#include "strava.h"
#include "Arduino.h"
#include <esp_task_wdt.h>

typedef struct sDistDay
{
    uint32_t distRun;
    uint32_t distBike;
    uint32_t timeRun;
    uint32_t timeBike;
    uint16_t climbRun;
    uint16_t climbBike;
} TsDistDay;

#define WEEK_IN_SECOND 604800U
#define DAYS_BY_YEAR 366
#define THIS_YEAR_OFFSET 366

const uint16_t monthOffset[] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366};

const char activitiesUrl[] = "https://www.strava.com/api/v3/athlete/activities?";
// struct tm tm;
// TsDistDay thisYear[DAYS_BY_YEAR];
// TsDistDay lastYear[DAYS_BY_YEAR];
TsDistDay loopYear[DAYS_BY_YEAR];
uint64_t lastActivitiesId[10];
time_t lastDayPopulate;
struct tm timeinfo;
Preferences preferences;
RTC_DATA_ATTR bool newActivity = true;
RTC_DATA_ATTR TsActivity lastActivity;

void printDateTime(struct tm *dateStruct);
bool getAccessToken(char *ret_token);
int8_t getLastActivitieDist(time_t start, time_t end);
time_t timeStringToTimestamp(char *str);
void timeStringToTm(const char *str, struct tm *tm);
TeActivityType getActivityType(const char *str);
void getYearActivities(time_t start, time_t end);
void printDB(uint16_t nbDays);

void initDB()
{
    preferences.begin("stravaDB", false);
    lastDayPopulate = preferences.getLong("lastTimestamp", 0);
    preferences.getBytes("loopYear", loopYear, sizeof(loopYear));
    preferences.end();
}

bool getAccessToken(char *ret_token)
{
    bool ret = false;
    static time_t expiryTimestamp = 0;
    static char token[45];

    time_t currentTimestamp;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return false;
    }
    time(&currentTimestamp);
    if (currentTimestamp < expiryTimestamp - 60)
    {
        strcpy(ret_token, token);
        ret = true;
    }
    else
    {

        HTTPClient http;
        http.begin(getTokenUrl);

        int httpResponseCode = http.POST("");

        if (httpResponseCode == 200)
        {
            String resp = http.getString();
            // const char *resp = http.getString().c_str();
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, resp.c_str());
            if (error)
            {
                Serial.print("GET ACCESS TOKEN deserializeJson() returned ");
                Serial.println(error.c_str());
            }
            else
            {

                Serial.println("Token OK");
                const char *accessToken = doc["access_token"];
                expiryTimestamp = std::stoi(doc["expires_at"].as<std::string>());
                strcpy(token, accessToken);
                strcpy(ret_token, token);
                ret = true;
            }
        }
        else
        {
            Serial.println("HTTP response " + String(httpResponseCode));
        }
        http.end();
    }

    return ret;
}

int8_t getLastActivitieDist(time_t start, time_t end)
{
    int8_t ret = -1;
    std::string t = std::to_string(start);
    char const *startTimestampStr = t.c_str();
    std::string u = std::to_string(end);
    char const *endTimestampStr = u.c_str();

    char bearerToken[53] = "Bearer ";
    while (!getAccessToken(&bearerToken[7]))
        ;
    Serial.println(bearerToken);

    char fullUrl[100];
    strcpy(fullUrl, activitiesUrl);
    strcat(fullUrl, "after=");
    strcat(fullUrl, startTimestampStr);
    strcat(fullUrl, "&before=");
    strcat(fullUrl, endTimestampStr);
    // strcat(fullUrl, "&per_page=10");
    Serial.println(fullUrl);

    HTTPClient http;
    http.begin(fullUrl);
    http.addHeader("Authorization", bearerToken);

    int httpResponseCode = http.GET();
    Serial.println(httpResponseCode);
    if (httpResponseCode == 200)
    {
        Serial.println("YEAH");
        String resp = http.getString();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, resp.c_str());

        if (error)
        {
            if (error == DeserializationError::EmptyInput)
            {
                Serial.println("no activities");
                ret = 0;
            }
            Serial.print("deserializeJson() returned ");
            Serial.println(error.c_str());
        }
        else
        {
            Serial.println("no error");
            JsonArray array = doc.as<JsonArray>();
            bool isFirst = true;
            time_t prevLastActivityTimestamp = lastDayPopulate;
            for (JsonVariant v : array)
            {
                if (v["trainer"].as<bool>() == true)
                {
                    continue;
                }
                struct tm tm;
                timeStringToTm(v["start_date"].as<const char *>(), &tm);
                TeActivityType activityType = getActivityType(v["type"].as<const char *>());
                time_t activityStartTime = mktime(&tm);
                time_t movingTime = v["moving_time"].as<int>();
                Serial.print("activity start timestamp : ");
                Serial.println(activityStartTime);
                if (activityStartTime >= lastActivity.timestamp)
                {
                    lastActivity.polyline = v["map"]["summary_polyline"].as<std::string>();
                    lastActivity.type = activityType;
                    lastActivity.time = movingTime;
                    lastActivity.deniv = v["total_elevation_gain"].as<int>();
                    lastActivity.dist = (uint16_t)(v["distance"].as<float>() / 10.0);
                    lastActivity.name = v["name"].as<std::string>();
                    if (activityStartTime == lastActivity.timestamp)
                    {
                        continue;
                    }
                    lastActivity.timestamp = activityStartTime;
                }
                if (activityStartTime == lastDayPopulate || prevLastActivityTimestamp == activityStartTime)
                {
                    // dont process previous last activity, it was already accounted
                    continue;
                }

                newActivity = true;
                // int utcOffset = v["utc_offset"].as<int>();
                if (activityStartTime /* + utcOffset */ > lastDayPopulate)
                {
                    lastDayPopulate = activityStartTime /*+ utcOffset*/;
                }
                Serial.println("added to year array");
                uint16_t dayIdx = monthOffset[tm.tm_mon] + tm.tm_mday - 1;
                switch (activityType)
                {
                case ACTIVITY_TYPE_BIKE:

                    loopYear[dayIdx].distBike += (uint32_t)v["distance"].as<float>() * 10.0;
                    loopYear[dayIdx].climbBike += v["total_elevation_gain"].as<int>();
                    loopYear[dayIdx].timeBike += (uint32_t)(v["moving_time"].as<int>());

                    break;
                case ACTIVITY_TYPE_RUN:

                    loopYear[dayIdx].distRun += (uint32_t)(v["distance"].as<float>() * 10.0);
                    loopYear[dayIdx].climbRun += v["total_elevation_gain"].as<int>();
                    loopYear[dayIdx].timeRun += (uint32_t)(v["moving_time"].as<int>());

                    break;
                default:
                    Serial.println("UNKNOWN TYPE");
                    break;
                }
                Serial.print(dayIdx);
                Serial.print(" : ");
                Serial.print(v["name"].as<String>());
                Serial.print(" : ");
                Serial.print(v["distance"].as<float>() / 1000.0, 2);
                Serial.print(" km -> ");
                Serial.println(v["type"].as<const char *>());
            }
            ret = 0;
        }
    }
    else if (httpResponseCode == 429)
    {
        Serial.println("Too Much requests");
        ret = -2;
    }
    else
    {
        Serial.println("OHHH");
    }
    esp_task_wdt_reset();
    http.end();
    return ret;
}

time_t timeStringToTimestamp(char *str)
{
    int Year, Month, Day, Hour, Minute, Second;
    struct tm tm;
    sscanf(str, "%d-%d-%dT%d:%d:%dZ", &Year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
    tm.tm_mon--;
    tm.tm_year = Year - 1900;

    return mktime(&tm);
}

void timeStringToTm(const char *str, struct tm *tm)
{
    int Year, Month, Day, Hour, Minute, Second;

    sscanf(str, "%d-%d-%dT%d:%d:%dZ", &Year, &tm->tm_mon, &tm->tm_mday, &tm->tm_hour, &tm->tm_min, &tm->tm_sec);
    tm->tm_mon--;
    tm->tm_year = Year - 1900;
    mktime(tm);
}

void printDateTime(struct tm *dateStruct)
{
    Serial.print(dateStruct->tm_mday);
    Serial.print("/");
    Serial.print(dateStruct->tm_mon + 1);
    Serial.print("/");
    Serial.print(dateStruct->tm_year + 1900);
    Serial.print(" ");
    Serial.print(dateStruct->tm_hour);
    Serial.print(":");
    Serial.print(dateStruct->tm_min);
    Serial.print(":");
    Serial.println(dateStruct->tm_sec);
    Serial.print("Day of year : ");
    Serial.println(dateStruct->tm_yday);
}

void populateDB(void)
{

    Serial.print("LastDayPopulate at start : ");
    Serial.println(lastDayPopulate);

    // reset everything
    // lastDayPopulate = 0;
    // lastDayPopulate = 1734837250;
    // for (uint16_t i = 356; i < 358; i++)
    // {
    //     loopYear[i].climbRun = 0;
    //     loopYear[i].distRun = 0;
    //     loopYear[i].climbBike = 0;
    //     loopYear[i].distBike = 0;
    // }

    struct tm tmpTm;
    time_t startTimestamp, endTimestamp;

    while (!getLocalTime(&timeinfo))
    {
        Serial.println("getLocalTime Failed");
    }

    // get timestamp of month afer day one year ago
    if (lastDayPopulate == 0)
    {
        tmpTm.tm_year = timeinfo.tm_year - 1;
        tmpTm.tm_mon = timeinfo.tm_mon + 1;
        tmpTm.tm_mday = 1;
        tmpTm.tm_hour = 1;
        tmpTm.tm_min = 1;
        tmpTm.tm_sec = 1;
        startTimestamp = mktime(&tmpTm);
    }
    else
    {
        startTimestamp = lastDayPopulate;
    }
    Serial.print("Start Timestamp : ");
    Serial.println(startTimestamp);

    endTimestamp = mktime(&timeinfo);
    Serial.print("End Timestamp : ");
    Serial.println(endTimestamp);

    getYearActivities(startTimestamp, endTimestamp);
    // save loopYear
    preferences.begin("stravaDB", false);
    preferences.clear();
    preferences.putLong("lastTimestamp", lastDayPopulate);
    preferences.putBytes("loopYear", loopYear, sizeof(loopYear));
    preferences.end();
    Serial.print("lastdaypopulate end : ");
    Serial.println(lastDayPopulate);
}

void getYearActivities(time_t start, time_t end)
{

    time_t tmp = start;
    int8_t ret = -1;

    while (tmp < end)
    {
        if ((tmp + WEEK_IN_SECOND) < end)
        {
            tmp += WEEK_IN_SECOND;
        }
        else
        {
            tmp = end;
        }
        ret = -1;
        while (ret != 0)
        {
            ret = getLastActivitieDist(start, tmp);
            if (ret == -2) // error 429 (too much requests)
            {
                // save arrays, save lastDay populate, exit
                Serial.println("error 429, too much requests, exit");
                return;
            }
        }
        start = tmp + 1;
    }
}

void printDB(uint16_t nbDays)
{
    Serial.println("j -> run  ; bike ");
    for (uint16_t i = monthOffset[11]; i < monthOffset[12]; i++)
    {
        Serial.print(i);
        Serial.print(" -> ");
        Serial.print("loop Year : Run ");
        Serial.print(loopYear[i].distRun);
        Serial.print(" Bike ");
        Serial.print(loopYear[i].distBike);
        Serial.println(" ; ");
    }
}

TeActivityType getActivityType(const char *str)
{
    TeActivityType ret = ACTIVITY_TYPE_UNKNOWN;
    const char strRun[] = "Run";
    const char strBike[] = "Ride";

    if (strcmp(str, strRun) == 0)
    {
        ret = ACTIVITY_TYPE_RUN;
    }
    else if (strcmp(str, strBike) == 0)
    {
        ret = ACTIVITY_TYPE_BIKE;
    }

    return ret;
}

uint32_t getTotal(TeActivityType activityType, TeDataType dataType, uint16_t startDay, uint16_t endDay)
{
    uint32_t ret = 0;
    uint16_t maxValue;

    if (activityType == ACTIVITY_TYPE_BIKE)
    {
        for (uint16_t i = startDay; i < endDay + 1; i++)
        {
            if (dataType == DATA_TYPE_DENIV)
            {
                ret += loopYear[i].climbBike;
            }
            else if (dataType == DATA_TYPE_DISTANCE)
            {
                ret += loopYear[i].distBike;
            }
            else if (dataType == DATA_TYPE_TIME)
            {
                // ret += 0;
                ret += loopYear[i].timeBike;
            }
        }
    }
    else if (activityType == ACTIVITY_TYPE_RUN)
    {
        for (uint16_t i = startDay; i < endDay + 1; i++)
        {
            if (dataType == DATA_TYPE_DENIV)
            {
                ret += loopYear[i].climbRun;
            }
            else if (dataType == DATA_TYPE_DISTANCE)
            {
                ret += loopYear[i].distRun;
            }
            else if (dataType == DATA_TYPE_TIME)
            {
                // ret += 0;
                ret += loopYear[i].timeRun;
            }
        }
    }

    return ret;
}

void newYearBegin()
{
    // copy this year to lastYear
    // memcpy(lastYear, thisYear, sizeof(lastYear));

    // reset this year
    // memset(thisYear, 0, sizeof(thisYear));
}

void newMonthBegin(struct tm tm)
{
    preferences.begin("stravaDB", false);
    preferences.getBytes("loopYear", loopYear, sizeof(loopYear));

    for (uint16_t i = monthOffset[tm.tm_mon]; i < monthOffset[tm.tm_mon + 1]; i++)
    {
        memset(&loopYear[i], 0, sizeof(TsDistDay));
    }
    preferences.clear();
    preferences.putBytes("loopYear", loopYear, sizeof(loopYear));
    preferences.end();
}

TsActivity *getStravaLastActivity()
{
    return &lastActivity;
}