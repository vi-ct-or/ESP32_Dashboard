#ifndef GPSTIME_H
#define GPSTIME_H

bool setRtcTime();
bool adjustLocalTimeFromRtc();
bool rtcAvailable();
void resetClock();

#endif