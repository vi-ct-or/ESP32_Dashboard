#ifndef POLYLINE_H
#define POLYLINE_H

#include <list>
typedef struct sCoordinates
{
    int lat;
    int lng;
} TsCoordinates;
extern std::list<TsCoordinates> coordList;
void decode(const char *arr, int length);
void getMinMaxLatLngFromDecode(const char *arr, int length, int *minLat, int *maxLat, int *minLng, int *maxLng);
TsCoordinates continuousDecode(const char *arr, int length, int *index, int *tmpLat, int *tmpLng);

#endif