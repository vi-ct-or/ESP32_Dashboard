
#ifndef CREDENTIAL_H
#define CREDENTIAL_H
#include <Arduino.h>

#define NB_NETWORK 3

extern char *ssidArr[NB_NETWORK];
extern char *pswdArr[NB_NETWORK];
extern uint8_t nbWifi;
extern char wifiSsid0[32];
extern char wifiPswd0[32];
extern char wifiSsid1[32];
extern char wifiPswd1[32];
extern char wifiSsid2[32];
extern char wifiPswd2[32];

extern char apiRefreshToken[41];
extern uint64_t clientId;
extern char clientSecret[41];

// extern const char getTokenUrl[];

#endif