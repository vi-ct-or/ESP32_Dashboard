
typedef enum eActivityType
{
    ACTIVITY_TYPE_UNKNOWN,
    ACTIVITY_TYPE_RUN,
    ACTIVITY_TYPE_BIKE,
} TeActivityType;

float getTotal(TeActivityType activityType, bool dataType, bool year, bool allRound);
void populateDB(void);