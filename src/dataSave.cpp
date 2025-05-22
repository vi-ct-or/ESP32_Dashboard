// #include <Arduino.h>
#include <JC_EEPROM.h>
#include "credentials.h"
#include "dataSave.h"
#include "strava.h"
#include "otaUpdate.h"

JC_EEPROM eep(JC_EEPROM::kbits_32, 1, 32, 0x57); // 24C32

/* DATA SAVED

-- Credentials : -> 154 bytes
Wifi SSID -> 32 bytes
Wifi Password -> 32 bytes

-- OTA : -> 1 byte
lastVersion -> 1 byte

lastActivity -> 87 bytes


*/

uint8_t DataSave_Init()
{

    static bool initialized = false;
    uint8_t ret = 0;
    if (!initialized)
    {
        ret = eep.begin();
        initialized = true;
    }
    return ret;
}

void DataSave_RetrieveWifiCredentials()
{
    DataSave_Init();
    static bool initialized = false;
    if (initialized)
    {
        return;
    }
    initialized = true;
    // Retrieve Credentials
    uint32_t offset = 1;
    eep.read(offset, (uint8_t *)&wifiSsid0, sizeof(wifiSsid0));
    Serial.print("wifiSsid0 : ");
    Serial.println(wifiSsid0);
    offset += sizeof(wifiSsid0);
    eep.read(offset, (uint8_t *)&wifiPswd0, sizeof(wifiPswd0));
    Serial.print("wifiPswd0 : ");
    Serial.println(wifiPswd0);
    offset += sizeof(wifiPswd0);
}

void DataSave_RetreiveLastActivity()
{
    uint32_t offset = 0;
    DataSave_Init();
    // Retrieve last activity data
    offset = 1000;
    // last Activity
    TsActivity *lastActivity = getStravaLastActivity();
    eep.read(offset, (uint8_t *)&lastActivity->deniv, sizeof(lastActivity->deniv));
    offset += sizeof(lastActivity->deniv);
    eep.read(offset, (uint8_t *)&lastActivity->dist, sizeof(lastActivity->dist));
    offset += sizeof(lastActivity->dist);
    eep.read(offset, (uint8_t *)&lastActivity->time, sizeof(lastActivity->time));
    offset += sizeof(lastActivity->time);
    eep.read(offset, (uint8_t *)&lastActivity->timestamp, sizeof(lastActivity->timestamp));
    offset += sizeof(lastActivity->timestamp);
    eep.read(offset, (uint8_t *)&lastActivity->type, sizeof(lastActivity->type));
    offset += sizeof(lastActivity->type);
    eep.read(offset, (uint8_t *)&lastActivity->kudos, sizeof(lastActivity->kudos));
    offset += sizeof(lastActivity->kudos);
    eep.read(offset, (uint8_t *)&lastActivity->isFilled, sizeof(lastActivity->isFilled));
    offset += sizeof(lastActivity->isFilled);
    eep.read(offset, (uint8_t *)&lastActivity->name, sizeof(lastActivity->name));
    offset += sizeof(lastActivity->name);
    Serial.print("lastActivity.name : ");
    Serial.println(lastActivity->name);
    std::string polyline = "";

    char charPolyline;
    uint16_t cnt = 0;
    do
    {
        eep.read(offset, (uint8_t *)&charPolyline, sizeof(charPolyline));
        polyline += charPolyline;
        offset += sizeof(charPolyline);
        cnt++;
        if (cnt > 1000)
        {
            Serial.println("Error : polyline too long");
            break;
        }
    } while (charPolyline != '\0');

    lastActivity->polyline = polyline;

    Serial.println("end of DataSave_RetreiveData");
}

void DataSave_RetrieveOTAData()
{
    DataSave_Init();
    // Retrieve OTA data
    eep.read(0, (uint8_t *)&currentVersion, sizeof(currentVersion));
}

void DataSave_SaveLastActivity()
{
    DataSave_Init();
    // Save strava data
    uint32_t offset = 1000;
    // last Activity
    TsActivity *lastActivity = getStravaLastActivity();
    eep.write(offset, (uint8_t *)&(lastActivity->deniv), sizeof(lastActivity->deniv));
    offset += sizeof(lastActivity->deniv);
    eep.write(offset, (uint8_t *)&lastActivity->dist, sizeof(lastActivity->dist));
    offset += sizeof(lastActivity->dist);
    eep.write(offset, (uint8_t *)&lastActivity->time, sizeof(lastActivity->time));
    offset += sizeof(lastActivity->time);
    eep.write(offset, (uint8_t *)&lastActivity->timestamp, sizeof(lastActivity->timestamp));
    offset += sizeof(lastActivity->timestamp);
    eep.write(offset, (uint8_t *)&lastActivity->type, sizeof(lastActivity->type));
    offset += sizeof(lastActivity->type);
    eep.write(offset, (uint8_t *)&lastActivity->kudos, sizeof(lastActivity->kudos));
    offset += sizeof(lastActivity->kudos);
    eep.write(offset, (uint8_t *)&lastActivity->isFilled, sizeof(lastActivity->isFilled));
    offset += sizeof(lastActivity->isFilled);
    eep.write(offset, (uint8_t *)&lastActivity->name, sizeof(lastActivity->name));
    offset += sizeof(lastActivity->name);
    const char *polyline = lastActivity->polyline.c_str();
    eep.write(offset, (uint8_t *)polyline, lastActivity->polyline.size() + 1);

    Serial.println("end of DataSave_SaveStravaData");
}

void DataSave_SaveOTAData()
{
    DataSave_Init();
    // Save OTA data
    eep.write(0, (uint8_t *)&currentVersion, sizeof(currentVersion));
}

uint8_t DataSave_SaveWifiCredentials()
{
    DataSave_Init();
    uint8_t ret = 0;
    uint32_t offset = 1;
    ret = eep.write(offset, (uint8_t *)&wifiSsid0, sizeof(wifiSsid0)) Ò;
    offset += sizeof(wifiSsid0);
    Ò
        ret = eep.write(offset, (uint8_t *)&wifiPswd0, sizeof(wifiPswd0));
    return ret;
}

uint8_t DataSave_ResetOTA()
{
    DataSave_Init();
    uint8_t ret = 0;
    uint32_t offset = 0;
    uint8_t versionZero = 2;
    ret = eep.write(offset, (uint8_t *)&versionZero, sizeof(versionZero));
    return ret;
}