#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "AsyncJson.h"
#include "htmlPages.h"
#include <ESPmDNS.h>
#include <Preferences.h>
#include "displayEpaper.h"
#include "credentials.h"
#include "network.h"

AsyncWebServer server(80);

typedef enum
{
    CONFIG_STATE_NONE,
    CONFIG_STATE_WAIT_ACTION,
    CONFIG_STATE_AP_STARTED,
    CONFIG_STATE_AP_CLIENT_CONNECTED,
    CONFIG_STATE_CLIENT_CONNECTED_WIFI_URL,
    CONFIG_STATE_CREDENTIALS_ENTERED,
    CONFIG_STATE_CONNECTING,
    CONFIG_STATE_CONNECTED,
    CONFIG_STATE_CONNECTION_FAILED,
    CONFIG_STATE_STRAVA_START_AUTH,
    CONFIG_STATE_STRAVA_WAIT_AUTH,
    CONFIG_STATE_STRAVA_AUTH_CODE_RECEIVED,
    CONFIG_STATE_STRAVA_AUTH_FAILED,
    CONFIG_STATE_SERVER_OFF,
} TeConfigState;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 5000;

TeConfigState state = CONFIG_STATE_NONE;

int nbNetwork = 0;
int lastScan = -10000;
bool lockScan = false;
int connectionStart = 0;
Preferences configPreferences;

String newSSID, newPasswd, code;

String header;
void webroutes();
String buildResponseHome();
bool getRefreshToken();

bool loopWebServer()
{
    bool l_ret = false;
    switch (state)
    {
    case CONFIG_STATE_NONE:
        // start AP
        if (ssid1[0] == 0 || !connectWifi(20000))
        {
            WiFi.disconnect();
            WiFi.mode(WIFI_MODE_APSTA);
            WiFi.softAP("Dashboard_Strava", "defaultPassword");
            nbNetwork = WiFi.scanNetworks();
            lastScan = millis();
            webroutes();
            server.begin();
            state = CONFIG_STATE_AP_STARTED;
            Serial.println("connect to ap wfi");
            displayQrAP();
        }
        else
        {
            webroutes();
            server.begin();
            state = CONFIG_STATE_CONNECTED;
            Serial.println("skip server AP");
        }

        break;

    case CONFIG_STATE_AP_STARTED:

        if (WiFi.softAPgetStationNum() > 0)
        {
            state = CONFIG_STATE_AP_CLIENT_CONNECTED;
            // display qr code wifi configuration
            displayQrUrl();

            Serial.println("select the wifi...");
        }
        break;

    case CONFIG_STATE_AP_CLIENT_CONNECTED:
        break;

    case CONFIG_STATE_CLIENT_CONNECTED_WIFI_URL:
        if (millis() - lastScan > 10000 && !lockScan)
        {
            lockScan = true;
            Serial.println("scannning");
            nbNetwork = WiFi.scanNetworks();
            lockScan = false;
            lastScan = millis();
        }
        break;

    case CONFIG_STATE_CREDENTIALS_ENTERED:
        WiFi.begin(newSSID, newPasswd);
        Serial.println("connecting");
        connectionStart = millis();
        state = CONFIG_STATE_CONNECTING;
        break;

    case CONFIG_STATE_CONNECTING:
        if (millis() - connectionStart < 20000)
        {
            if (WiFi.isConnected())
            {
                configPreferences.begin("configNetwork");
                nbNetwork = configPreferences.getUShort("nbNetwork", 0);
                configPreferences.putString("ssid", newSSID);
                configPreferences.putString("pswd", newPasswd);
                configPreferences.putUShort("nbNetwork", 1);
                configPreferences.end();

                state = CONFIG_STATE_CONNECTED;
            }
        }
        else
        {
            state = CONFIG_STATE_CONNECTION_FAILED;
        }
        break;

    case CONFIG_STATE_CONNECTED:

        Serial.println("connected to wifi");
        MDNS.begin("dashboard_esp32");
        WiFi.softAPdisconnect();
        state = CONFIG_STATE_WAIT_ACTION;
        break;

    case CONFIG_STATE_CONNECTION_FAILED:
        // restart wifi config
        Serial.print("CONFIG_STATE_CONNECTION_FAILED");
        WiFi.disconnect();
        state = CONFIG_STATE_AP_STARTED;
        break;

    case CONFIG_STATE_STRAVA_START_AUTH:
        // Serial.print("http://www.strava.com/oauth/authorize?client_id=51666&response_type=code&redirect_uri=http://");
        // Serial.print("dashboard_esp32.local");
        // Serial.println("/exchange_token&approval_prompt=force&scope=activity:read_all");
        state = CONFIG_STATE_STRAVA_WAIT_AUTH;
        break;
    case CONFIG_STATE_STRAVA_WAIT_AUTH:
        break;

    case CONFIG_STATE_STRAVA_AUTH_CODE_RECEIVED:
        server.end();
        Serial.println("Server off");
        while (!getRefreshToken())
            ;
        state = CONFIG_STATE_SERVER_OFF;
        break;
    case CONFIG_STATE_STRAVA_AUTH_FAILED:
        Serial.println("CONFIG_STATE_STRAVA_AUTH_FAILED");
        state = CONFIG_STATE_STRAVA_START_AUTH;
        break;
    case CONFIG_STATE_SERVER_OFF:
        l_ret = true;
        delay(10);
        break;

    case CONFIG_STATE_WAIT_ACTION:
        break;

    default:
        break;
    }
    return l_ret;
}

