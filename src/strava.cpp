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
#include "RTCTime.h"
#include <esp_task_wdt.h>
#include <algorithm> // std::max

typedef struct sDistDay
{
    uint32_t distRun;   // in decimeters - first bit used to know if there is activity that day
    uint32_t distBike;  // in decimeters
    uint32_t timeRun;   // in seconds
    uint32_t timeBike;  // in seconds
    uint16_t climbRun;  // in decimeters
    uint16_t climbBike; // in decimeters
} TsDistDay;

#define WEEK_IN_SECOND 604800U
#define TWO_WEEK_IN_SECOND 2 * WEEK_IN_SECOND
#define DAYS_BY_YEAR 366
#define THIS_YEAR_OFFSET 366

const uint16_t monthOffset[] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366};

const char activitiesUrl[] = "https://www.strava.com/api/v3/athlete/activities?";
const char weatherUrl[] = "https://www.cvsevrier.fr/wp-json/gm/v1/wind";

TsDistDay loopYear[DAYS_BY_YEAR];
uint64_t lastActivitiesId[NB_LAST_ACTIVITIES];
time_t lastActivityTimestamp;
uint64_t tmpLastActivitiesId[NB_LAST_ACTIVITIES];
time_t lastDayPopulate;
uint64_t lastActivityId;
struct tm timeinfo;
RTC_DATA_ATTR bool newActivityUploaded;
RTC_DATA_ATTR bool activityUpdated = false;
RTC_DATA_ATTR TsActivity lastActivity;
RTC_DATA_ATTR uint16_t prevKudos = 0;
RTC_DATA_ATTR uint16_t prevPrevKudos = 0;
RTC_DATA_ATTR int airTemperature;
RTC_DATA_ATTR int waterTemperature;

QueueHandle_t xQueueStrava;
SemaphoreHandle_t xSemaphore = NULL;
SemaphoreHandle_t mutex = NULL;

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
void sendMessage(std::string msg);
bool isArrayZero(const TsDistDay *array, size_t size);
void getWeather(int *airTemp, int *waterTemp);

void test_NVM()
{
    static uint16_t i = 0;
    static uint8_t j = 0;
    Preferences preferences;

    if (preferences.begin("stravaDB", false))
    {
        lastActivityId = preferences.getLong64("lastActivityId", 0);
        lastDayPopulate = preferences.getLong("lastDayPopulate", 0);
        size_t l_bytesRead = preferences.getBytes("loopYear", loopYear, sizeof(loopYear));
        // Serial.print("Bytes read from preferences loopYear : ");
        // Serial.print(l_bytesRead);
        // Serial.println(" / " + String(sizeof(loopYear)));
        if (l_bytesRead != sizeof(loopYear))
        {
            Serial.println("error read");
            sendMessage("error%20read%20stravaDB");
        }

        // Serial.println("Read OK");

        loopYear[i].distBike = i + j;

        preferences.clear();
        size_t l_bytesWritten = preferences.putBytes("loopYear", loopYear, sizeof(loopYear));
        if (l_bytesWritten != sizeof(loopYear))
        {
            Serial.println("error write");
        }
        preferences.putLong("lastDayPopulate", lastDayPopulate);
        preferences.putLong64("lastActivityId", lastActivityId);
        // Serial.print("Bytes written to preferences loopYear : ");
        // Serial.print(l_bytesWritten);
        // Serial.println(" / " + String(sizeof(loopYear)));
        preferences.end();
        // Serial.println("Write OK");

        i++;
        if (i >= DAYS_BY_YEAR)
        {
            i = 0;
            j++;
        }
        Serial.print("i = ");
        Serial.print(i);
        Serial.print(" / j = ");
        Serial.println(j);
    }
}

void sendMessage(std::string msg)
{
    if (connectWifi(20000))
    {
        HTTPClient http;
        std::string url = "https://smsapi.free-mobile.fr/sendmsg?user=15021218&pass=Qgaj1FvAsab08d&msg=";
        url += msg;

        http.begin(url.c_str());

        int httpResponseCode = http.GET();
        Serial.println("HTTP Response code: " + String(httpResponseCode));
        http.end(); // Free resources
    }
}

void getWeather(int *airTemp, int *waterTemp)
{

    HTTPClient http;

    http.begin(weatherUrl);

    int httpResponseCode = http.GET();

    if (httpResponseCode == 200)
    {
        String resp = http.getString();
        // const char *resp = http.getString().c_str();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, resp.c_str());
        if (error)
        {
            Serial.print("GET WEATHER deserializeJson() returned ");
            Serial.println(error.c_str());
        }
        else
        {
            *airTemp = doc["temperature"];
            *waterTemp = doc["water"];

            Serial.print("Air temperature : ");
            Serial.println(*airTemp);
            Serial.print("Water temperature : ");
            Serial.println(*waterTemp);
        }
    }
}

