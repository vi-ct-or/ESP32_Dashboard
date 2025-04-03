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
    bool isFilled = false;
    uint16_t dist = 0;
    uint16_t time = 0;
    uint16_t deniv = 0;
    time_t timestamp = 0;
    TeActivityType type;
    std::string name = "";
    std::string polyline = "";
    uint32_t kudos = 0;
} TsActivity;

#define DAYS_BY_YEAR 366
extern bool newActivity;
extern const uint16_t monthOffset[];

void initDB();
uint32_t getTotal(TeActivityType activityType, TeDataType dataType, uint16_t startDay, uint16_t endDay);
void populateDB(void);
void newYearBegin();
void newMonthBegin(uint8_t prevMonth, struct tm tm);

TsActivity *getStravaLastActivity();

#endif