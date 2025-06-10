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
#include "dataSave.h"
#include "factorySetup.h"

#define NB_MENU 5
#define uS_TO_S_FACTOR 1000000ULL
#define mS_TO_S_FACTOR 1000ULL
#define DEEPSLEEP_TIME 50
#define WDT_TIMEOUT 60

#define REFRESH_HOUR 3 // hour to refresh the display

typedef enum eTimeSource
{
  TIME_SOURCE_NONE,
  TIME_SOURCE_GPS,
  TIME_SOURCE_NTP,
} TeTimeSource;

/****** NTP settings ******/
const char *NTP_SERVER = "pool.ntp.org";
// your time zone (https://remotemonitoringsystems.ca/time-zone-abbreviations.php)
const char *TZ_INFO = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";
struct tm timeinfo1;
RTC_DATA_ATTR uint8_t prevMinute;
RTC_DATA_ATTR uint8_t prevHour;
RTC_DATA_ATTR uint8_t prevDay;
RTC_DATA_ATTR uint8_t prevMonth;
RTC_DATA_ATTR TeTimeSource timeSource = TIME_SOURCE_NONE;

TaskHandle_t TimeTaskHandle, DisplayTaskHandle, StravaTaskHandle;
TickType_t TimeTaskDelay = pdMS_TO_TICKS(100);

void TimeTaskFunction(void *parameter);
void displayTaskFunction(void *parameter);
bool readyToGoToSleep();

uint16_t prevYear;
Preferences preferences2;
const int buttonPin = 0;
esp_sleep_wakeup_cause_t wakeup_reason;
bool goToSleep = false;
bool GPSSync = false;
bool rtcWasAvailable = true;

uint8_t taskFinishedCnt = 0;

// static void IRAM_ATTR buttonInterrupt(void);
void syncNTP();
void cbSyncTime(struct timeval *tv);

void setup()
{
  Serial.begin(9600);
  Serial.println("START");

  // nvs_flash_erase(); // erase the NVS partition and...
  // nvs_flash_init();  // initialize the NVS partition.
  // Serial.println("nvs erased");

  // FactorySteup_InitEEPROM();
  // DataSave_RetrieveWifiCredentials();
  // Serial.println("eeprom erased");

  // while (true)
  //   ;

  // esp_task_wdt_init(WDT_TIMEOUT, true); // Initialize ESP32 Task WDT
  // esp_task_wdt_add(NULL);               // Subscribe to the Task WDT

  pinMode(2, OUTPUT);
  //  digitalWrite(2, HIGH);
  sntp_set_time_sync_notification_cb(cbSyncTime);

  wakeup_reason = esp_sleep_get_wakeup_cause();
  // attachInterrupt(buttonPin, buttonInterrupt, FALLING);

  // initGpsTime();
  initDisplay();

  if (wakeup_reason != ESP_SLEEP_WAKEUP_TIMER)
  {
    // first boot
    // Serial.println("wakeup after reset");
    // Serial.println(wakeup_reason);

    // FactorySetup_ResetActivities();
    // Serial.println("activities reset");

    displayTemplate();
    DataSave_RetreiveLastActivity();
    initDB();

    prevMinute = 255;
    prevHour = 255;
    prevDay = 255;

    // esp_task_wdt_reset();
  }
  else
  {

    if (timeSource == TIME_SOURCE_NTP)
    {
      configTzTime(TZ_INFO, NTP_SERVER);
    }
    // Serial.println("wakeup after deepsleep timer");
  }
  // esp_task_wdt_reset();

  xSemaphore = xSemaphoreCreateCounting(3, 0);

  xQueueDisplay = xQueueCreate(10, sizeof(TeDisplayMessage));
  xQueueStrava = xQueueCreate(10, sizeof(TeStravaMessage));

  // start task
  xTaskCreatePinnedToCore(
      TimeTaskFunction, /* Function to implement the task */
      "TimeTask",       /* Name of the task */
      8192,             /* Stack size in words */
      NULL,             /* Task input parameter */
      5,                /* Priority of the task */
      &TimeTaskHandle,  /* Task handle. */
      0);               /* Core where the task should run */

  xTaskCreatePinnedToCore(
      displayTaskFunction, /* Function to implement the task */
      "DisplayTask",       /* Name of the task */
      8192,                /* Stack size in words */
      &taskFinishedCnt,    /* Task input parameter */
      5,                   /* Priority of the task */
      &DisplayTaskHandle,  /* Task handle. */
      1);                  /* Core where the task should run */
  xTaskCreatePinnedToCore(
      StravaTaskFunction, /* Function to implement the task */
      "StravaTask",       /* Name of the task */
      8192,               /* Stack size in words */
      &taskFinishedCnt,   /* Task input parameter */
      5,                  /* Priority of the task */
      &StravaTaskHandle,  /* Task handle. */
      1);                 /* Core where the task should run */
}

