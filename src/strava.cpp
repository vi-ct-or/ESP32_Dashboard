#include <HTTPClient.h>
#include "ArduinoJson.h"
#include "credentials.h"
#include <string.h>

const char activitiesUrl[] = "https://www.strava.com/api/v3/athlete/activities?";

void getAccessToken(char *ret_token)
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
            Serial.print("deserializeJson() returned ");
            Serial.println(error.c_str());
            Serial.println(resp);
        }
        else
        {
            const char *accessToken = doc["access_token"];
            strcpy(ret_token, accessToken);
        }
    }
    else
    {
        Serial.println("HTTP response " + String(httpResponseCode));
    }
    http.end();
}

int getLastActivitieDist()
{
    char token[45];
    getAccessToken(token);
    char bearerToken[53] = "Bearer ";
    strcat(bearerToken, token);
    Serial.println(bearerToken);

    char fullUrl[100];
    strcpy(fullUrl, activitiesUrl);
    // strcat(fullUrl, "after=1729247974");
    // strcat(fullUrl, "&before=1729427974");
    strcat(fullUrl, "&per_page=10");
    // Serial.println(fullUrl);

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
