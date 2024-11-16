#include <HTTPClient.h>
#include "ArduinoJson.h"
#include "credentials.h"
#include "time.h"
#include <string.h>
#include <Preferences.h>

typedef struct sDistDay
{
    uint16_t distRun;
    uint16_t distBike;
    // uint32_t timeRun;
    // uint32_t timeBike;
    uint16_t climbRun;
    uint16_t climbBike;
} TsDistDay;

typedef enum eActivityType
{
    ACTIVITY_TYPE_UNKNOWN,
    ACTIVITY_TYPE_RUN,
    ACTIVITY_TYPE_BIKE,
} TeActivityType;

#define WEEK_IN_SECOND 604800U
#define DAYS_BY_YEAR 366
#define THIS_YEAR_OFFSET 366

const char activitiesUrl[] = "https://www.strava.com/api/v3/athlete/activities?";
// struct tm tm;
TsDistDay thisYear[DAYS_BY_YEAR];
TsDistDay lastYear[DAYS_BY_YEAR];
time_t lastDayPopulate;
struct tm timeinfo;
Preferences preferences;

void printDateTime(struct tm *dateStruct);
bool getAccessToken(char *ret_token);
int8_t getLastActivitieDist(time_t start, time_t end);
time_t timeStringToTimestamp(char *str);
void timeStringToTm(const char *str, struct tm *tm);
TeActivityType getActivityType(const char *str);
float getTotal(TeActivityType activityType, bool dataType, bool year, bool allRound);
void getYearActivities(time_t start, time_t end);
void printDB(uint16_t nbDays);

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
            const char *resp = http.getString().c_str();
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, resp);
            if (error)
            {
                Serial.print("GET ACCESS TOKEN deserializeJson() returned ");
                Serial.println(error.c_str());
                Serial.println(resp);
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
        const char *resp = http.getString().c_str();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, resp);
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
            for (JsonVariant v : array)
            {
                struct tm tm;
                timeStringToTm(v["start_date"].as<const char *>(), &tm);
                TeActivityType activityType = getActivityType(v["type"].as<const char *>());
                time_t activityStartTime = mktime(&tm);
                time_t elapsedTime = v["elapsed_time"].as<int>();
                Serial.print("activity start timestamp : ");
                Serial.println(activityStartTime);
                if (activityStartTime + elapsedTime > lastDayPopulate)
                {
                    lastDayPopulate = activityStartTime + elapsedTime + 1;
                }

                switch (activityType)
                {
                case ACTIVITY_TYPE_BIKE:
                    if (tm.tm_year == timeinfo.tm_year)
                    {
                        thisYear[tm.tm_yday].distBike += (uint16_t)(v["distance"].as<float>() / 10.0);
                        thisYear[tm.tm_yday].climbBike += v["total_elevation_gain"].as<int>();
                    }
                    else
                    {
                        lastYear[tm.tm_yday].distBike += (uint16_t)(v["distance"].as<float>() / 10.0);
                        lastYear[tm.tm_yday].climbBike += v["total_elevation_gain"].as<int>();
                    }
                    break;
                case ACTIVITY_TYPE_RUN:
                    if (tm.tm_year == timeinfo.tm_year)
                    {
                        thisYear[tm.tm_yday].distRun += (uint16_t)(v["distance"].as<float>() / 10.0);
                        thisYear[tm.tm_yday].climbRun += v["total_elevation_gain"].as<int>();
                    }
                    else
                    {
                        lastYear[tm.tm_yday].distRun += (uint16_t)(v["distance"].as<float>() / 10.0);
                        lastYear[tm.tm_yday].climbRun += v["total_elevation_gain"].as<int>();
                    }
                    break;
                default:
                    Serial.println("UNKNOWN TYPE");
                    break;
                }

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
    preferences.begin("stravaDB", false);

    lastDayPopulate = preferences.getLong("lastTimestamp", 0);
    size_t size;
    size = preferences.getBytes("lastYear", lastYear, sizeof(lastYear));
    if (size == 0)
    {
        memset(lastYear, 0, sizeof(lastYear));
    }
    size = preferences.getBytes("thisYear", thisYear, sizeof(thisYear));
    if (size == 0)
    {
        memset(thisYear, 0, sizeof(thisYear));
    }
    // reset everything
    // memset(lastYear, 0, sizeof(lastYear));
    // memset(thisYear, 0, sizeof(thisYear));
    // lastDayPopulate = 0;
    // lastDayPopulate = 1731742497;
    // thisYear[320].climbBike = 0;
    // thisYear[320].distBike = 0;

    struct tm tmpTm;
    time_t startTimestamp, endTimestamp;

    while (!getLocalTime(&timeinfo))
    {
        Serial.println("getLocalTime Failed");
    }

    // get all last year activities
    char firstDayOfYear[] = "2024-01-01T01:01:00Z";
    // get timestamp of first day of last year
    if (lastDayPopulate == 0)
    {
        timeStringToTm(firstDayOfYear, &tmpTm);
        tmpTm.tm_year = timeinfo.tm_year - 1;
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

    Serial.println(lastDayPopulate);
    preferences.end();
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
                preferences.putLong("lastTimestamp", lastDayPopulate);
                preferences.putBytes("lastYear", lastYear, sizeof(lastYear));
                preferences.putBytes("thisYear", thisYear, sizeof(thisYear));
                return;
            }
        }
        start = tmp + 1;
    }
    // save arrays, save lastDay populate, exit
    preferences.putLong("lastTimestamp", lastDayPopulate);
    preferences.putBytes("lastYear", lastYear, sizeof(lastYear));
    preferences.putBytes("thisYear", thisYear, sizeof(thisYear));
}