void loop()
{
}

void syncNTP()
{
  setenv("TZ", TZ_INFO, 1); // Set environment variable with your time zone - causes NTP call
  tzset();
}

void cbSyncTime(struct timeval *tv)
{ // callback function to show when NTP was synchronized
  Serial.println("callback NTP time now synched");
  setRtcTime();
  goToSleep = true;
}

void TimeTaskFunction(void *parameter)
{
  unsigned long timeToTimeTask = millis();
  Serial.print("TimeTaskFunction started at ");
  Serial.println(timeToTimeTask);
  TeDisplayMessage queueDisplayMessage;
  TeStravaMessage queueStravaMessage;
  while (true)
  {
    queueDisplayMessage = DISPLAY_MESSAGE_NONE;
    queueStravaMessage = STRAVA_MESSAGE_NONE;

    if (getLocalTime(&timeinfo1))
    {
      if (timeinfo1.tm_min != prevMinute)
      {
        Serial.println("update minute");
        prevMinute = timeinfo1.tm_min;
        // displayTime(&timeinfo1);
        queueDisplayMessage = DISPLAY_MESSAGE_TIME;
        xQueueSendToFront(xQueueDisplay, &queueDisplayMessage, 0);
        Serial.print(timeinfo1.tm_hour);
        Serial.print(":");
        Serial.println(timeinfo1.tm_min);
        goToSleep = true;
        if (timeinfo1.tm_min == 00 /*|| timeinfo1.tm_min == 14 || timeinfo1.tm_min == 29 || timeinfo1.tm_min == 44*/)
        {
          if (timeSource == TIME_SOURCE_NTP && connectWifi(10000))
          {
            configTzTime(TZ_INFO, NTP_SERVER);
            sntp_restart();
            Serial.println("sntp synchro...");
            goToSleep = false;
          }
        }
        if ((timeinfo1.tm_hour == REFRESH_HOUR) && timeinfo1.tm_min == 0 && timeinfo1.tm_sec <= 48)
        {
          DataSave_RetreiveLastActivity();
          initDB();
          queueDisplayMessage = DISPLAY_MESSAGE_REFRESH;
          xQueueSend(xQueueDisplay, &queueDisplayMessage, 0);
        }
      }
      if (timeinfo1.tm_mday != prevDay)
      {
        // Serial.println("update day");

        prevDay = timeinfo1.tm_mday;

        queueDisplayMessage = DISPLAY_MESSAGE_DATE;
        xQueueSend(xQueueDisplay, &queueDisplayMessage, 0);
        // displayDate(&timeinfo1);
      }
      if (timeinfo1.tm_mon != prevMonth)
      {
        // Serial.println("update month");
        // newMonthBegin(prevMonth, timeinfo1);
        queueStravaMessage = STRAVA_MESSAGE_NEW_MONTH;
        xQueueSend(xQueueStrava, &queueStravaMessage, 0);

        preferences2.begin("date", false);
        prevMonth = timeinfo1.tm_mon;
        preferences2.putUShort("prevMonth", prevMonth);
        preferences2.end();
      }
      if (timeinfo1.tm_hour != prevHour)
      {
        Serial.println("update hour");
        prevHour = timeinfo1.tm_hour;

        // here populate
        queueStravaMessage = STRAVA_MESSAGE_POPULATE;
        xQueueSend(xQueueStrava, &queueStravaMessage, 0);
      }
      if (!rtcWasAvailable)
      {
        rtcWasAvailable = true;
        queueDisplayMessage = DISPLAY_MESSAGE_TOTAL_YEAR;
        xQueueSend(xQueueDisplay, &queueDisplayMessage, 0);
      }
    }
    else if (wakeup_reason != ESP_SLEEP_WAKEUP_TIMER)
    {
      // first boot
      if (rtcAvailable())
      {
        rtcWasAvailable = true;
        Serial.println("RTC available");
        adjustLocalTimeFromRtc();
        queueDisplayMessage = DISPLAY_MESSAGE_TIME;
        xQueueSend(xQueueDisplay, &queueDisplayMessage, 0);
        queueDisplayMessage = DISPLAY_MESSAGE_DATE;
        xQueueSend(xQueueDisplay, &queueDisplayMessage, 0);
        queueDisplayMessage = DISPLAY_MESSAGE_TOTAL_YEAR;
        xQueueSend(xQueueDisplay, &queueDisplayMessage, 0);
        if (getLocalTime(&timeinfo1) && timeinfo1.tm_min % 2 == 0)
        {
          queueDisplayMessage = DISPLAY_MESSAGE_MONTHS;
        }
        else
        {
          queueDisplayMessage = DISPLAY_MESSAGE_WEEKS;
        }
        prevMinute = timeinfo1.tm_min;
        prevDay = timeinfo1.tm_mday;
      }
      else
      {
        rtcWasAvailable = false;
      }

      queueDisplayMessage = DISPLAY_MESSAGE_LAST_ACTIVITY;
      xQueueSend(xQueueDisplay, &queueDisplayMessage, 0);
      queueDisplayMessage = DISPLAY_MESSAGE_POLYLINE;
      xQueueSend(xQueueDisplay, &queueDisplayMessage, 0);

      if (connectWifi(10000))
      {
        goToSleep = false;
      }
      configTzTime(TZ_INFO, NTP_SERVER);
      timeSource = TIME_SOURCE_NTP;

      // if (connectWifi(10000))
      // {
      //   updateFW();
      // }

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

    if (timeinfo1.tm_sec >= 58)
    {
      goToSleep = false;
    }
    if (goToSleep && readyToGoToSleep())
    {

      // esp_task_wdt_reset();
      uint32_t sleepTime = (60 * uS_TO_S_FACTOR) - ((timeToTimeTask + 100) * mS_TO_S_FACTOR);

      if (timeSource == TIME_SOURCE_GPS)
      {
        sleepTime = 54;
      }
      adjustLocalTimeFromRtc();
      getLocalTime(&timeinfo1);
      if (timeinfo1.tm_sec < sleepTime - 1 && timeinfo1.tm_min == prevMinute)
      {
        esp_sleep_enable_timer_wakeup(sleepTime - (timeinfo1.tm_sec * uS_TO_S_FACTOR));
        Serial.print("Going to sleep for ");
        Serial.print(sleepTime - (timeinfo1.tm_sec * uS_TO_S_FACTOR));
        Serial.println(" us");
        Serial.flush();
        // digitalWrite(2, LOW);
        esp_deep_sleep_start();
      }
    }

    // esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  vTaskDelete(NULL);
}

bool readyToGoToSleep()
{
  bool l_ret = false;
  if (uxQueueMessagesWaiting(xQueueStrava) == 0 && uxQueueMessagesWaiting(xQueueDisplay) == 0 && taskFinishedCnt == 0)
  {
    Serial.println("Ready to go to sleep !");
    l_ret = true;
  }
  else
  {
    // Serial.println("NOT ready to go to sleep !");
  }
  return l_ret;
}