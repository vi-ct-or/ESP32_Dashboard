#include <HTTPClient.h>
#include "ArduinoJson.h"
#include "credentials.h"
#include "time.h"
#include <string.h>
#include <Preferences.h>
#include "strava.h"
#include "Arduino.h"
#include "network.h"
#include "displayEpaper.h"
#include "dataSave.h"
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
#define TWO_WEEK_IN_SECOND 2 * WEEK_IN_SECOND
#define DAYS_BY_YEAR 366
#define THIS_YEAR_OFFSET 366

const uint16_t monthOffset[] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366};

const char activitiesUrl[] = "https://www.strava.com/api/v3/athlete/activities?";
// struct tm tm;
// TsDistDay thisYear[DAYS_BY_YEAR];
// TsDistDay lastYear[DAYS_BY_YEAR];
TsDistDay loopYear[DAYS_BY_YEAR];
uint64_t lastActivitiesId[NB_LAST_ACTIVITIES];
uint64_t tmpLastActivitiesId[NB_LAST_ACTIVITIES];
time_t lastDayPopulate;
uint64_t lastActivityId;
struct tm timeinfo;
Preferences preferences;
RTC_DATA_ATTR bool newActivityUploaded;
RTC_DATA_ATTR bool activityUpdated = false;
RTC_DATA_ATTR TsActivity lastActivity;

QueueHandle_t xQueueStrava;
SemaphoreHandle_t xSemaphore = NULL;

void printDateTime(struct tm *dateStruct);
bool getAccessToken(char *ret_token);
int8_t getLastActivitieDist(time_t start, time_t end, bool isLast);
time_t timeStringToTimestamp(char *str);
void timeStringToTm(const char *str, struct tm *tm);
TeActivityType getActivityType(const char *str);
void getYearActivities(time_t start, time_t end);
void printDB(uint16_t nbDays);
bool lastActivityUpdated(TsActivity *newActivity);
void addIdLastActivities(uint64_t id);
bool isIdLastActivities(uint64_t id);

void resetDB()
{
    for (uint16_t i = 0; i < DAYS_BY_YEAR; i++)
    {
        loopYear[i].distBike = 0;
        loopYear[i].distRun = 0;
        loopYear[i].timeBike = 0;
        loopYear[i].timeRun = 0;
        loopYear[i].climbBike = 0;
        loopYear[i].climbRun = 0;
    }
    lastDayPopulate = 0;
    lastActivityId = 0;

    preferences.begin("stravaDB", false);

    preferences.getString("apiRefreshToken", apiRefreshToken, sizeof(apiRefreshToken));
    preferences.getString("clientSecret", clientSecret, sizeof(clientSecret));
    clientId = preferences.getLong64("clientId", 0);

    preferences.clear();
    preferences.putLong("lastDayPopulate", lastDayPopulate);
    preferences.putLong64("lastActivityId", lastActivityId);
    preferences.putBytes("loopYear", loopYear, sizeof(loopYear));
    preferences.putString("apiRefreshToken", apiRefreshToken);
    preferences.putString("clientSecret", clientSecret);
    preferences.putLong64("clientId", clientId);

    preferences.end();
}

