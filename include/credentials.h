
#ifndef CREDENTIAL_H
#define CREDENTIAL_H
#include <Arduino.h>

#define NB_NETWORK 1

extern const char *ssidArr[NB_NETWORK];
extern const char *pswdArr[NB_NETWORK];
extern char wifiSsid0[32];
extern char wifiPswd0[32];

extern char apiRefreshToken[41];
extern uint64_t clientId;
extern char clientSecret[41];

// extern const char getTokenUrl[];

#endif