#ifndef BATTERYMANAGER_H
#define BATTERYMANAGER_H

#include <Arduino.h>

int8_t getBatteryPercentage();
int8_t getPrevBatteryPercentage();
bool getBatteryStatus();

#endif // BATTERYMANAGER_H