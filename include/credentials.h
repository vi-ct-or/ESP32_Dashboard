
#ifndef CREDENTIAL_H
#define CREDENTIAL_H
#include <Arduino.h>

#define NB_NETWORK 3

extern const char *ssidArr[NB_NETWORK];
extern const char *pswdArr[NB_NETWORK];

extern const char apiRefreshToken[];
extern const char clientId[];
extern const char clientSecret[];

extern const char getTokenUrl[];

#endif