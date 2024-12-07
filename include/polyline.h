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

#endif