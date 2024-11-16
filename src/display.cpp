#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "time.h"
#include "strava.h"

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

    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
}

void displayTime(struct tm *now)
{
    display.clearDisplay();

    display.setCursor(0, 0);
    display.print(now->tm_hour);
    display.print(":");
    if (now->tm_min < 10)
    {
        display.print("0");
    }
    display.print(now->tm_min);
    display.print("  -  ");
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
    display.print(now->tm_mon);
    display.print("/");
    display.println(now->tm_year + 1900);
    display.display();
}

void displayStrava()
{
    display.setCursor(0, 20);
    display.println("2024          2023");
    // display.print("Total run : ");
    display.print((int)getTotal(ACTIVITY_TYPE_RUN, false, 1, true));
    display.print("km        ");
    display.print((int)getTotal(ACTIVITY_TYPE_RUN, false, 0, true));
    display.println("km");
    // display.print(getTotal(ACTIVITY_TYPE_RUN, true, 1, true));
    // display.println("m d+");
    // display.print("Total bike : ");
    display.print((int)getTotal(ACTIVITY_TYPE_BIKE, false, 1, true));
    display.print("km       ");
    display.print((int)getTotal(ACTIVITY_TYPE_BIKE, false, 0, true));
    display.println("km");
    // display.print(getTotal(ACTIVITY_TYPE_BIKE, true, 1, true));
    // display.println("m d+");

    display.println("oui!");
    display.display();
}

void displayText(const char msg[])
{
    display.println(msg);
    display.display();
}