void resetDB()
{
    Preferences preferences;
    sendMessage("resetDB");

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

    preferences.clear();
    preferences.putLong("lastDayPopulate", lastDayPopulate);
    preferences.putLong64("lastActivityId", lastActivityId);
    preferences.putBytes("loopYear", loopYear, sizeof(loopYear));

    preferences.end();

    DataSave_resetLastActivities();
    resetClock();
}

bool initDB()
{
    Preferences preferences;

    static bool isDBInit = false;
    if (isDBInit == false)
    {
        Serial.println("initDB done");
        while (!xSemaphoreTake(mutex, portMAX_DELAY))
        {
            // wait for mutex to be available
            Serial.println("waiting for mutex");
        }
        if (preferences.begin("stravaDB", true))
        {
            lastActivityId = preferences.getLong64("lastActivityId", 0);
            lastDayPopulate = preferences.getLong("lastDayPopulate", 0);
            size_t l_bytesRead = preferences.getBytes("loopYear", loopYear, sizeof(loopYear));
            if (l_bytesRead != sizeof(loopYear))
            {
                Serial.println("error read");
                sendMessage("error%20read%20stravaDB");

                preferences.end();
                sendMessage("l_bytesRead%20!=%20sizeof(loopYear),%20resetting%20stravaDB");
                resetDB();
                preferences.begin("stravaDB", true);
            }
            if (isArrayZero(loopYear, DAYS_BY_YEAR) && lastDayPopulate != 0)
            {
                Serial.println("loopYear is empty, resetting");
                preferences.end();
                resetDB();
                sendMessage("loopYears%20is%20empty,%20resetting%20stravaDB");
                ESP.restart();
            }

            preferences.end();
            DataSave_RetreiveLastActivity();

            isDBInit = l_bytesRead != 0;
        }
        DataSave_RetrieveStravaCredentials();
        xSemaphoreGive(mutex);
    }
    else
    {
        Serial.print("initDB already done : ");
        Serial.print("lastDayPopulate = ");
        Serial.println(lastDayPopulate);
    }
    return isDBInit;
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
            Serial.println("getAccessToken HTTP response " + String(httpResponseCode));
            Serial.println(getTokenUrl.c_str());
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
            if (error == DeserializationError::NoMemory)
            {
                Serial.println("not enough memory for deserialization");
                ret = -5;
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
                    0,                     // kudos
                    false                  // isVisible
                };

                if (i == NB_LAST_ACTIVITIES - 1)
                {
                    lastActivityTimestamp = activityStartTime;
                }

                if (/*activityStartTime >= lastActivity.timestamp &&*/ isLast && i == 0)
                {
                    Serial.println("process last activity");
                    // this is the last activity, update lastActivity
                    tmpActivity.isFilled = true;

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

                    tmpActivity.type = activityType;
                    tmpActivity.time = (uint16_t)movingTime;
                    tmpActivity.deniv = array[i]["total_elevation_gain"].as<uint16_t>();
                    tmpActivity.dist = (uint16_t)(array[i]["distance"].as<float>() / 10.0);

                    tmpActivity.kudos = array[i]["kudos_count"].as<uint32_t>();
                    tmpActivity.timestamp = activityStartTime;
                    tmpActivity.isVisible = !array[i]["private"].as<bool>();
                    activityUpdated = lastActivityUpdated(&tmpActivity);
                    newActivityUploaded = newActivityUploaded || activityUpdated;
                    if (activityStartTime == lastActivity.timestamp)
                    {
                        continue;
                    }

                    lastActivity.timestamp = activityStartTime;
                }
                if (activityStartTime - 1 == lastDayPopulate || prevLastActivityTimestamp == activityStartTime || activityId == lastActivityId || isIdLastActivities(activityId) == true || lastActivityTimestamp > activityStartTime)
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
                // save that there is an activity this day
                loopYear[dayIdx].distRun |= 0x80000000;
                if (array[i]["trainer"].as<bool>() == true)
                {
                    // ignore trainer activities in total stats
                    continue;
                }
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
    if (lastActivity.kudos == prevKudos && prevKudos != prevPrevKudos)
    {
        // prevKudos is updated later in drawLastActivity
        l_ret = true;
        prevPrevKudos = prevKudos;
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
    if (lastActivity.isVisible != newActivity->isVisible)
    {
        l_ret = true;
        lastActivity.isVisible = newActivity->isVisible;
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
    // for (uint16_t i = 0; i < 366; i++)
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
        if (initDB())
        {
            while (!xSemaphoreTake(mutex, portMAX_DELAY))
            {
                // wait for mutex to be available
                Serial.println("waiting for mutex");
            }
            Preferences preferences;
            if (preferences.begin("stravaDB", false))
            {
                preferences.clear();
                preferences.putLong("lastDayPopulate", lastDayPopulate);
                preferences.putLong64("lastActivityId", lastActivityId);
                if (isArrayZero(loopYear, DAYS_BY_YEAR))
                {
                    Serial.println("loopYear is empty, resetting");
                    sendMessage("loopYears%20is%20empty%20during%20populateDB");
                }

                size_t l_writtenBytes = preferences.putBytes("loopYear", loopYear, sizeof(loopYear));
                if (l_writtenBytes != sizeof(loopYear))
                {
                    Serial.println("error write");
                    sendMessage("error%20write%20stravaDB");
                }
                preferences.end();
            }
            xSemaphoreGive(mutex);
        }
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
            else if (ret == -5)
            { // deserialization no memory
                Serial.println("deserialization no memory");
                tmp = start + (tmp - start) / 2;
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
            Serial.print((loopYear[i].distRun & 0x7FFFFFFF) / 10000);
            Serial.print(" Bike ");
            Serial.print(loopYear[i].distBike / 10000);
            Serial.println(" ; ");
            totalMon += loopYear[i].distBike + loopYear[i].distRun & 0x7FFFFFFF;
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
                if ((loopYear[i].distRun & 0x80000000) != 0)
                {

                    ret += loopYear[i].distRun & 0x7FFFFFFF;
                }
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
            for (uint8_t j = lastDayPopulateTm.tm_mon + 1; j != tm.tm_mon; j++)
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
        if (initDB())
        {
            while (!xSemaphoreTake(mutex, portMAX_DELAY))
            {
                // wait for mutex to be available
                Serial.println("waiting for mutex");
            }
            Preferences preferences;

            if (preferences.begin("stravaDB", false))
            {
                preferences.clear();
                preferences.putLong("lastDayPopulate", lastDayPopulate);
                preferences.putLong64("lastActivityId", lastActivityId);
                size_t l_writtenBytes = preferences.putBytes("loopYear", loopYear, sizeof(loopYear));
                if (l_writtenBytes != sizeof(loopYear))
                {
                    Serial.println("error write");
                    sendMessage("error%20write%20stravaDB");
                }
                preferences.end();
            }
            xSemaphoreGive(mutex);
        }
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

bool isArrayZero(const TsDistDay *array, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        if (array[i].distRun != 0 ||
            array[i].distBike != 0 ||
            array[i].timeRun != 0 ||
            array[i].timeBike != 0 ||
            array[i].climbRun != 0 ||
            array[i].climbBike != 0)
        {
            return false;
        }
    }
    return true;
}

bool isLeapYear(int year)
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

bool isLastActivityFromToday()
{
    bool isFromToday = false;
    struct tm tm;
    if (!getLocalTime(&tm))
    {
        Serial.println("Failed to obtain time");
        return false;
    }
    uint16_t todayIdx = monthOffset[tm.tm_mon] + tm.tm_mday - 1;

    if ((loopYear[todayIdx].distRun & 0x80000000) == 0x80000000)
    {
        isFromToday = true;
    }

    return isFromToday;
}

uint32_t getCurrentStreakDays()
{
    initDB();
    struct tm tm;
    if (!getLocalTime(&tm))
    {
        Serial.println("Failed to obtain time");
        return 0;
    }
    uint16_t todayIdx = monthOffset[tm.tm_mon] + tm.tm_mday - 1;
    uint32_t streakDays = 0;

    uint16_t yearToCkeckLeap = tm.tm_year + 1900;
    if (todayIdx < 59)
    {
        yearToCkeckLeap--;
    }
    bool isLeap = isLeapYear(yearToCkeckLeap);
    int16_t lastDayToCheck = todayIdx + 2;
    if (isLeap)
    {
        lastDayToCheck = todayIdx + 1;
    }

    for (int16_t i = todayIdx - 1; i != todayIdx + 1; i--)
    {
        Serial.print("checking day index : ");
        Serial.println(i);
        Serial.println(loopYear[i].distRun & 0x80000000);
        if (i < 0)
        {
            i = DAYS_BY_YEAR - 1;
        }
        if (isLeap == false && i == 59)
        {
            // skip feb 29 in non leap year
            continue;
        }

        if ((loopYear[i].distRun & (uint32_t)0x80000000) == (uint32_t)0x80000000)
        {
            streakDays++;
        }
        else
        {
            break;
        }
    }
    if ((loopYear[todayIdx].distRun & 0x80000000) == 0x80000000)
    {
        streakDays++;
    }
    return streakDays;
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
                if (connectWifi(20000))
                {
                    populateDB();
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
                messageDisplay = DISPLAY_MESSAGE_STATUS;
                xQueueSend(xQueueDisplay, &messageDisplay, 0);
                break;

            case STRAVA_MESSAGE_GET_TEMPERATURE:
            {
                int prevAirTemp = airTemperature;
                int prevWaterTemp = waterTemperature;

                Serial.println("get temperature");

                if (connectWifi(10000))
                {
                    getWeather(&airTemperature, &waterTemperature);
                }

                if (airTemperature != prevAirTemp || waterTemperature != prevWaterTemp)
                {
                    messageDisplay = DISPLAY_MESSAGE_TEMPERATURE;
                    xQueueSend(xQueueDisplay, &messageDisplay, 0);
                }
                break;
            }

            default:
                break;
            }
            (*taskCnt)--;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(NULL);
}