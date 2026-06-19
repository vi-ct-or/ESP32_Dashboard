#ifndef NETWORK_H
#define NETWORK_H

typedef enum eNetworkStrength
{
    NETWORK_STRENGTH_NONE,
    NETWORK_STRENGTH_BAD,
    NETWORK_STRENGTH_MEDIUM,
    NETWORK_STRENGTH_GOOD,
} TeNetworkStrength;

bool connectWifi(int timeoutms);
bool isWifiConnected();
void initWifi();
void disconnectWifi();
void testWifi();
TeNetworkStrength getNetworkStrength();


#endif