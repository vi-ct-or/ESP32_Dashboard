
#ifndef DISPLAY_H
#define DISPLAY_H

typedef enum eDisplayMessage
{
    DISPLAY_MESSAGE_NONE,
    DISPLAY_MESSAGE_DATE,
    DISPLAY_MESSAGE_TIME,
    DISPLAY_MESSAGE_MONTHS,
    DISPLAY_MESSAGE_WEEKS,
    DISPLAY_MESSAGE_LAST_ACTIVITY,
    DISPLAY_MESSAGE_POLYLINE,
    DISPLAY_MESSAGE_TOTAL_YEAR,
    DISPLAY_MESSAGE_REFRESH,
    DISPLAY_MESSAGE_TEMPLATE,
    DISPLAY_MESSAGE_NEW_ACTIVITY,
} TeDisplayMessage;
extern QueueHandle_t xQueueDisplay;
extern bool hasBeenRefreshed;

void initDisplay(void);
void displayTemplate();
void displayTime(struct tm *now);
void displayDate(struct tm *now);
void displayStravaAllYear(struct tm *now);
void displayStravaMonths(struct tm *now);
void displayStravaWeeks(struct tm *now);
void displayStravaPolyline();
void displayLastActivity();
void displayUpdating(uint8_t state);
void displayTimeSync(bool gpsSync);
void displayTaskFunction(void *parameter);
#endif
