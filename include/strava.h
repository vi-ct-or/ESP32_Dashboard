#ifndef STRAVA_H
#define STRAVA_H
typedef enum eActivityType
{
    ACTIVITY_TYPE_UNKNOWN,
    ACTIVITY_TYPE_RUN,
    ACTIVITY_TYPE_BIKE,
} TeActivityType;

typedef enum eDataType
{
    DATA_TYPE_DISTANCE,
    DATA_TYPE_DENIV,
    DATA_TYPE_TIME,
} TeDataType;

#define DAYS_BY_YEAR 366

extern std::string lastPolyline;
float getTotal(TeActivityType activityType, TeDataType dataType, bool year, uint16_t startDay, uint16_t endDay);
void populateDB(void);
void newYearBegin();
void getPolyline(void *out);

#endif