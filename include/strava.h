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

typedef struct sActivity
{
    uint16_t dist;
    uint16_t time;
    uint16_t deniv;
    time_t timestamp;
    TeActivityType type;
    std::string name;
    std::string polyline;
} TsActivity;

#define DAYS_BY_YEAR 366
extern bool newActivity;
extern const uint16_t monthOffset[];

void initDB();
uint32_t getTotal(TeActivityType activityType, TeDataType dataType, uint16_t startDay, uint16_t endDay);
void populateDB(void);
void newYearBegin();
void newMonthBegin(struct tm tm);

TsActivity *getStravaLastActivity();

#endif