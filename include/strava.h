#ifndef STRAVA_H
#define STRAVA_H

#define MAX_NAME_LENGTH 40

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
    bool isFilled;
    uint16_t dist;
    uint16_t time;
    uint16_t deniv;
    time_t timestamp;
    TeActivityType type;
    char name[MAX_NAME_LENGTH];
    std::string polyline;
    uint32_t kudos;
} TsActivity;

typedef enum eStravaMessage
{
    STRAVA_MESSAGE_NONE,
    STRAVA_MESSAGE_NEW_MONTH,
    STRAVA_MESSAGE_POPULATE,
} TeStravaMessage;
extern QueueHandle_t xQueueStrava;
extern SemaphoreHandle_t xSemaphore;

#define DAYS_BY_YEAR 366
#define NB_LAST_ACTIVITIES 20

extern bool newActivityUploaded;
extern const uint16_t monthOffset[];
extern uint64_t lastActivitiesId[NB_LAST_ACTIVITIES];

void initDB();
uint32_t getTotal(TeActivityType activityType, TeDataType dataType, uint16_t startDay, uint16_t endDay);
void populateDB(void);
void newYearBegin();
void newMonthBegin();
void StravaTaskFunction(void *parameter);

TsActivity *getStravaLastActivity();

#endif