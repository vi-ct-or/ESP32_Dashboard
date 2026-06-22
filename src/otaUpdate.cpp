#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "displayEpaper.h"

#include "otaUpdate.h"
#include "strava.h"
#include "RTCTime.h"

#include <OTA-Hub.hpp>
#include <OTA-Hub/FOTA-providers/github.hpp>

#define OTAGH_OWNER_NAME "vi-ct-or"
#define OTAGH_REPO_NAME "ESP32_Dashboard"
#define OTAGH_BEARER "YOUR PRIVATE REPO TOKEN" // Follow the docs if using a private repo. Remove if repo is public.

WiFiClientSecure wifi_client;
OTAHub::FOTA::GithubProvider provider(
    OTAGH_OWNER_NAME,
    OTAGH_REPO_NAME);

// Define the name for the downloaded firmware file
#define FILE_NAME "firmware.bin"

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

    if (details.condition == OTAHub::FOTA::NEW_DIFFERENT)
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
            resetDB();
        }
        delay(5000);
        resetClock();
        ESP.restart();
    }
    else
    {
        Serial.println("No new update available. Continuing...");
    }
}