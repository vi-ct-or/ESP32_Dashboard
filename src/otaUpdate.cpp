#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "displayEpaper.h"

#include "otaUpdate.h"
#include "strava.h"
#include "RTCTime.h"
#include "dataSave.h"

#include <OTA-Hub.hpp>
#include <OTA-Hub/FOTA-providers/github.hpp>

#define OTAGH_OWNER_NAME "vi-ct-or"
#define OTAGH_REPO_NAME "ESP32_Dashboard"
#define OTAGH_BEARER "YOUR PRIVATE REPO TOKEN" // Follow the docs if using a private repo. Remove if repo is public.

TsVersion currentVersion;
TsVersion tmpInVersion = {0};

WiFiClientSecure wifi_client;
OTAHub::FOTA::GithubProvider provider(
    OTAGH_OWNER_NAME,
    OTAGH_REPO_NAME);

// Define the name for the downloaded firmware file
#define FILE_NAME "firmware.bin"

bool isNewUpdateAvailable(String incomingVersion);

void updateFW()
{
    // Initialise OTA
    // wifi_client.setCACert(OTAHub::certs::GITHUB_CA); // Set the api.github.cm SSL cert on the WiFi Client
    wifi_client.setInsecure(); // Set client to allow insecure connections

    OTAHub::FOTA::init(wifi_client, provider);

    Serial.print("current build time = ");
    Serial.println(cvtDate());

    // Check OTA for updates
    OTAHub::FOTA::UpdateObject details = OTAHub::FOTA::isUpdateAvailable();
    details.print();

    Serial.println(OTAHub::FOTA::ota_provider->FIRMWARE_WAS_BUILT_ON_PROVIDER
                       ? "This was built on Git."
                       : "This was built locally.");

    if (isNewUpdateAvailable(details.tag_name))
    {
        displayUpdating(0);
        displayUpdating(1);

        Serial.println("An update is available!");
        // Perform OTA update - will auto restart
        if (OTAHub::FOTA::performUpdate(&details, false) != OTAHub::FOTA::SUCCESS)
        {
            Serial.println("Update failed. Restarting...");
            displayUpdating(4);
        }
        else
        {
            Serial.println("Update successful. Restarting...");
            displayUpdating(5);
            currentVersion = tmpInVersion;
            DataSave_SaveOTAData();
            // resetDB();
        }
        resetClock();
        ESP.restart();
    }
    else
    {
        Serial.println("No new update available. Continuing...");
    }
}

bool isNewUpdateAvailable(String incomingVersion)
{
    bool ret = false;

    if (incomingVersion.isEmpty())
    {
        Serial.println("incoming version empty : " + incomingVersion);
        return ret;
    }

    DataSave_RetrieveOTAData();

    uint8_t tmpIndex = 0;
    tmpInVersion.major = incomingVersion.substring(tmpIndex, incomingVersion.indexOf(".")).toInt();
    tmpIndex = incomingVersion.indexOf(".") + 1;
    tmpInVersion.minor = incomingVersion.substring(tmpIndex, incomingVersion.indexOf(".", tmpIndex)).toInt();
    tmpIndex = incomingVersion.indexOf(".", tmpIndex) + 1;
    tmpInVersion.patch = incomingVersion.substring(tmpIndex, incomingVersion.indexOf(".", tmpIndex)).toInt();

    Serial.print("Current version : ");
    Serial.print(currentVersion.major);
    Serial.print(".");
    Serial.print(currentVersion.minor);
    Serial.print(".");
    Serial.print(currentVersion.patch);
    Serial.print(" ; githubVersion : ");
    Serial.print(tmpInVersion.major);
    Serial.print(".");
    Serial.print(tmpInVersion.minor);
    Serial.print(".");
    Serial.println(tmpInVersion.patch);

    if (tmpInVersion.major > currentVersion.major)
    {
        ret = true;
    }
    else if (tmpInVersion.major == currentVersion.major && tmpInVersion.minor > currentVersion.minor)
    {
        ret = true;
    }
    else if (tmpInVersion.major == currentVersion.major && tmpInVersion.minor == currentVersion.minor && tmpInVersion.patch > currentVersion.patch)
    {
        ret = true;
    }

    return ret;
}

bool isVersionNull()
{
    bool ret = false;
    DataSave_RetrieveOTAData();

    if (currentVersion.major == 0 && currentVersion.minor == 0 && currentVersion.patch == 0)
    {
        ret = true;
    }
    return ret;
}

std::string getVersionStr()
{
    std::string ret;
    DataSave_RetrieveOTAData();
    ret = std::to_string(currentVersion.major) + "." + std::to_string(currentVersion.minor) + "." + std::to_string(currentVersion.patch);
    return ret;
}