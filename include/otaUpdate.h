#ifndef OTA_H
#define OTA_H

typedef struct sVersion
{
    uint8_t major;
    uint8_t minor;
    uint8_t patch;

} TsVersion;

extern TsVersion currentVersion;
void updateFW();

#endif