void initDB()
{
    static bool isDBInit = false;
    if (isDBInit == false)
    {
        Serial.println("initDB done");
        preferences.begin("stravaDB", false);
        lastActivityId = preferences.getLong64("lastActivityId", 0);
        lastDayPopulate = preferences.getLong("lastDayPopulate", 0);
        preferences.getBytes("loopYear", loopYear, sizeof(loopYear));

        preferences.getString("apiRefreshToken", apiRefreshToken, sizeof(apiRefreshToken));
        preferences.getString("clientSecret", clientSecret, sizeof(clientSecret));
        clientId = preferences.getLong64("clientId", 0);

        preferences.end();
        DataSave_RetreiveLastActivity();
        isDBInit = true;
    }
    else
    {
        Serial.print("initDB already done : ");
        Serial.print("lastDayPopulate = ");
        Serial.println(lastDayPopulate);
    }
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
        std::string getTokenUrl;
        getTokenUrl = "https://www.strava.com/oauth/token?grant_type=refresh_token&client_id=";
        getTokenUrl += std::to_string(clientId);
        getTokenUrl += "&client_secret=";
        getTokenUrl += clientSecret;
        getTokenUrl += "&refresh_token=";
        getTokenUrl += apiRefreshToken;

        http.begin(getTokenUrl.c_str());

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

int8_t getLastActivitieDist(time_t start, time_t end, bool isLast)
{
    int8_t ret = -1;
    std::string t = std::to_string(start);
    char const *startTimestampStr = t.c_str();
    std::string u = std::to_string(end);
    char const *endTimestampStr = u.c_str();

    char bearerToken[53] = "Bearer ";
    uint8_t tokenReqCnt = 0;
    while (!getAccessToken(&bearerToken[7]) && tokenReqCnt < 5)
    {
        tokenReqCnt++;
        if (tokenReqCnt == 5)
        {
            Serial.println("pb server strava ?");
            return -4;
        }
    }
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
            // for (JsonVariant v : array)
            for (int8_t i = array.size() - 1; i >= 0; i--)
            {
                Serial.print("i = ");
                Serial.println(i);
                if (array[i]["trainer"].as<bool>() == true)
                {
                    continue;
                }
                struct tm tm;
                timeStringToTm(array[i]["start_date_local"].as<const char *>(), &tm);
                TeActivityType activityType = getActivityType(array[i]["type"].as<const char *>());
                time_t activityStartTime = mktime(&tm);
                time_t movingTime = array[i]["moving_time"].as<int>();
                uint64_t activityId = array[i]["id"].as<uint64_t>();
                addIdLastActivities(activityId);
                Serial.print("activityId = ");
                Serial.println(activityId);
                Serial.print("activity start timestamp : ");
                Serial.println(activityStartTime);
                TsActivity tmpActivity = {
                    false,                 // isFilled
                    0,                     // dist
                    0,                     // time
                    0,                     // deniv
                    0,                     // timestamp
                    ACTIVITY_TYPE_UNKNOWN, // type
                    "",                    // name
                    "",                    // polyline
                    0                      // kudos
                };
                if (activityStartTime >= lastActivity.timestamp && isLast && i == 0)
                {
                    tmpActivity.isFilled = true;
                    if (/*isLast &&*/ i == 0)
                    {
                        if (array[i]["map"]["summary_polyline"].is<std::string>())
                        {
                            tmpActivity.polyline = array[i]["map"]["summary_polyline"].as<std::string>();
                        }
                        if (array[i]["name"].is<const char *>())
                        {
                            strncpy(tmpActivity.name, array[i]["name"].as<const char *>(), sizeof(tmpActivity.name));
                            // memcpy(tmpActivity.name, tmpName.c_str(), min((int)tmpName.size(), MAX_NAME_LENGTH - 1));
                            // tmpActivity.name[min((int)tmpName.size(), MAX_NAME_LENGTH - 1)] = '\0';
                        }
                    }
                    tmpActivity.type = activityType;
                    tmpActivity.time = (uint16_t)movingTime;
                    tmpActivity.deniv = array[i]["total_elevation_gain"].as<uint16_t>();
                    tmpActivity.dist = (uint16_t)(array[i]["distance"].as<float>() / 10.0);

                    tmpActivity.kudos = array[i]["kudos_count"].as<uint32_t>();
                    tmpActivity.timestamp = activityStartTime;
                    activityUpdated = lastActivityUpdated(&tmpActivity);
                    newActivityUploaded = newActivityUploaded || activityUpdated;
                    if (activityStartTime == lastActivity.timestamp)
                    {
                        continue;
                    }

                    lastActivity.timestamp = activityStartTime;
                }
                if (activityStartTime - 1 == lastDayPopulate || prevLastActivityTimestamp == activityStartTime || activityId == lastActivityId || isIdLastActivities(activityId) == true)
                {
                    Serial.println("already processed");
                    // dont process previous last activity, it was already accounted
                    continue;
                }

                newActivityUploaded = true;
                int utcOffset = array[i]["utc_offset"].as<int>();
                Serial.print("cucu");
                if (activityStartTime /*+ utcOffset*/ > lastDayPopulate)
                {
                    lastDayPopulate = activityStartTime /*+ utcOffset*/ - 1;
                    lastActivityId = activityId;
                }
                Serial.println("added to year array");
                uint16_t dayIdx = monthOffset[tm.tm_mon] + tm.tm_mday - 1;
                switch (activityType)
                {
                case ACTIVITY_TYPE_BIKE:

                    loopYear[dayIdx].distBike += (uint32_t)(array[i]["distance"].as<float>() * 10.0);
                    loopYear[dayIdx].climbBike += array[i]["total_elevation_gain"].as<int>();
                    loopYear[dayIdx].timeBike += (uint32_t)(array[i]["moving_time"].as<int>());

                    break;
                case ACTIVITY_TYPE_RUN:

                    loopYear[dayIdx].distRun += (uint32_t)(array[i]["distance"].as<float>() * 10.0);
                    loopYear[dayIdx].climbRun += array[i]["total_elevation_gain"].as<int>();
                    loopYear[dayIdx].timeRun += (uint32_t)(array[i]["moving_time"].as<int>());

                    break;
                default:
                    Serial.println("UNKNOWN TYPE");
                    break;
                }
                Serial.print(dayIdx);
                Serial.print(" : ");
                Serial.print(array[i]["name"].as<const char *>());
                Serial.print(" : ");
                Serial.print(array[i]["distance"].as<float>() / 1000.0, 2);
                Serial.print(" km -> ");
                Serial.println(array[i]["type"].as<const char *>());
            }
            memcpy(lastActivitiesId, tmpLastActivitiesId, sizeof(tmpLastActivitiesId));
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
        ret = -3;
    }
    // esp_task_wdt_reset();
    http.end();
    return ret;
}

