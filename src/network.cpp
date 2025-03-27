#include "credentials.h"
#include "network.h"
#include "WiFi.h"
#include <Preferences.h>

#define NB_NETWORK 1

Preferences networkPreferences;
RTC_DATA_ATTR char ssid1[32] = {0};
RTC_DATA_ATTR char password1[32] = {0};
char *ssidArr[1] = {ssid1};
char *pswdArr[1] = {password1};

void initWifi()
{
    WiFi.mode(WIFI_STA);
    if (ssid1[0] == 0 && password1[0] == 0)
    {
        networkPreferences.begin("configNetwork");
        networkPreferences.getString("ssid", ssid1, 32);
        networkPreferences.getString("pswd", password1, 32);
        networkPreferences.end();
    }
}

bool connectWifi(int timeoutms)
{
    bool connected = false;
    if (isWifiConnected())
    {
        return true;
    }
    Serial.println("Scanning");
    int n = WiFi.scanNetworks();
    if (n != 0)
    {
        int index = -1;
        for (int i = 0; i < NB_NETWORK; i++)
        {
            for (int j = 0; j < n; ++j)
            {
                if (WiFi.SSID(j) == ssidArr[i])
                {
                    Serial.println("Network found");
                    index = i;
                    break;
                }
            }
            if (index != -1)
            {
                Serial.println(ssidArr[index]);
                Serial.println("Connecting");
                WiFi.begin(ssidArr[index], pswdArr[index]);
                unsigned long start = millis();
                while (millis() - start < timeoutms && !isWifiConnected())
                    ;
                if (isWifiConnected())
                {
                    Serial.println("Connected");
                    connected = true;
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
    return connected;
}

bool isWifiConnected()
{
    return WiFi.isConnected();
}

void disconnectWifi()
{
    WiFi.disconnect(true);
}