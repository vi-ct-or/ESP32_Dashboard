
#ifndef DISPLAY_H
#define DISPLAY_H

extern bool refresh;

void initDisplay(void);
void displayTemplate();
void displayTime(struct tm *now);
void displayDate(struct tm *now);
void displayStravaAllYear();
void displayStravaMonths(struct tm *now);
void displayStravaWeeks(struct tm *now);
void displayStravaPolyline();
void displayLastActivity();
void displayUpdating(uint8_t state);
void displayTimeSync(bool gpsSync);
#endif
