#ifndef GPSTIME_H
#define GPSTIME_H

void initGpsTime();
bool setGPSTime();
bool setRtcTime();
bool adjustLocalTimeFromRtc();
bool rtcAvailable();

#endif