bool lastActivityUpdated(TsActivity *newActivity)
{
    Serial.println("start lastActivityUpdated");
    bool l_ret = false;
    if (lastActivity.timestamp != newActivity->timestamp)
    {
        l_ret = true;
        // lastActivity.timestamp = newActivity.timestamp;
    }
    if (lastActivity.dist != newActivity->dist)
    {
        l_ret = true;
        lastActivity.dist = newActivity->dist;
    }
    if (lastActivity.deniv != newActivity->deniv)
    {
        l_ret = true;
        lastActivity.deniv = newActivity->deniv;
    }
    if (lastActivity.time != newActivity->time)
    {
        l_ret = true;
        lastActivity.time = newActivity->time;
    }
    if (lastActivity.kudos != newActivity->kudos)
    {
        l_ret = true;
        lastActivity.kudos = newActivity->kudos;
    }
    if (lastActivity.type != newActivity->type)
    {
        l_ret = true;
        lastActivity.type = newActivity->type;
    }
    if (strncmp(lastActivity.name, newActivity->name, sizeof(lastActivity.name)) != 0)
    {
        l_ret = true;
        strncpy(lastActivity.name, newActivity->name, sizeof(lastActivity.name));
    }
    if (lastActivity.isFilled != newActivity->isFilled)
    {
        l_ret = true;
        lastActivity.isFilled = newActivity->isFilled;
    }
    lastActivity.polyline = newActivity->polyline;
    if (hasBeenRefreshed)
    {
        l_ret = true;
        hasBeenRefreshed = false;
    }
    Serial.print("lastActivityUpdated() : ");
    Serial.println(l_ret);
    return l_ret;
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
    initDB();
    Serial.print("LastDayPopulate at start : ");
    Serial.println(lastDayPopulate);

    // reset everything
    // lastDayPopulate = 0;
    // lastDayPopulate = 1746061295;
    // for (uint16_t i = 121; i < 133; i++)
    // {
    //     loopYear[i].climbRun = 0;
    //     loopYear[i].timeRun = 0;
    //     loopYear[i].distRun = 0;
    //     loopYear[i].climbBike = 0;
    //     loopYear[i].distBike = 0;
    //     loopYear[i].timeBike = 0;
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
    newActivityUploaded = false;
    getYearActivities(startTimestamp, endTimestamp);
    // save loopYear
    if (newActivityUploaded)
    {
        preferences.begin("stravaDB", false);
        preferences.clear();
        preferences.putLong("lastDayPopulate", lastDayPopulate);
        preferences.putLong64("lastActivityId", lastActivityId);
        preferences.putBytes("loopYear", loopYear, sizeof(loopYear));
        preferences.putString("apiRefreshToken", apiRefreshToken);
        preferences.putString("clientSecret", clientSecret);
        preferences.putLong64("clientId", clientId);
        preferences.end();
        Serial.print("lastdaypopulate end : ");
        Serial.println(lastDayPopulate);
        Serial.print("lastActivityId end : ");
        Serial.println(lastActivityId);
        DataSave_SaveLastActivity();
    }
    else
    {
        Serial.println("no new activity, no save");
    }
    // printDB(0);
}

