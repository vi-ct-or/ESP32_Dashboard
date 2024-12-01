#include <WiFi.h>
#include <SPIFFS.h>
#include "Update.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "display.h"

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

void updateFW()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        Serial.print("error init nvs flash : ");
        Serial.println(err);
        // Retry nvs_flash_init
        err = nvs_flash_init();
    }

    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        Serial.print("Error opening NVS handle!");
        Serial.println(esp_err_to_name(err));
    }
    uint8_t currentVersion = 0; // value will default to 0, if not set yet in NVS
    err = nvs_get_u8(my_handle, "currentVersion", &currentVersion);
    switch (err)
    {
    case ESP_OK:
        break;
    case ESP_ERR_NVS_NOT_FOUND:
        Serial.print("The value is not initialized yet!\n");
        break;
    default:
        Serial.print("Error nvs get u8!");
        Serial.println(esp_err_to_name(err));
    }

    uint8_t newVersion = getVersion();
    Serial.print("Current Version :");
    Serial.println(currentVersion);
    Serial.print("New Version :");
    Serial.println(newVersion);
    if (newVersion > currentVersion)
    {
        displayText("Updating firmware...");
        if (getFileFromServer())
        {
            if (performOTAUpdateFromSPIFFS())
            {
                err = nvs_set_u8(my_handle, "currentVersion", newVersion);
                Serial.println((err != ESP_OK) ? "nvs_set_u8 Failed!" : "nvs_set_u8 Done");
                Serial.print("Committing updates in NVS ... ");
                err = nvs_commit(my_handle);
                Serial.println((err != ESP_OK) ? "nvs_commit Failed!" : "nvs_commit Done");

                // Close
                nvs_close(my_handle);
            }
        }
        displayText("Reboot in 5s...");
        Serial.println("Reset in 5 seconds....");
        delay(5000);
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
        version = (uint8_t)http.getString().toInt();
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
        displayText("Downloading new FW");
        bool endOfHeaders = false;
        String headers = "";
        String http_response_code = "error";
        const size_t bufferSize = 1024; // Buffer size for reading data
        uint8_t buffer[bufferSize];

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
        while (client.connected())
        {
            if (client.available())
            {
                size_t bytesRead = client.readBytes(buffer, bufferSize);
                file.write(buffer, bytesRead); // Write data to file
            }
        }
        file.close();  // Close the file
        client.stop(); // Close the client connection
        Serial.println("File saved successfully");
        displayText("Download successful");
        l_ret = true;
    }
    else
    {
        Serial.println("Failed to connect to server");
        displayText("Download failed");
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
    displayText("Starting update");
    size_t fileSize = file.size(); // Get the file size
    Serial.println(fileSize);

    // Begin OTA update process with specified size and flash destination
    if (!Update.begin(fileSize, U_FLASH))
    {
        Serial.println("Cannot do the update");
        return false;
    }

    // Write firmware data from file to OTA update
    Update.writeStream(file);

    // Complete the OTA update process
    if (Update.end())
    {
        Serial.println("Successful update");
        displayText("Update successful");
        l_ret = true;
    }
    else
    {
        Serial.println("Error Occurred:" + String(Update.getError()));
        displayText("Update failed");
        return false;
    }

    file.close(); // Close the file

    return l_ret;
}