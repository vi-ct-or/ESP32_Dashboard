#include "batteryManager.h"
#include "Adafruit_MAX1704X.h"

Adafruit_MAX17048 maxlipo;
int8_t batteryPercentage = -1;
RTC_DATA_ATTR int8_t prevBatteryPercentage = -1;

int8_t getBatteryPercentage()
{
    return batteryPercentage;
}

int8_t getPrevBatteryPercentage()
{
    int8_t ret = prevBatteryPercentage;
    prevBatteryPercentage = batteryPercentage;
    return ret;
}

bool getBatteryStatus()
{
    bool ret = false;
    uint8_t retryNb = 0;
    batteryPercentage = -1;

    while (!maxlipo.begin())
    {
        if (retryNb > 10)
        {
            Serial.println(F("Couldnt find Adafruit MAX17048?\nMake sure a battery is plugged in!"));
            return ret;
        }
        delay(200);
        retryNb++;
    }

    maxlipo.wake();
    maxlipo.sleep(false);

    float cellVoltage = maxlipo.cellVoltage();
    float cellPercent = maxlipo.cellPercent();
    if (isnan(cellVoltage))
    {
        Serial.println("Failed to read cell voltage, check battery is connected!");
    }
    else
    {
        ret = true;
        batteryPercentage = int(cellPercent);
    }

    Serial.print(F("Batt Voltage: "));
    Serial.print(cellVoltage, 3);
    Serial.println(" V");
    Serial.print(F("Batt Percent: "));
    Serial.print(cellPercent, 1);
    Serial.println(" %");
    Serial.println();
    maxlipo.hibernate();
    maxlipo.sleep(true);
    return ret;
}