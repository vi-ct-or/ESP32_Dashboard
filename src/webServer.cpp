#include <WiFi.h>

WiFiServer server(80);

void initWebServer()
{
    WiFi.disconnect();
    WiFi.mode(WIFI_MODE_AP);
    WiFi.softAP("Dashboard_Strava", "defaultPassword");
    server.begin();
}