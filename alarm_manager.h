#ifndef ALARM_MANAGER_H
#define ALARM_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Maksimum alarm sayısı
#define MAX_ALARMS 12

// Gün maskeleri - bitwise kombinasyon için
#define SUNDAY      (1 << 0)  // tm_wday ile uyumlu
#define MONDAY      (1 << 1)  
#define TUESDAY     (1 << 2)
#define WEDNESDAY   (1 << 3)
#define THURSDAY    (1 << 4)
#define FRIDAY      (1 << 5)
#define SATURDAY    (1 << 6)

// Hazır kombinasyonlar
#define WEEKDAYS    (MONDAY | TUESDAY | WEDNESDAY | THURSDAY | FRIDAY)
#define WEEKENDS    (SATURDAY | SUNDAY)
#define EVERYDAY    (0x7F)

// Alarm tekrarlama modları
typedef enum {
    ALARM_ONCE = 0,        // Tek seferlik (belirli tarih)
    ALARM_DAILY,           // Her gün
    ALARM_WEEKDAYS,        // Hafta içi (Pzt-Cum)
    ALARM_WEEKEND,         // Hafta sonu (Cmt-Paz)
    ALARM_WEEKLY,          // Haftada bir (belirli günler)
    ALARM_MONTHLY,         // Ayda bir (belirli gün)
    ALARM_YEARLY,          // Yılda bir (belirli tarih)
    ALARM_CUSTOM_DAYS,     // Özel günler (bit maskesi ile)
    ALARM_DATE_RANGE       // Tarih aralığı
} alarm_repeat_mode_t;

// Alarm durumları
typedef enum {
    ALARM_STATE_DISABLED = 0,
    ALARM_STATE_ENABLED,
    ALARM_STATE_TRIGGERED,
    ALARM_STATE_SNOOZED,
    ALARM_STATE_EXPIRED    // Tarih geçmiş
} alarm_state_t;

// Alarm öncelik seviyeleri
typedef enum {
    ALARM_PRIORITY_LOW = 0,
    ALARM_PRIORITY_NORMAL,
    ALARM_PRIORITY_HIGH,
    ALARM_PRIORITY_CRITICAL
} alarm_priority_t;

// Tarih yapısı
typedef struct {
    uint8_t day;           // Gün (1-31, 0=herhangi)
    uint8_t month;         // Ay (1-12, 0=herhangi)
    uint16_t year;         // Yıl (2000+, 0=herhangi)
} alarm_date_t;

// Alarm yapısı
typedef struct {
    uint8_t id;                    // Alarm ID
    char name[16];                 // Alarm ismi
    
    // Zaman bilgileri
    uint8_t hour;                  // Saat (0-23)
    uint8_t minute;                // Dakika (0-59)
    uint8_t second;                // Saniye (0-59)
    
    // Tarih bilgileri
    alarm_date_t start_date;       // Başlangıç tarihi
    alarm_date_t end_date;         // Bitiş tarihi (ALARM_DATE_RANGE için)
    
    // Tekrar ve durum
    alarm_repeat_mode_t repeat_mode;
    uint8_t weekday_mask;          // Hafta günleri (bit 0=Pazar)
    alarm_state_t state;
    alarm_priority_t priority;
    
    // Snooze özellikleri
    uint8_t snooze_minutes;        // Erteleme süresi (dakika)
    uint8_t snooze_count;          // Mevcut erteleme sayısı
    uint8_t max_snooze_count;      // Maksimum erteleme sayısı
    time_t snooze_until;           // Erteleme bitiş zamanı
    
    // Son tetiklenme zamanı
    time_t last_triggered;
    
    // Callback fonksiyonu
    void (*callback)(uint8_t alarm_id);
	
	uint16_t missed_timeout_seconds; 
    
    // Kullanıcı verisi
    void *user_data;
    
} alarm_t;

// Alarm yöneticisi yapısı
typedef struct {
    alarm_t alarms[MAX_ALARMS];
    uint8_t alarm_count;
    bool initialized;
    
    // Global callback'ler
    void (*on_any_alarm)(uint8_t alarm_id);
    void (*on_alarm_missed)(uint8_t alarm_id);
    void (*on_alarm_expired)(uint8_t alarm_id);
    
} alarm_manager_t;

// Alarm manager fonksiyonları
void alarmInit(void);
void alarmProcess(void);

void alarmSetMissedTimeout(uint8_t alarm_id, uint16_t timeout_sec);
// Alarm ekleme/silme/güncelleme
uint8_t alarmAdd(const char *name, uint8_t hour, uint8_t minute, 
                 alarm_repeat_mode_t repeat_mode, 
                 void (*callback)(uint8_t));

uint8_t alarmAddWithDate(const char *name, uint8_t hour, uint8_t minute,
                         uint8_t day, uint8_t month, uint16_t year,
                         alarm_repeat_mode_t repeat_mode,
                         void (*callback)(uint8_t));

uint8_t alarmAddDateRange(const char *name, uint8_t hour, uint8_t minute,
                          alarm_date_t start_date, alarm_date_t end_date,
                          uint8_t weekday_mask,
                          void (*callback)(uint8_t));

bool alarmRemove(uint8_t alarm_id);
bool alarmUpdate(uint8_t alarm_id, alarm_t *alarm);

// Alarm kontrol fonksiyonları
bool alarmEnable(uint8_t alarm_id);
bool alarmDisable(uint8_t alarm_id);
bool alarmSnooze(uint8_t alarm_id);
bool alarmDismiss(uint8_t alarm_id);

// Alarm bilgi fonksiyonları
alarm_t* alarmGet(uint8_t alarm_id);
uint8_t alarmGetActive(alarm_t **alarm_list);
uint8_t alarmGetTriggered(alarm_t **alarm_list);
uint8_t alarmGetExpired(alarm_t **alarm_list);
time_t alarmGetNextAlarmTime(void);
bool alarmIsAnyActive(void);

// Yardımcı fonksiyonlar
bool alarmSetWeekdayMask(uint8_t alarm_id, uint8_t mask);
bool alarmSetSnooze(uint8_t alarm_id, uint8_t minutes, uint8_t max_count);
bool alarmSetPriority(uint8_t alarm_id, alarm_priority_t priority);
bool alarmSetStartDate(uint8_t alarm_id, uint8_t day, uint8_t month, uint16_t year);
bool alarmSetEndDate(uint8_t alarm_id, uint8_t day, uint8_t month, uint16_t year);
bool alarmSetDateRange(uint8_t alarm_id, alarm_date_t start, alarm_date_t end);

// Tarih yardımcı fonksiyonları
bool alarmIsDateValid(alarm_date_t date);
bool alarmIsDateInRange(alarm_date_t check_date, alarm_date_t start, alarm_date_t end);
int alarmCompareDates(alarm_date_t date1, alarm_date_t date2);
time_t alarmDateToTime(alarm_date_t date, uint8_t hour, uint8_t minute);

// Global callback ayarları
void alarmSetGlobalCallback(void (*on_any)(uint8_t), 
                           void (*on_missed)(uint8_t),
                           void (*on_expired)(uint8_t));

// Debug fonksiyonları
void alarmPrintStatus(void);
void alarmPrint(uint8_t alarm_id);
void alarmPrintUpcoming(uint8_t days_ahead);

#endif // ALARM_MANAGER_H