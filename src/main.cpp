#include <Arduino.h>
#include <Preferences.h>
#include <esp_task_wdt.h>
#include <nvs_flash.h>
#include <sys/time.h>

#include "esp_sntp.h"
#include "otaUpdate.h"
#include "strava.h"
#include "displayEpaper.h"
#include "network.h"
#include "GPSTime.h"

#define NB_MENU 5
#define uS_TO_S_FACTOR 1000000ULL
#define DEEPSLEEP_TIME 50
#define WDT_TIMEOUT 60

/****** NTP settings ******/
const char *NTP_SERVER = "pool.ntp.org";
// your time zone (https://remotemonitoringsystems.ca/time-zone-abbreviations.php)
const char *TZ_INFO = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";
struct tm timeinfo1;
RTC_DATA_ATTR uint8_t prevMinute;
RTC_DATA_ATTR uint8_t prevHour;
RTC_DATA_ATTR uint8_t prevDay;
RTC_DATA_ATTR uint8_t prevMonth;

uint16_t prevYear;
Preferences preferences2;
const int buttonPin = 0;
esp_sleep_wakeup_cause_t wakeup_reason;
bool goToSleep = false;
RTC_DATA_ATTR bool doSyncNtp = false;
bool GPSSync = false;

// static void IRAM_ATTR buttonInterrupt(void);
void syncNTP();
void cbSyncTime(struct timeval *tv);

void setup()
{
  Serial.begin(9600);

  // nvs_flash_erase(); // erase the NVS partition and...
  // nvs_flash_init();  // initialize the NVS partition.
  // Serial.println("nvs erased");
  // while (true)
  //   ;
  Serial.println("START");

  esp_task_wdt_init(WDT_TIMEOUT, true); // Initialize ESP32 Task WDT
  esp_task_wdt_add(NULL);               // Subscribe to the Task WDT

  pinMode(2, OUTPUT);
  //  digitalWrite(2, HIGH);
  sntp_set_time_sync_notification_cb(cbSyncTime);

  wakeup_reason = esp_sleep_get_wakeup_cause();
  // attachInterrupt(buttonPin, buttonInterrupt, FALLING);

  initWifi();
  initDisplay();
  initGpsTime();

  if (setGPSTime())
  {
    setenv("TZ", TZ_INFO, 1);
    tzset();
    Serial.println("Set By GPS");
    GPSSync = true;
  }
  else
  {
    if (!getLocalTime(&timeinfo1))
    {
      connectWifi(10000);
    }
    configTzTime(TZ_INFO, NTP_SERVER);
    if (doSyncNtp && connectWifi(10000))
    {
      sntp_restart();
      doSyncNtp = false;
    }
  }
  while (!getLocalTime(&timeinfo1))
  {
    esp_task_wdt_reset();
  }
  if (wakeup_reason != ESP_SLEEP_WAKEUP_TIMER)
  {
    // first boot
    // Serial.println("wakeup after reset");
    // Serial.println(wakeup_reason);

    // preferences2.begin("stravaDB", false);
    // preferences2.clear();
    // preferences2.end();

    if (connectWifi(10000))
    {
      updateFW();
    }

    esp_task_wdt_reset();

    prevMinute = 255;
    prevHour = 255;
    prevDay = 255;
    preferences2.begin("date", false);
    prevMonth = preferences2.getUShort("prevMonth", 255);
    if (prevMonth == 255)
    {
      prevMonth = timeinfo1.tm_mon;
      preferences2.putUShort("prevMonth", prevMonth);
      // Serial.println("prevMonth saved in flash");
    }
    preferences2.end();
  }
  else
  {
    // Serial.println("wakeup after deepsleep timer");
  }
  displayTemplate();
  displayTimeSync(GPSSync);
  esp_task_wdt_reset();
}

