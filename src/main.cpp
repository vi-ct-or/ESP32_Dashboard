#include <Arduino.h>
#include "credentials.h"
#include "WiFi.h"
#include "otaUpdate.h"
#include "strava.h"
#include "display.h"
#include <Preferences.h>

#define NB_MENU 5

/****** NTP settings ******/
const char *NTP_SERVER = "pool.ntp.org";
// your time zone (https://remotemonitoringsystems.ca/time-zone-abbreviations.php)
const char *TZ_INFO = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";
struct tm timeinfo1;
uint8_t prevMinute = 255;
uint8_t prevHour = 255;
uint8_t prevMenu = UINT8_MAX, currentMenu = 0;
uint16_t prevYear;
Preferences preferences2;
const int buttonPin = 0;

static void IRAM_ATTR buttonInterrupt(void);

void setup()
{
  Serial.begin(9600);
  initDisplay();
  attachInterrupt(buttonPin, buttonInterrupt, FALLING);

  WiFi.begin(wifiSsid, wifiPswd);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }
  Serial.println("connected");
  displayText("Connected to wifi");

  configTzTime(TZ_INFO, NTP_SERVER);
  while (!getLocalTime(&timeinfo1))
    ;
  updateFW();
  displayClearContent();

  preferences2.begin("date", false);
  prevYear = preferences2.getUShort("prevYear", timeinfo1.tm_year + 1900);
  preferences2.end();
}

void loop()
{
  // update time
  getLocalTime(&timeinfo1);
  if (timeinfo1.tm_min != prevMinute && currentMenu != 4)
  {
    prevMinute = timeinfo1.tm_min;
    displayTime(&timeinfo1);
  }
  if (timeinfo1.tm_year + 1900 == prevYear + 1)
  {
    newYearBegin();
    preferences2.begin("date", false);
    prevYear = timeinfo1.tm_year + 1900;
    preferences2.putUShort("prevYear", prevYear);
    preferences2.end();
  }
  if (timeinfo1.tm_hour != prevHour)
  {
    prevHour = timeinfo1.tm_hour;
    populateDB();
    prevMenu = UINT8_MAX; // to refresh current menu with new activity
  }

  // update menu if button pressed
  if (prevMenu != currentMenu)
  {
    prevMenu = currentMenu;

    switch (currentMenu)
    {
    case 0: // all year
      displayStravaAllYear();
      break;

    case 1: // ytd
      displayStravaYTD();
      break;

    case 2: // this week
      displayStravaCurrentWeek();
      break;

    case 3: // today
      displayStravaToday();
      break;

    case 4:
      displayStravaPolyline();
      break;

    default:
      break;
    }
  }

  delay(1000);
}

static void IRAM_ATTR buttonInterrupt(void)
{
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 5s, assume it's a bounce and ignore
  if ((interrupt_time - last_interrupt_time > 500) || last_interrupt_time > interrupt_time)
  {

    currentMenu++;
    if (currentMenu >= NB_MENU)
    {
      currentMenu = 0;
    }
    last_interrupt_time = interrupt_time;
  }
}