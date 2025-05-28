
#ifndef DATA_SAVE_H
#define DATA_SAVE_H

uint8_t DataSave_Init();
void DataSave_RetrieveWifiCredentials();
void DataSave_RetreiveLastActivity();
void DataSave_RetrieveOTAData();
void DataSave_SaveLastActivity();
void DataSave_SaveOTAData();
uint8_t DataSave_SaveWifiCredentials();
uint8_t DataSave_ResetOTA();
void DataSave_resetLastActivities();
void DataSave_EraseEEPROM();

#endif