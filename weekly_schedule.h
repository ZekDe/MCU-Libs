#ifndef WEEKLY_SCHEDULE_H
#define WEEKLY_SCHEDULE_H

#include <stdint.h>
#include <time.h>

#define HOURS_IN_WEEK 168  // 7 gün x 24 saat = 168

// Program modları
typedef enum {
	SCHEDULE_MODE_NONE = 0,
    SCHEDULE_MODE_ONOFF = 1,    // Mode 1: 0/1 (kapalı/açık)
    SCHEDULE_MODE_LEVEL,        // Mode 2: 0-3 seviye kontrolü
    SCHEDULE_MODE_TEMP,         // Mode 3: Sıcaklık değeri
	SCHEDULE_MODE_END,
} schedule_mode_t;

// Haftalık program yapısı
typedef struct 
{
	schedule_mode_t current_mode;            // Aktif mod
    uint16_t schedule_hours[HOURS_IN_WEEK];  // 168 saatlik dizi
    uint32_t is_active;                       // Program aktif mi?
} weekly_schedule_t;

// Global haftalık program
extern weekly_schedule_t g_weekly_schedule;

// Fonksiyon prototipleri
void weeklyScheduleInit(void);
void weeklyScheduleDeInit(void);
void weeklyScheduleSetMode(schedule_mode_t mode);
uint8_t weeklyScheduleGetCurrentHourIndex(void);
uint16_t weeklyScheduleGetCurrentValue(void);
void weeklyScheduleSetHour(uint8_t hour_index, uint16_t value);
uint16_t weeklyScheduleGetHour(uint8_t hour_index);
void weeklyScheduleProcessCurrentHour(void);

// Utility fonksiyonları
uint8_t weeklyScheduleDayHourToIndex(uint8_t day, uint8_t hour);
void weeklyScheduleIndexToDayHour(uint8_t index, uint8_t *day, uint8_t *hour);
void weeklyScheduleClearAll(void);
void weeklyScheduleSetDay(uint8_t day, uint16_t value);
uint8_t isWeeklyScheduleEmpty(void);

// Debug fonksiyonları
void weeklySchedulePrintWeek(void);
void weeklySchedulePrintDay(uint8_t day);

#endif // WEEKLY_SCHEDULE_H