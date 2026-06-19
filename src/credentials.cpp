#include "credentials.h"

uint8_t nbWifi = 3;
char wifiSsid0[32];
char wifiPswd0[32];
char wifiSsid1[32];
char wifiPswd1[32];
char wifiSsid2[32];
char wifiPswd2[32];
char apiRefreshToken[41];
uint64_t clientId;
char clientSecret[41];

char *ssidArr[NB_NETWORK] = {wifiSsid0, wifiSsid2, wifiSsid1};
char *pswdArr[NB_NETWORK] = {wifiPswd0, wifiPswd2, wifiPswd1};
