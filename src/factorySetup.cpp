
#include "Arduino.h"
#include "dataSave.h"
#include "factorySetup.h"
#include "Preferences.h"
#include "credentials.h"
#include "strava.h"

void FactorySteup_InitEEPROM()
{
    Preferences preferences5;
    // Initialize EEPROM
    DataSave_EraseEEPROM();
    DataSave_SaveWifiCredentials();
    DataSave_SaveStravaCredentials();
    DataSave_ResetOTA();
}

void FactorySetup_ResetActivities()
{
    DataSave_resetLastActivities();
    resetDB();
}