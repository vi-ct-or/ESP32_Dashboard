
#ifndef DISPLAY_H
#define DISPLAY_H

extern bool refresh;

void initDisplay(void);
void displayTemplate();
void displayTime(struct tm *now);
void displayDate(struct tm *now);
void displayStravaAllYear();
void displayStravaYTD();
void displayStravaMonths(struct tm *now);
void displayStravaCurrentWeek();
void displayStravaToday();
void displayText(const char msg[]);
void displayStravaPolyline();
void displayStravaThisMonth(struct tm *now);
void displayLastActivity();
void displayUpdating(uint8_t state);
#endif
