#ifndef DISPLAY_H
#define DISPLAY_H
void initDisplay(void);
void displayTime(struct tm *now);
void displayStravaAllYear();
void displayStravaYTD();
void displayStravaCurrentWeek();
void displayStravaToday();
void displayText(const char msg[]);
void displayClearContent(void);
void displayStravaPolyline();
#endif
