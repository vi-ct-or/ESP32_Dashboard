#ifndef STRAVA_H
#define STRAVA_H

#define MAX_NAME_LENGTH 40

typedef enum eActivityType
{
    ACTIVITY_TYPE_UNKNOWN,
    ACTIVITY_TYPE_RUN,
    ACTIVITY_TYPE_BIKE,
    ACTIVITY_TYPE_SWIM,
    ACTIVITY_TYPE_WORKOUT,
    ACTIVITY_TYPE_RACKET,
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
    bool isVisible;
} TsActivity;

typedef enum eStravaMessage
{
    STRAVA_MESSAGE_NONE,
    STRAVA_MESSAGE_NEW_MONTH,
    STRAVA_MESSAGE_POPULATE,
    STRAVA_MESSAGE_GET_TEMPERATURE,
    STRAVA_MESSAGE_RESET_ALL,
} TeStravaMessage;
extern QueueHandle_t xQueueStrava;
extern SemaphoreHandle_t xSemaphore;
extern SemaphoreHandle_t mutex;

#define DAYS_BY_YEAR 366
#define NB_LAST_ACTIVITIES 20

extern bool newActivityUploaded;
extern const uint16_t monthOffset[];
extern uint64_t lastActivitiesId[NB_LAST_ACTIVITIES];
extern time_t lastActivityTimestamp;
extern uint16_t prevKudos;
extern float airTemperature;
extern float waterTemperature;
extern uint32_t tempDataOldness;
extern uint32_t sunriseTimestamp;
extern uint32_t sunsetTimestamp;

bool initDB();
uint32_t getTotal(TeActivityType activityType, TeDataType dataType, uint16_t startDay, uint16_t endDay);
void populateDB(void);
void newYearBegin();
void newMonthBegin();
void StravaTaskFunction(void *parameter);
void resetDB();
bool isLastActivityFromToday();
uint32_t getCurrentStreakDays();
void test_NVM();

TsActivity *getStravaLastActivity();

#endif