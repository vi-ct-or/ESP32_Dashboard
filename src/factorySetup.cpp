
#include "Arduino.h"
#include "dataSave.h"
#include "factorySetup.h"
#include "Preferences.h"
#include "credentials.h"

void FactorySteup_InitEEPROM()
{
    Preferences preferences5;
    // Initialize EEPROM
    DataSave_EraseEEPROM();
    DataSave_SaveWifiCredentials();
    DataSave_ResetOTA();

    // init clientID,...
    preferences5.begin("stravaDB", false);
    preferences5.putLong64("clientId", clientId);
    preferences5.putString("clientSecret", clientSecret);
    preferences5.putString("apiRefreshToken", apiRefreshToken);
    preferences5.putLong("lastDayPopulate", 0);
    preferences5.end();
}