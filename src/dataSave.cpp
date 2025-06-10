// #include <Arduino.h>
#include <JC_EEPROM.h>
#include "credentials.h"
#include "dataSave.h"
#include "strava.h"
#include "otaUpdate.h"

JC_EEPROM eep(JC_EEPROM::kbits_32, 1, 32, 0x57); // 24C32

/* DATA SAVED

-- Wifi Credentials :
Nb of WIFI 1 byte   // addr 0
Wifi SSID 1 -> 32 bytes // addr 1-32
Wifi Password 1-> 32 bytes       // addr 33-64
WIfi SSID 2 -> 32 bytes // addr 65-96
Wifi Password 2 -> 32 bytes // addr 97-128
WIfi SSID 3 -> 32 bytes // addr 129-160
Wifi Password 3 -> 32 bytes // addr 161-192

-- OTA : -> 1 byte
lastVersion -> 1 byte // addr 500

lastActivity -> 87 bytes // 501


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

    uint32_t offset = 0;
    eep.read(offset, (uint8_t *)&nbWifi, sizeof(nbWifi));
    offset += sizeof(nbWifi);
    eep.read(offset, (uint8_t *)&wifiSsid0, sizeof(wifiSsid0));
    offset += sizeof(wifiSsid0);
    eep.read(offset, (uint8_t *)&wifiPswd0, sizeof(wifiPswd0));
    offset += sizeof(wifiPswd0);
    eep.read(offset, (uint8_t *)&wifiSsid1, sizeof(wifiSsid1));
    offset += sizeof(wifiSsid1);
    eep.read(offset, (uint8_t *)&wifiPswd1, sizeof(wifiPswd1));
    offset += sizeof(wifiPswd1);
    eep.read(offset, (uint8_t *)&wifiSsid2, sizeof(wifiSsid2));
    offset += sizeof(wifiSsid2);
    eep.read(offset, (uint8_t *)&wifiPswd2, sizeof(wifiPswd2));
    offset += sizeof(wifiPswd2);

    Serial.print("nbWifi : ");
    Serial.println(nbWifi);
    Serial.print("wifiSsid0 : ");
    Serial.println(wifiSsid0);
    Serial.print("wifiSsid1 : ");
    Serial.println(wifiSsid1);
    Serial.print("wifiSsid2 : ");
    Serial.println(wifiSsid2);
}

void DataSave_RetreiveLastActivity()
{
    uint32_t offset = 0;
    DataSave_Init();
    // Retrieve last activity data
    offset = 1000;

    // retrieve last activities list
    eep.read(offset, (uint8_t *)&lastActivitiesId, sizeof(lastActivitiesId));
    offset += sizeof(lastActivitiesId);
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
    do
    {
        eep.read(offset, (uint8_t *)&charPolyline, sizeof(charPolyline));
        polyline += charPolyline;
        offset += sizeof(charPolyline);
        if (offset > 4000)
        {
            Serial.println("Error : polyline too long");
            break;
        }
    } while (charPolyline != '\0');

    lastActivity->polyline = polyline;

    Serial.println("end of DataSave_RetreiveLastactivity");
}

void DataSave_RetrieveOTAData()
{
    DataSave_Init();
    // Retrieve OTA data
    uint32_t offset = 500; // offset for OTA data
    eep.read(offset, (uint8_t *)&currentVersion, sizeof(currentVersion));
}

void DataSave_SaveLastActivity()
{
    DataSave_Init();
    // Save strava data
    uint32_t offset = 1000;
    eep.write(offset, (uint8_t *)&lastActivitiesId, sizeof(lastActivitiesId));
    offset += sizeof(lastActivitiesId);
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
    uint32_t offset = 500; // offset for OTA data
    eep.write(offset, (uint8_t *)&currentVersion, sizeof(currentVersion));
}

uint8_t DataSave_SaveWifiCredentials()
{
    DataSave_Init();
    uint8_t ret = 0;
    uint32_t offset = 0;

    ret = eep.write(offset, (uint8_t *)&nbWifi, sizeof(nbWifi));
    offset += sizeof(nbWifi);
    ret = eep.write(offset, (uint8_t *)&wifiSsid0, sizeof(wifiSsid0));
    offset += sizeof(wifiSsid0);
    ret = eep.write(offset, (uint8_t *)&wifiPswd0, sizeof(wifiPswd0));
    offset += sizeof(wifiPswd0);
    ret = eep.write(offset, (uint8_t *)&wifiSsid1, sizeof(wifiSsid1));
    offset += sizeof(wifiSsid1);
    ret = eep.write(offset, (uint8_t *)&wifiPswd1, sizeof(wifiPswd1));
    offset += sizeof(wifiPswd1);
    ret = eep.write(offset, (uint8_t *)&wifiSsid2, sizeof(wifiSsid2));
    offset += sizeof(wifiSsid2);
    ret = eep.write(offset, (uint8_t *)&wifiPswd2, sizeof(wifiPswd2));
    offset += sizeof(wifiPswd2);
    return ret;
}

uint8_t DataSave_ResetOTA()
{
    DataSave_Init();
    uint8_t ret = 0;
    uint32_t offset = 500; // offset for OTA data
    uint8_t versionZero = 0;
    ret = eep.write(offset, (uint8_t *)&versionZero, sizeof(versionZero));
    return ret;
}

void DataSave_EraseEEPROM()
{
    DataSave_Init();
    const uint16_t eepromSize = 4096; // 32kbit = 4096 bytes for 24C32
    uint8_t blank = 0x0;
    for (uint16_t addr = 0; addr < eepromSize; ++addr)
    {
        eep.write(addr, &blank, 1);
    }
    Serial.println("EEPROM erased.");
}

void DataSave_resetLastActivities()
{
    DataSave_Init();
    uint32_t offset = 1000;
    memset(&lastActivitiesId, 0, sizeof(lastActivitiesId));

    eep.write(offset, (uint8_t *)&lastActivitiesId, sizeof(lastActivitiesId));
    offset += sizeof(lastActivitiesId);

    // last Activity
    TsActivity *lastActivity = getStravaLastActivity();
    lastActivity->deniv = 0;
    lastActivity->dist = 0;
    lastActivity->time = 0;
    lastActivity->timestamp = 0;
    lastActivity->type = ACTIVITY_TYPE_UNKNOWN;
    lastActivity->kudos = 0;
    lastActivity->isFilled = false;
    memset(lastActivity->name, 0, sizeof(lastActivity->name));
    lastActivity->polyline.clear();
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
}