void printDB(uint16_t nbDays)
{
    Serial.println("j -> run  ; bike ");
    for (uint16_t i = 0; i < nbDays; i++)
    {
        Serial.print(i);
        Serial.print(" -> ");
        Serial.print("Last Year : Run ");
        Serial.print(lastYear[i].distRun);
        Serial.print(" Bike ");
        Serial.print(lastYear[i].distBike);
        Serial.print(" ; ");
        Serial.print("ThisYear : Run ");
        Serial.print(thisYear[i].distRun);
        Serial.print(" Bike ");
        Serial.println(thisYear[i].distBike);
    }
}

void test()
{
    Serial.println("start test");
    populateDB();
    // printDB(DAYS_BY_YEAR);
    Serial.println("THIS YEAR :");
    Serial.print("Total run : ");
    Serial.print(getTotal(ACTIVITY_TYPE_RUN, false, 1, true));
    Serial.print("km; ");
    Serial.print(getTotal(ACTIVITY_TYPE_RUN, true, 1, true));
    Serial.println("m d+");
    Serial.print("Total bike : ");
    Serial.print(getTotal(ACTIVITY_TYPE_BIKE, false, 1, true));
    Serial.print("km; ");
    Serial.print(getTotal(ACTIVITY_TYPE_BIKE, true, 1, true));
    Serial.println("m d+");

    Serial.println("\nLAST YEAR :");
    Serial.print("Total run : ");
    Serial.println(getTotal(ACTIVITY_TYPE_RUN, false, 0, true));
    Serial.print("Total bike : ");
    Serial.println(getTotal(ACTIVITY_TYPE_BIKE, false, 0, true));

    Serial.println("\nLAST YEAR ytd :");
    Serial.print("Total run : ");
    Serial.println(getTotal(ACTIVITY_TYPE_RUN, false, 0, false));
    Serial.print("Total bike : ");
    Serial.println(getTotal(ACTIVITY_TYPE_BIKE, false, 0, false));

    Serial.println("end test");
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

float getTotal(TeActivityType activityType, bool dataType, bool year, bool allRound)
{
    float ret = 0.0;
    uint16_t maxValue;
    TsDistDay *arr;
    if (year == true)
    { // this year
        arr = thisYear;
    }
    else
    {
        arr = lastYear;
    }
    if (allRound == false)
    {
        getLocalTime(&timeinfo);
        maxValue = timeinfo.tm_yday;
    }
    else
    {
        maxValue = DAYS_BY_YEAR;
    }
    if (activityType == ACTIVITY_TYPE_BIKE)
    {
        for (uint16_t i = 0; i < maxValue; i++)
        {
            if (dataType)
            {
                ret += (int)arr[i].climbBike;
            }
            else
            {
                ret += (float)arr[i].distBike / 100.0;
            }
        }
    }
    else if (activityType == ACTIVITY_TYPE_RUN)
    {
        for (uint16_t i = 0; i < maxValue; i++)
        {
            if (dataType)
            {
                ret += (int)arr[i].climbRun;
            }
            else
            {
                ret += (float)arr[i].distRun / 100.0;
            }
        }
    }

    return ret;
}