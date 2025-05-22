#ifndef NETWORK_H
#define NETWORK_H

bool connectWifi(int timeoutms);
bool isWifiConnected();
void initWifi();
void disconnectWifi();

#endif