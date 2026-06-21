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

        if (*index >= length)
        {
            Serial.println("trans: premature end of buffer");
            break;
        }

        byte = (int)((uint8_t)arr[*index]) - 63;
        *index += 1;
        result |= (byte & 0x1f) << shift;
        shift += 5;
        comp = result & 1;
        if (shift > 30)
        {
            Serial.println("trans: shift cap reached");
            break;
        }

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
    coord.lat = 0;
    coord.lng = 0;
    while (index < length)
    {
        int before = index;
        lat_change = _trans(arr, &index, length);
        lng_change = _trans(arr, &index, length);

        // if nothing advanced, stop to avoid infinite loop
        if (index == before)
        {
            Serial.println("decode: index did not advance, aborting decode loop");
            break;
        }

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

void getMinMaxLatLngFromDecode(const char *arr, int length, int *minLat, int *maxLat, int *minLng, int *maxLng)
{
    int index = 0, lat = 0, lng = 0, factor = 100000, lat_change, lng_change;
    *minLat = 1800000;
    *maxLat = -1800000;
    *minLng = 1800000;
    *maxLng = -1800000;
    TsCoordinates coord;
    coord.lat = 0;
    coord.lng = 0;
    while (index < length)
    {

        lat_change = _trans(arr, &index, length);
        lng_change = _trans(arr, &index, length);

        lat += lat_change;
        lng += lng_change;
        coord.lat = lat / 10; // (float)factor;
        coord.lng = lng / 10; // (float)factor;

        if (coord.lat > *maxLat)
        {
            *maxLat = coord.lat;
        }
        if (coord.lng > *maxLng)
        {
            *maxLng = coord.lng;
        }
        if (coord.lat < *minLat)
        {
            *minLat = coord.lat;
        }
        if (coord.lng < *minLng)
        {
            *minLng = coord.lng;
        }
    }
}

TsCoordinates continuousDecode(const char *arr, int length, int *index, int *tmpLat, int *tmpLng)
{
    int factor = 100000, lat_change, lng_change;
    TsCoordinates coord;
    coord.lat = 0;
    coord.lng = 0;
    if (*index < length)
    {

        lat_change = _trans(arr, index, length);
        lng_change = _trans(arr, index, length);

        *tmpLat += lat_change;
        *tmpLng += lng_change;
        coord.lat = *tmpLat / 10; // (float)factor;
        coord.lng = *tmpLng / 10; // (float)factor;
    }
    return coord;
}