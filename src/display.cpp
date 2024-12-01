#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "time.h"
#include "strava.h"

#include "display.h"

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void initDisplay(void)
{
    // init display
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }
    display.clearDisplay();

    display.setTextColor(WHITE, BLACK);
    display.setTextSize(1);
    display.setCursor(0, 0);
}

void displayTime(struct tm *now)
{

    display.setCursor(0, 0);
    display.print(" ");
    display.print(now->tm_hour);
    display.print(":");
    if (now->tm_min < 10)
    {
        display.print("0");
    }
    display.print(now->tm_min);
    display.print(" -- ");
    if (now->tm_mday < 10)
    {
        display.print("0");
    }
    display.print(now->tm_mday);
    display.print("/");
    if (now->tm_mon < 10)
    {
        display.print("0");
    }
    display.print(now->tm_mon + 1);
    display.print("/");
    display.println(now->tm_year + 1900);
    display.display();
}

void displayStravaAllYear()
{
    displayClearContent();
    display.setCursor(0, 20);
    display.println("2024   Total   2023\n");
    display.print((int)getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DISTANCE, 1, 0, DAYS_BY_YEAR - 1));
    display.print("km        ");
    display.print((int)getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DISTANCE, 0, 0, DAYS_BY_YEAR - 1));
    display.println("km");
    // display.print(getTotal(ACTIVITY_TYPE_RUN, true, 1, true));
    // display.println("m d+");
    display.print((int)getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DISTANCE, 1, 0, DAYS_BY_YEAR - 1));
    display.print("km       ");
    display.print((int)getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DISTANCE, 0, 0, DAYS_BY_YEAR - 1));
    display.println("km");
    // display.print(getTotal(ACTIVITY_TYPE_BIKE, true, 1, true));
    // display.println("m d+");
    display.display();
}

void displayStravaYTD()
{
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    displayClearContent();
    display.setCursor(0, 20);
    display.println("2024    YTD   2023\n");
    display.print((int)getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DISTANCE, 1, 0, timeinfo.tm_yday));
    display.print("km        ");
    display.print((int)getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DISTANCE, 0, 0, timeinfo.tm_yday));
    display.println("km");
    // display.print(getTotal(ACTIVITY_TYPE_RUN, true, 1, true));
    // display.println("m d+");
    display.print((int)getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DISTANCE, 1, 0, timeinfo.tm_yday));
    display.print("km       ");
    display.print((int)getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DISTANCE, 0, 0, timeinfo.tm_yday));
    display.println("km");
    // display.print(getTotal(ACTIVITY_TYPE_BIKE, true, 1, true));
    // display.println("m d+");
    display.display();
}

void displayStravaCurrentWeek()
{
    struct tm timeinfo;
    getLocalTime(&timeinfo);

    uint16_t startDay = timeinfo.tm_yday - (timeinfo.tm_wday + 6) % 7;
    uint16_t endDay = timeinfo.tm_yday;
    displayClearContent();
    display.setCursor(0, 20);
    display.println("This week\n");
    display.print(getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DISTANCE, 1, startDay, endDay));
    display.print("km   ");
    display.print((int)getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DENIV, 1, startDay, endDay));
    display.println("m d+");
    display.print(getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DISTANCE, 1, startDay, endDay));
    display.print("km   ");
    display.print((int)getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DENIV, 1, startDay, endDay));
    display.println("m d+");
    display.display();
}

void displayStravaToday()
{
    struct tm timeinfo;
    getLocalTime(&timeinfo);

    uint16_t startDay = timeinfo.tm_yday;
    uint16_t endDay = timeinfo.tm_yday;
    float distBike = getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DISTANCE, 1, startDay, endDay);
    float distRun = getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DISTANCE, 1, startDay, endDay);

    displayClearContent();
    display.setCursor(0, 20);
    display.println("Today\n");
    if (distRun > 0.1)
    {
        display.print(distRun);
        display.print("km   ");
        display.print((int)getTotal(ACTIVITY_TYPE_RUN, DATA_TYPE_DENIV, 1, startDay, endDay));
        display.println("m d+");
    }
    if (distBike > 0.1)
    {
        display.print(distBike);
        display.print("km   ");
        display.print((int)getTotal(ACTIVITY_TYPE_BIKE, DATA_TYPE_DENIV, 1, startDay, endDay));
        display.println("m d+");
    }
    display.display();
}

void displayText(const char msg[])
{
    display.println(msg);
    display.display();
}

void displayClearContent(void)
{
    uint8_t x = 0, y = 7, w = 128 - x, h = 64 - y;
    display.fillRect(x, y, w, h, BLACK);
    display.display();
}