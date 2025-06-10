#include <WiFi.h>
#include <SPIFFS.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <esp_task_wdt.h>
#include "Update.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "displayEpaper.h"
#include "dataSave.h"

#include "otaUpdate.h"

// Define server details and file path
#define HOST "raw.githubusercontent.com"
#define PATH_FW "/vi-ct-or/ESP32_Dashboard/refs/heads/develop/.pio/build/esp32dev/firmware.bin"
#define PATH_Version "https://raw.githubusercontent.com/vi-ct-or/ESP32_Dashboard/refs/heads/develop/version.txt"
#define PORT 443

// Define the name for the downloaded firmware file
#define FILE_NAME "firmware.bin"

bool getFileFromServer();
bool performOTAUpdateFromSPIFFS();
uint8_t getVersion();
uint8_t currentVersion;

void updateFW()
{
    DataSave_RetrieveOTAData();

    uint8_t newVersion = getVersion();
    Serial.print("Current Version :");
    Serial.println(currentVersion);
    Serial.print("New Version :");
    Serial.println(newVersion);
    if (newVersion > currentVersion)
    {

        displayUpdating(0);
        if (getFileFromServer())
        {
            if (performOTAUpdateFromSPIFFS())
            {
                currentVersion = newVersion;
                DataSave_SaveOTAData();
            }
        }
        delay(1000);
        ESP.restart(); // Restart ESP32 to apply the update
    }
}

uint8_t getVersion()
{
    uint8_t version = 0;
    HTTPClient http;
    http.begin(PATH_Version);
    int httpResponse = http.GET();
    if (httpResponse == 200)
    {
        String resp = http.getString();
        version = (uint8_t)resp.toInt();
    }
    else
    {
        Serial.print("HTTP GET failed with error: ");
        Serial.println(httpResponse);
        version = 0; // Set version to 0 if the request fails
    }

    http.end();
    return version;
}

bool getFileFromServer()
{
    bool l_ret = false;
    WiFiClientSecure client;
    client.setInsecure(); // Set client to allow insecure connections

    if (client.connect(HOST, PORT))
    { // Connect to the server
        Serial.println("Connected to server");
        client.print("GET " + String(PATH_FW) + " HTTP/1.1\r\n"); // Send HTTP GET request
        client.print("Host: " + String(HOST) + "\r\n");           // Specify the host
        client.println("Connection: close\r\n");                  // Close connection after response
        client.println();                                         // Send an empty line to indicate end of request headers

        SPIFFS.begin(true);
        File file = SPIFFS.open("/" + String(FILE_NAME), FILE_WRITE); // Open file in SPIFFS for writing
        if (!file)
        {
            Serial.println("Failed to open file for writing");
            return false;
        }
        bool endOfHeaders = false;
        String headers = "";
        String http_response_code = "error";
        const size_t bufferSize = 1024; // Buffer size for reading data
        uint8_t buffer[bufferSize];
        displayUpdating(1);

        // Loop to read HTTP response headers
        while (client.connected() && !endOfHeaders)
        {
            if (client.available())
            {
                char c = client.read();
                headers += c;
                if (headers.startsWith("HTTP/1.1"))
                {
                    http_response_code = headers.substring(9, 12);
                }
                if (headers.endsWith("\r\n\r\n"))
                { // Check for end of headers
                    endOfHeaders = true;
                }
            }
        }

        Serial.println("HTTP response code: " + http_response_code); // Print received headers

        // Loop to read and write raw data to file
        int cnt = 0;
        while (client.connected())
        {
            if (client.available())
            {
                size_t bytesRead = client.readBytes(buffer, bufferSize);
                file.write(buffer, bytesRead); // Write data to file
                digitalWrite(2, !digitalRead(2));
                // esp_task_wdt_reset();
            }
        }
        file.close(); // Close the file
        Serial.print("size file downloaded = ");
        Serial.println(file.size());
        client.stop(); // Close the client connection
        Serial.println("File saved successfully");
        displayUpdating(2);

        l_ret = true;
    }
    else
    {
        Serial.println("Failed to connect to server");
        l_ret = false;
    }
    return l_ret;
}

bool performOTAUpdateFromSPIFFS()
{
    bool l_ret = false;
    // Open the firmware file in SPIFFS for reading
    File file = SPIFFS.open("/" + String(FILE_NAME), FILE_READ);
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return false;
    }

    Serial.println("Starting update..");
    displayUpdating(3);

    size_t fileSize = file.size(); // Get the file size
    Serial.print("size file reopen = ");
    Serial.println(fileSize);

    // Begin OTA update process with specified size and flash destination

    // esp_task_wdt_reset();
    if (!Update.begin(fileSize, U_FLASH))
    {
        Serial.println("Cannot do the update");
        displayUpdating(4);
        return false;
    }

    // Write firmware data from file to OTA update
    Update.writeStream(file);

    // Complete the OTA update process
    if (Update.end())
    {
        Serial.println("Successful update");
        displayUpdating(5);
        l_ret = true;
    }
    else
    {
        displayUpdating(4);
        Serial.println("Error Occurred:" + String(Update.getError()));
        return false;
    }

    file.close(); // Close the file

    return l_ret;
}