void getYearActivities(time_t start, time_t end)
{

    time_t tmp = start;
    int8_t ret = -1;
    bool lastRequest = false;

    while (tmp < end)
    {
        if ((tmp + TWO_WEEK_IN_SECOND) < end)
        {
            tmp += TWO_WEEK_IN_SECOND;
            lastRequest = false;
        }
        else
        {
            tmp = end;
            start = tmp - TWO_WEEK_IN_SECOND;
            lastRequest = true;
        }
        ret = -1;
        while (ret != 0)
        {
            ret = getLastActivitieDist(start, tmp, lastRequest);
            if (ret == -2) // error 429 (too much requests)
            {
                // save arrays, save lastDay populate, exit
                Serial.println("error 429, too much requests, exit");
                return;
            }
            else if (ret == -4)
            {
                Serial.println("pb server ?");
                return;
            }
        }
        start = tmp + 1;
        if (!lastRequest)
        {
            TeDisplayMessage msg;
            msg = DISPLAY_MESSAGE_WEEKS;
            xQueueSend(xQueueDisplay, &msg, 0);
            // msg = DISPLAY_MESSAGE_TOTAL_YEAR;
            // xQueueSend(xQueueDisplay, &msg, 0);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }
    }
}

void printDB(uint16_t nbDays)
{
    Serial.println("j -> run  ; bike ");
    for (uint8_t j = 0; j < 12; j++)
    {
        uint32_t totalMon = 0;
        Serial.print("mois : ");
        Serial.println(j);
        for (uint16_t i = monthOffset[j]; i < monthOffset[j + 1]; i++)
        {
            Serial.print(i);
            Serial.print(" -> ");
            Serial.print("loop Year : Run ");
            Serial.print(loopYear[i].distRun / 10000);
            Serial.print(" Bike ");
            Serial.print(loopYear[i].distBike / 10000);
            Serial.println(" ; ");
            totalMon += loopYear[i].distBike + loopYear[i].distRun;
        }
        Serial.print("Total : ");
        Serial.println(totalMon / 10000);
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

void newMonthBegin()
{
    initDB();
    struct tm tm, lastDayPopulateTm;

    lastDayPopulateTm = *localtime(&lastDayPopulate);

    if (getLocalTime(&tm))
    {
        if (lastDayPopulateTm.tm_year < tm.tm_year - 1 || (lastDayPopulateTm.tm_year == tm.tm_year - 1 && lastDayPopulateTm.tm_mon <= tm.tm_mon))
        {
            for (uint16_t i = 0; i < DAYS_BY_YEAR; i++)
            {
                memset(&loopYear[i], 0, sizeof(TsDistDay));
            }
        }
        else
        {
            for (uint8_t j = lastDayPopulateTm.tm_mon; j != tm.tm_mon; j++)
            {
                if (j > 11)
                {
                    j = 0;
                }
                Serial.print("erasing month ");
                Serial.println(j);
                for (uint16_t i = monthOffset[j]; i < monthOffset[j + 1]; i++)
                {
                    Serial.print(i);
                    Serial.println(" erased");
                    memset(&loopYear[i], 0, sizeof(TsDistDay));
                }
            }
            for (uint16_t i = monthOffset[tm.tm_mon]; i < monthOffset[tm.tm_mon + 1]; i++)
            {
                Serial.print(i);
                Serial.println(" erased");
                memset(&loopYear[i], 0, sizeof(TsDistDay));
            }
        }
        preferences.begin("stravaDB", false);
        preferences.clear();
        preferences.putLong("lastDayPopulate", lastDayPopulate);
        preferences.putLong64("lastActivityId", lastActivityId);
        preferences.putBytes("loopYear", loopYear, sizeof(loopYear));
        preferences.putString("apiRefreshToken", apiRefreshToken);
        preferences.putString("clientSecret", clientSecret);
        preferences.putLong64("clientId", clientId);
        preferences.end();
    }
    // printDB(0);
}

TsActivity *getStravaLastActivity()
{
    return &lastActivity;
}

void addIdLastActivities(uint64_t id)
{
    static uint8_t i = 0;
    tmpLastActivitiesId[i] = id;
    i++;
    if (i >= NB_LAST_ACTIVITIES)
    {
        i = 0;
    }
}

bool isIdLastActivities(uint64_t id)
{
    Serial.println("in isIdLastActivities : ");
    for (uint8_t i = 0; i < NB_LAST_ACTIVITIES; i++)
    {
        if (lastActivitiesId[i] == id)
        {
            return true;
        }
    }
    Serial.println("out isIdLastActivities : ");
    return false;
}

void StravaTaskFunction(void *parameter)
{
    TeStravaMessage msg;
    TeDisplayMessage messageDisplay;
    uint8_t *taskCnt = (uint8_t *)parameter;
    struct tm timeinfo3;
    while (true)
    {
        msg = STRAVA_MESSAGE_NONE;
        if (xQueueReceive(xQueueStrava, &(msg), 0) == pdPASS)
        {
            Serial.print("Queue strava message received :");
            (*taskCnt)++;
            switch (msg)
            {
            case STRAVA_MESSAGE_NEW_MONTH:
                Serial.println("new month begin");
                newMonthBegin();
                break;
            case STRAVA_MESSAGE_POPULATE:
                Serial.println("populate");
                if (connectWifi(10000))
                {
                    // esp_task_wdt_reset();
                    populateDB();
                    // esp_task_wdt_reset();
                }
                if (newActivityUploaded)
                {
                    messageDisplay = DISPLAY_MESSAGE_NEW_ACTIVITY;
                    xQueueSend(xQueueDisplay, &messageDisplay, 0);
                    newActivityUploaded = false;
                }
                getLocalTime(&timeinfo3);
                if (timeinfo3.tm_hour % 2 == 0)
                {
                    messageDisplay = DISPLAY_MESSAGE_MONTHS;
                    xQueueSend(xQueueDisplay, &messageDisplay, 0);
                }
                else if (timeinfo3.tm_hour % 2 == 1)
                {
                    messageDisplay = DISPLAY_MESSAGE_WEEKS;
                    xQueueSend(xQueueDisplay, &messageDisplay, 0);
                }
                break;
            default:
                break;
            }
            (*taskCnt)--;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(NULL);
}