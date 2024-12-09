#include "credentials.h"
#include "network.h"
#include "WiFi.h"

void connectWifi(int timeoutms)
{
    WiFi.mode(WIFI_STA);
    Serial.println("Scanning");
    int n = WiFi.scanNetworks();
    if (n != 0)
    {
        int index = -1;
        for (int i = 0; i < NB_NETWORK; i++)
        {
            for (int j = 0; j < n; ++j)
            {
                if (strcmp(WiFi.SSID(j).c_str(), ssidArr[i]) == 0)
                {
                    Serial.println("Network found");
                    index = i;
                    break;
                }
            }
            if (index != -1)
            {
                Serial.println("Connecting");
                WiFi.begin(ssidArr[index], pswdArr[index]);
                unsigned long start = millis();
                while (millis() - start < timeoutms && !isWifiConnected())
                    ;
                if (isWifiConnected())
                {
                    Serial.println("Connected");
                }
                else
                {
                    Serial.println("Not connected");
                }

                break;
            }
        }
    }
    WiFi.scanDelete();
}

bool isWifiConnected()
{
    return WiFi.isConnected();
}