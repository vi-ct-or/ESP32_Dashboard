#include "polyline.h"
#include <Arduino.h>

std::list<TsCoordinates> coordList = {};

int _trans(const char *arr, int *index, int length)
{
    int result = 0, shift = 0, byte = INT32_MAX, comp;

    // Ensure we don't read past buffer
    if (*index >= length)
    {
        Serial.println("in *index >= length");
        return 0;
    }

    do
    {
        byte = (int)arr[*index] - 63;
        *index += 1;
        result |= (byte & 0x1f) << shift;
        shift += 5;
        comp = result & 1;
    } while (byte >= 0x20);

    int ret;
    if (comp)
    {
        ret = ~(result >> 1);
    }
    else
    {
        ret = result >> 1;
    }
    return ret;
}

void decode(const char *arr, int length)
{
    coordList.clear();
    int index = 0, lat = 0, lng = 0, factor = 100000, lat_change, lng_change;
    TsCoordinates coord;
    coord.lat = 0.0;
    coord.lng = 0.0;
    while (index < length)
    {
        lat_change = _trans(arr, &index, length);
        lng_change = _trans(arr, &index, length);

        // if index did not advance and we reached the end, break to avoid infinite loop
        if (index >= length && lat_change == 0 && lng_change == 0)
        {
            Serial.println("in if (index >= length && lat_change == 0 && lng_change == ");
            break;
        }

        lat += lat_change;
        lng += lng_change;
        coord.lat = lat / 10; // (float)factor;
        coord.lng = lng / 10; // (float)factor;
        coordList.push_back(coord);
    }
}