#include <HTTPClient.h>
#include "ArduinoJson.h"
#include "credentials.h"
#include <time.h>
#include <string.h>

#define WEEK_IN_SECOND 604800U
#define SIZE_OF_DB 40

const char activitiesUrl[] = "https://www.strava.com/api/v3/athlete/activities?";
// struct tm tm;
float distArray[SIZE_OF_DB] = {0};

void printDateTime(struct tm *dateStruct);
bool getAccessToken(char *ret_token);
int getLastActivitieDist(time_t start, time_t end);
time_t timeStringToTimestamp(char *str);
void timeStringToTm(const char *str, struct tm *tm);

bool getAccessToken(char *ret_token)
{
    bool ret = true;
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
            ret = false;
        }
        else
        {
            const char *accessToken = doc["access_token"];
            strcpy(ret_token, accessToken);
            Serial.println("Token OK");
        }
    }
    else
    {
        Serial.println("HTTP response " + String(httpResponseCode));
        ret = false;
    }
    http.end();
    return ret;
}

int getLastActivitieDist(time_t start, time_t end)
{
    std::string t = std::to_string(start);
    char const *startTimestampStr = t.c_str();
    std::string u = std::to_string(end);
    char const *endTimestampStr = u.c_str();

    char token[45];
    while (!getAccessToken(token))
        ;
    char bearerToken[53] = "Bearer ";
    strcat(bearerToken, token);
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
                distArray[tm.tm_yday] += v["distance"].as<float>() / 1000.0;
                Serial.print(v["name"].as<String>());
                Serial.print(" : ");
                Serial.print(v["distance"].as<float>() / 1000.0, 2);
                Serial.println(" km");
            }
        }
    }
    else
    {
        Serial.println("OHHH");
    }
    http.end();
    return 0;
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
    struct tm timeinfo;
    time_t startTimestamp, endTimestamp, tmp;
    // getLocalTime(&timeinfo);
    char startDate[] = "2024-01-01T00:00:00Z";
    char endDate[] = "2024-02-01T00:00:00Z";
    startTimestamp = timeStringToTimestamp(startDate);
    tmp = startTimestamp;
    // endTimestamp = mktime(&timeinfo);
    endTimestamp = timeStringToTimestamp(endDate);

    while (tmp < endTimestamp)
    {
        if ((tmp + WEEK_IN_SECOND) < endTimestamp)
        {
            tmp += WEEK_IN_SECOND;
        }
        else
        {
            tmp = endTimestamp;
        }
        getLastActivitieDist(startTimestamp, tmp);
        startTimestamp = tmp + 1;
    }
}

void printDB()
{
    for (uint8_t i = 0; i < SIZE_OF_DB; i++)
    {
        Serial.print(i);
        Serial.print(" -> ");
        Serial.println(distArray[i]);
    }
}

void test()
{
    Serial.println("start test");
    populateDB();
    printDB();
    Serial.println("end test");
}