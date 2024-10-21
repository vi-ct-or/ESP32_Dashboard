#include <Arduino.h>
#include "credentials.h"
#include "HTTPClient.h"
#include "WiFi.h"

/****** NTP settings ******/
const char *NTP_SERVER = "pool.ntp.org";
// your time zone (https://remotemonitoringsystems.ca/time-zone-abbreviations.php)
const char *TZ_INFO = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";
tm timeinfo;

void setup()
{
  Serial.begin(9600);

  WiFi.begin(wifiSsid, wifiPswd);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }
  Serial.println("connected");

  configTzTime(TZ_INFO, NTP_SERVER);
  getLocalTime(&timeinfo);
}

void loop()
{
}