void webroutes()
{

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        Serial.println("request on /");
        if(WiFi.isConnected()){
            String htmlPage = homeSTAStart;
            htmlPage += "http://www.strava.com/oauth/authorize?client_id=";
            htmlPage += clientId;
            htmlPage +="&response_type=code&redirect_uri=http://dashboard_esp32.local/exchange_token&approval_prompt=force&scope=activity:read_all";
            htmlPage+= homeSTAEnd;
            request->send(200, "text/html", htmlPage);
        }else{
        request->send(200, "text/html", homeAP); 
        } });

    server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        state = CONFIG_STATE_CLIENT_CONNECTED_WIFI_URL;
        Serial.println("request on /wifi");
        if (!lockScan)
        {
            Serial.println("scan OK");
            lockScan = true;
            request->send(200, "text/html", buildResponseHome());
            lockScan = false;
        }
        else
        {
            Serial.println("refresh page");
            request->send(200, "text/html", refreshPage);
        } });

    server.on("/exchange_token", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        if (request->hasParam("code")) {
            code = request->getParam("code")->value();
            Serial.println("code = " + code);
            Serial.print("scope : ");
            Serial.println(request->getParam("scope")->value());
            request->send(200, "text/html", configOK);
            state = CONFIG_STATE_STRAVA_AUTH_CODE_RECEIVED;
        } else {
            Serial.println("no code");
            request->send(200, "text/html", configKO);
            state = CONFIG_STATE_STRAVA_AUTH_FAILED;
        } });

    AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/connect", [](AsyncWebServerRequest *request, JsonVariant json)
                                                                           {
                                                                               JsonDocument jsonObj = json.as<JsonObject>();
                                                                               Serial.println("in callback");
                                                                               newSSID = json["ssid"].as<String>();
                                                                               newPasswd = json["passwd"].as<String>();
                                                                               Serial.print(newSSID);
                                                                               Serial.println(newPasswd); 
                                                                               state = CONFIG_STATE_CREDENTIALS_ENTERED; });
    server.addHandler(handler);
}

String buildResponseHome()
{
    String ret = home;
    ret += "<p>" + String(nbNetwork) + " networks founds :</p>\n";

    for (uint8_t i = 0; i < nbNetwork; i++)
    {
        ret += "<p><button onclick=\"myFunction('" + WiFi.SSID(i) + "')\" class=\"button\">" + WiFi.SSID(i) + "</button></a></p>\n";
    }
    return ret;
}

bool getRefreshToken()
{
    bool ret = false;
    Serial.println("get refresh token");

    HTTPClient http;
    String url = "https://www.strava.com/oauth/token?client_id=51666&client_secret=bea36fb9fdb9ca3466f93c4d6dd4e77fbad770d3&code=" + code + "&grant_type=authorization_code";
    http.begin(url);

    int httpResponseCode = http.POST("");

    if (httpResponseCode == 200)
    {
        String resp = http.getString();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, resp.c_str());
        if (error)
        {
            Serial.print("GET ACCESS TOKEN deserializeJson() returned ");
            Serial.println(error.c_str());
        }
        else
        {

            const char *refreshToken = doc["refresh_token"];
            Serial.println("Refresh Token OK");

            configPreferences.begin("stravaAuth");
            configPreferences.putString("refreshToken", refreshToken);
            configPreferences.end();
            ret = true;
        }
    }
    return ret;
}