void loop()
{
  // update time
  if (getLocalTime(&timeinfo1))
  {
    if (timeinfo1.tm_min != prevMinute || isRefreshed)
    {
      // Serial.println("update minute");
      if (timeinfo1.tm_min != prevMinute)
      {
        prevMinute = timeinfo1.tm_min;
      }
      displayTime(&timeinfo1);
      Serial.print(timeinfo1.tm_hour);
      Serial.print(":");
      Serial.println(timeinfo1.tm_min);
      if (timeinfo1.tm_min == 59 || timeinfo1.tm_min == 14 || timeinfo1.tm_min == 29 || timeinfo1.tm_min == 44)
      {
        doSyncNtp = true;
      }
      if ((timeinfo1.tm_hour == 10 || timeinfo1.tm_hour == 20) && timeinfo1.tm_min == 59 && timeinfo1.tm_sec <= 5)
      {
        refresh = true; // refresh screen next time
      }
      goToSleep = true;
    }
    if (timeinfo1.tm_mday != prevDay || isRefreshed)
    {
      // Serial.println("update day");
      if (timeinfo1.tm_mday != prevDay)
      {
        prevDay = timeinfo1.tm_mday;
      }
      displayDate(&timeinfo1);
    }
    if (timeinfo1.tm_mon != prevMonth)
    {
      // Serial.println("update month");
      newMonthBegin(prevMonth, timeinfo1);
      preferences2.begin("date", false);
      prevMonth = timeinfo1.tm_mon;
      preferences2.putUShort("prevMonth", prevMonth);
      preferences2.end();
    }
    if (timeinfo1.tm_hour != prevHour || isRefreshed)
    {
      // Serial.println("update hour");
      if (timeinfo1.tm_hour == 10)
      {
        if (connectWifi(10000))
        {
          updateFW();
        }
      }
      if (timeinfo1.tm_hour != prevHour)
      {
        prevHour = timeinfo1.tm_hour;
      }
      initDB();
      if (connectWifi(10000))
      {
        esp_task_wdt_reset();
        populateDB();
        esp_task_wdt_reset();
      }
      if (newActivity || isRefreshed)
      {
        displayStravaAllYear(&timeinfo1);
        displayStravaPolyline();
        newActivity = false;
      }
      displayLastActivity();
      if (timeinfo1.tm_hour % 2 == 0)
      {
        displayStravaMonths(&timeinfo1);
      }
      else if (timeinfo1.tm_hour % 2 == 1)
      {
        displayStravaWeeks(&timeinfo1);
      }
      isRefreshed = false;
      // prevMenu = UINT8_MAX; // to refresh current menu with new activity
    }
  }
  if (goToSleep)
  {
    esp_task_wdt_reset();
    uint8_t sleepTime = 53;
    if (refresh || doSyncNtp)
    {
      sleepTime = 48;
    }
    getLocalTime(&timeinfo1);
    if (timeinfo1.tm_sec < sleepTime - 1 && timeinfo1.tm_min == prevMinute)
    {
      esp_sleep_enable_timer_wakeup((sleepTime - timeinfo1.tm_sec) * uS_TO_S_FACTOR);
      // Serial.print("Going to sleep for ");
      // Serial.print(sleepTime - timeinfo1.tm_sec);
      // Serial.println("seconds");
      Serial.flush();
      // digitalWrite(2, LOW);
      esp_deep_sleep_start();
    }
    goToSleep = false;
  }

  esp_task_wdt_reset();
  delay(100);
}

// static void IRAM_ATTR buttonInterrupt(void)
// {
//   // static unsigned long last_interrupt_time = 0;
//   // unsigned long interrupt_time = millis();
//   // // If interrupts come faster than 500ms, assume it's a bounce and ignore
//   // if ((interrupt_time - last_interrupt_time > 500) || last_interrupt_time > interrupt_time)
//   // {

//   //   currentMenu++;
//   //   if (currentMenu >= NB_MENU)
//   //   {
//   //     currentMenu = 0;
//   //   }
//   //   last_interrupt_time = interrupt_time;
//   // }
// }

void syncNTP()
{
  setenv("TZ", TZ_INFO, 1); // Set environment variable with your time zone - causes NTP call
  tzset();
}

void cbSyncTime(struct timeval *tv)
{ // callback function to show when NTP was synchronized
  // Serial.println("callback NTP time now synched");
}