#include "alarm_manager.h"
#include <string.h>
#include <stdio.h>


// Global alarm yöneticisi
static alarm_manager_t alarm_mgr = {0};

// Dahili yardımcı fonksiyonlar
static bool isAlarmTimeReached(alarm_t *alarm, struct tm *current_tm, time_t current_time);
static bool checkWeekdayMatch(alarm_t *alarm, int weekday);
static bool checkDateMatch(alarm_t *alarm, struct tm *current_tm);
static bool isAlarmExpired(alarm_t *alarm, struct tm *current_tm);
static uint8_t getNextFreeId(void);
static alarm_t* getAlarmById(uint8_t alarm_id);

/**
 * @brief Alarm yöneticisini başlatır
 */
void alarmInit(void)
{
    memset(&alarm_mgr, 0, sizeof(alarm_mgr));
    alarm_mgr.initialized = true;
    alarm_mgr.alarm_count = 0;
    
    // Tüm alarmları devre dışı olarak başlat
    for (int i = 0; i < MAX_ALARMS; i++) {
        alarm_mgr.alarms[i].state = ALARM_STATE_DISABLED;
        alarm_mgr.alarms[i].id = 0xFF; // Geçersiz ID
		alarm_mgr.alarms[i].missed_timeout_seconds = 300; // alarm kacırılma default 300 sn
    }
}

/**
 * @brief Ana alarm işleme fonksiyonu (periyodik olarak çağrılmalı)
 */
void alarmProcess(void)
{
    if (!alarm_mgr.initialized) {
        return;
    }
    
    // Mevcut zamanı al
    time_t current_time = time(NULL);
    struct tm *current_tm = localtime(&current_time);
    
    // Tüm alarmları kontrol et
    for (int i = 0; i < MAX_ALARMS; i++) {
        alarm_t *alarm = &alarm_mgr.alarms[i];
        
        // Sadece aktif alarmları kontrol et
        if (alarm->state == ALARM_STATE_DISABLED)
            continue;

        // Tarihi geçmiş alarmları kontrol et
        if (isAlarmExpired(alarm, current_tm)) {
            if (alarm->state != ALARM_STATE_EXPIRED) {
                alarm->state = ALARM_STATE_EXPIRED;
                if (alarm_mgr.on_alarm_expired) {
                    alarm_mgr.on_alarm_expired(alarm->id);
                }
            }
            continue;
        }

        // Erteleme durumunu kontrol et
        if (alarm->state == ALARM_STATE_SNOOZED) {
            if (current_time >= alarm->snooze_until) {
                alarm->state = ALARM_STATE_TRIGGERED;
                alarm->last_triggered = current_time;
                
                // Callback'i çağır
                if (alarm->callback)
                    alarm->callback(alarm->id);

                if (alarm_mgr.on_any_alarm) 
                    alarm_mgr.on_any_alarm(alarm->id);
            }
            continue;
        }
        
        // Normal alarm kontrolü
        if (isAlarmTimeReached(alarm, current_tm, current_time)) {
            // Son tetiklenmeden en az 1 dakika geçmiş olmalı
            if (current_time - alarm->last_triggered > 60) 
			{
                alarm->state = ALARM_STATE_TRIGGERED;
                alarm->last_triggered = current_time;
                alarm->snooze_count = 0;
                
                // Callback'i çağır
                if (alarm->callback) 
                    alarm->callback(alarm->id);
 
                if (alarm_mgr.on_any_alarm)
                    alarm_mgr.on_any_alarm(alarm->id);
            }
        }
        
        // Kaçırılan alarm kontrolü
        if ((alarm->state == ALARM_STATE_TRIGGERED) && 
            (current_time - alarm->last_triggered > alarm->missed_timeout_seconds)) 
		{
            if (alarm_mgr.on_alarm_missed) 
			{
                alarm_mgr.on_alarm_missed(alarm->id);
            }
            
            // Tekrar moduna göre alarmı yeniden etkinleştir veya devre dışı bırak
            if (alarm->repeat_mode == ALARM_ONCE) 
			{
                alarm->state = ALARM_STATE_DISABLED;
            } else 
			{
                alarm->state = ALARM_STATE_ENABLED;
            }
        }
    }
}

/**
 * @brief alarm tetiklendiğinde timeout_sec süre içinde dismiss edilmezse, missed callback çağrılır(eğer fonksiyon tanımlanmışsa).
 */
void alarmSetMissedTimeout(uint8_t alarm_id, uint16_t timeout_sec)
{
	if(alarm_id < 1) alarm_id = 1;
	else if(alarm_id > MAX_ALARMS) alarm_id = MAX_ALARMS;
	
	alarm_mgr.alarms[alarm_id-1].missed_timeout_seconds = timeout_sec;
}

/**
 * @brief Basit alarm ekler (tarihsiz)
 */
uint8_t alarmAdd(const char *name, uint8_t hour, uint8_t minute, 
                 alarm_repeat_mode_t repeat_mode, 
                 void (*callback)(uint8_t))
{
    return alarmAddWithDate(name, hour, minute, 0, 0, 0, repeat_mode, callback);
}

/**
 * @brief Tarihli alarm ekler
 */
uint8_t alarmAddWithDate(const char *name, uint8_t hour, uint8_t minute,
                         uint8_t day, uint8_t month, uint16_t year,
                         alarm_repeat_mode_t repeat_mode,
                         void (*callback)(uint8_t))
{
    if (!alarm_mgr.initialized || alarm_mgr.alarm_count >= MAX_ALARMS) {
        return 0xFF; // Hata
    }
    
    // Boş slot bul
    for (int i = 0; i < MAX_ALARMS; i++) {
        if (alarm_mgr.alarms[i].id == 0xFF) {
            alarm_t *alarm = &alarm_mgr.alarms[i];
            
            // Alarm bilgilerini ayarla
            alarm->id = getNextFreeId();
            strncpy(alarm->name, name, sizeof(alarm->name) - 1);
            alarm->hour = hour % 24;
            alarm->minute = minute % 60;
            alarm->second = 0;
            alarm->repeat_mode = repeat_mode;
            alarm->state = ALARM_STATE_ENABLED;
            alarm->priority = ALARM_PRIORITY_NORMAL;
            alarm->callback = callback;
            
            // Tarih bilgilerini ayarla
            alarm->start_date.day = day;
            alarm->start_date.month = month;
            alarm->start_date.year = year;
            
            // Varsayılan ayarlar
            alarm->snooze_minutes = 5;
            alarm->max_snooze_count = 3;
            alarm->snooze_count = 0;
            alarm->last_triggered = 0;
            alarm->weekday_mask = EVERYDAY;
            
            alarm_mgr.alarm_count++;
            return alarm->id;
        }
    }
    
    return 0xFF; // Hata
}

/**
 * @brief Tarih aralığı ile alarm ekler
 */
uint8_t alarmAddDateRange(const char *name, uint8_t hour, uint8_t minute,
                          alarm_date_t start_date, alarm_date_t end_date,
                          uint8_t weekday_mask,
                          void (*callback)(uint8_t))
{
    uint8_t alarm_id = alarmAddWithDate(name, hour, minute, 
                                        start_date.day, start_date.month, start_date.year,
                                        ALARM_DATE_RANGE, callback);
    
    if (alarm_id != 0xFF) {
        alarm_t *alarm = getAlarmById(alarm_id);
        if (alarm) {
            alarm->end_date = end_date;
            alarm->weekday_mask = weekday_mask;
        }
    }
    
    return alarm_id;
}

/**
 * @brief Alarmı siler
 */
bool alarmRemove(uint8_t alarm_id)
{
    alarm_t *alarm = getAlarmById(alarm_id);
    if (alarm) {
        memset(alarm, 0, sizeof(alarm_t));
        alarm->id = 0xFF;
        alarm->state = ALARM_STATE_DISABLED;
        alarm_mgr.alarm_count--;
        return true;
    }
    return false;
}

/**
 * @brief Alarmı günceller
 */
bool alarmUpdate(uint8_t alarm_id, alarm_t *new_alarm)
{
    alarm_t *alarm = getAlarmById(alarm_id);
    if (alarm && new_alarm) {
        uint8_t saved_id = alarm->id;
        memcpy(alarm, new_alarm, sizeof(alarm_t));
        alarm->id = saved_id; // ID'yi koru
        return true;
    }
    return false;
}

/**
 * @brief Alarmı etkinleştirir
 */
bool alarmEnable(uint8_t alarm_id)
{
    alarm_t *alarm = getAlarmById(alarm_id);
    if (alarm) {
        alarm->state = ALARM_STATE_ENABLED;
        alarm->snooze_count = 0;
        return true;
    }
    return false;
}

/**
 * @brief Alarmı devre dışı bırakır
 */
bool alarmDisable(uint8_t alarm_id)
{
    alarm_t *alarm = getAlarmById(alarm_id);
    if (alarm) {
        alarm->state = ALARM_STATE_DISABLED;
        return true;
    }
    return false;
}

/**
 * @brief Alarmı erteler
 */
bool alarmSnooze(uint8_t alarm_id)
{
    alarm_t *alarm = getAlarmById(alarm_id);
    if (alarm && alarm->state == ALARM_STATE_TRIGGERED) {
        if (alarm->snooze_count < alarm->max_snooze_count) {
            alarm->state = ALARM_STATE_SNOOZED;
            alarm->snooze_count++;
            alarm->snooze_until = time(NULL) + (alarm->snooze_minutes * 60);
            return true;
        }
    }
    return false;
}

/**
 * @brief Alarmı kapatır (dismiss)
 */
bool alarmDismiss(uint8_t alarm_id)
{
    alarm_t *alarm = getAlarmById(alarm_id);
    if (alarm && (alarm->state == ALARM_STATE_TRIGGERED || 
                  alarm->state == ALARM_STATE_SNOOZED)) {
        // Tekrar moduna göre işle
        if (alarm->repeat_mode == ALARM_ONCE) {
            alarm->state = ALARM_STATE_DISABLED;
        } else {
            alarm->state = ALARM_STATE_ENABLED;
        }
        alarm->snooze_count = 0;
        return true;
    }
    return false;
}

/**
 * @brief Alarm bilgisini döndürür
 */
alarm_t* alarmGet(uint8_t alarm_id)
{
    return getAlarmById(alarm_id);
}

/**
 * @brief Aktif alarmların listesini döndürür
 */
uint8_t alarmGetActive(alarm_t **alarm_list)
{
    uint8_t count = 0;
    for (int i = 0; i < MAX_ALARMS && count < MAX_ALARMS; i++) {
        if (alarm_mgr.alarms[i].state == ALARM_STATE_ENABLED) {
            if (alarm_list) {
                alarm_list[count] = &alarm_mgr.alarms[i];
            }
            count++;
        }
    }
    return count;
}

/**
 * @brief Tetiklenmiş alarmların listesini döndürür
 */
uint8_t alarmGetTriggered(alarm_t **alarm_list)
{
    uint8_t count = 0;
    for (int i = 0; i < MAX_ALARMS && count < MAX_ALARMS; i++) {
        if (alarm_mgr.alarms[i].state == ALARM_STATE_TRIGGERED ||
            alarm_mgr.alarms[i].state == ALARM_STATE_SNOOZED) {
            if (alarm_list) {
                alarm_list[count] = &alarm_mgr.alarms[i];
            }
            count++;
        }
    }
    return count;
}

/**
 * @brief Başlangıç tarihini ayarlar
 */
bool alarmSetStartDate(uint8_t alarm_id, uint8_t day, uint8_t month, uint16_t year)
{
    alarm_t *alarm = getAlarmById(alarm_id);
    if (alarm) {
        alarm->start_date.day = day;
        alarm->start_date.month = month;
        alarm->start_date.year = year;
        return true;
    }
    return false;
}

/**
 * @brief Bitiş tarihini ayarlar
 */
bool alarmSetEndDate(uint8_t alarm_id, uint8_t day, uint8_t month, uint16_t year)
{
    alarm_t *alarm = getAlarmById(alarm_id);
    if (alarm) {
        alarm->end_date.day = day;
        alarm->end_date.month = month;
        alarm->end_date.year = year;
        return true;
    }
    return false;
}

/**
 * @brief Tarih aralığını ayarlar
 */
bool alarmSetDateRange(uint8_t alarm_id, alarm_date_t start, alarm_date_t end)
{
    alarm_t *alarm = getAlarmById(alarm_id);
    if (alarm) {
        alarm->start_date = start;
        alarm->end_date = end;
        alarm->repeat_mode = ALARM_DATE_RANGE;
        return true;
    }
    return false;
}

int isleap(int x)
{
    return ((x % 4 == 0) && (x % 100 != 0)) || (x % 400 == 0);
}

bool alarmIsDateValid(alarm_date_t date)
{
    if (date.month > 12 || date.day > 31) {
        return false;
    }
    
    if (date.year != 0 && date.year < 2000) {
        return false;
    }
    
    // Şubat ayı - artık yıl kontrolü ile
    if (date.month == 2) {
        if (isleap(date.year) && date.day > 29) {
            return false;
        }
        if (!isleap(date.year) && date.day > 28) {
            return false;
        }
    }
    
    // 30 günlük aylar (Nisan, Haziran, Eylül, Kasım)
    if ((date.month == 4 || date.month == 6 || date.month == 9 || date.month == 11) && 
        date.day > 30) {
        return false;
    }
    
    return true;
}

/**
 * @brief Tarihin aralık içinde olup olmadığını kontrol eder
 */
bool alarmIsDateInRange(alarm_date_t check_date, alarm_date_t start, alarm_date_t end)
{
    time_t check_time = alarmDateToTime(check_date, 0, 0);
    time_t start_time = alarmDateToTime(start, 0, 0);
    time_t end_time = alarmDateToTime(end, 23, 59);
    
    return (check_time >= start_time && check_time <= end_time);
}

/**
 * @brief İki tarihi karşılaştırır
 */
int alarmCompareDates(alarm_date_t date1, alarm_date_t date2)
{
    if (date1.year != date2.year) {
        return (date1.year > date2.year) ? 1 : -1;
    }
    if (date1.month != date2.month) {
        return (date1.month > date2.month) ? 1 : -1;
    }
    if (date1.day != date2.day) {
        return (date1.day > date2.day) ? 1 : -1;
    }
    return 0; // Eşit
}

/**
 * @brief Alarm tarihini time_t'ye çevirir
 */
time_t alarmDateToTime(alarm_date_t date, uint8_t hour, uint8_t minute)
{
    struct tm tm_date = {0};
    tm_date.tm_year = (date.year > 0) ? date.year - 1900 : 0;
    tm_date.tm_mon = (date.month > 0) ? date.month - 1 : 0;
    tm_date.tm_mday = (date.day > 0) ? date.day : 1;
    tm_date.tm_hour = hour;
    tm_date.tm_min = minute;
    tm_date.tm_sec = 0;
    
    return mktime(&tm_date);
}

/**
 * @brief Süresi dolmuş alarmları döndürür
 */
uint8_t alarmGetExpired(alarm_t **alarm_list)
{
    uint8_t count = 0;
    for (int i = 0; i < MAX_ALARMS && count < MAX_ALARMS; i++) {
        if (alarm_mgr.alarms[i].state == ALARM_STATE_EXPIRED) {
            if (alarm_list) {
                alarm_list[count] = &alarm_mgr.alarms[i];
            }
            count++;
        }
    }
    return count;
}


/* ========== Dahili Yardımcı Fonksiyonlar ========== */

/**
 * @brief Alarm zamanının gelip gelmediğini kontrol eder
 */
static bool isAlarmTimeReached(alarm_t *alarm, struct tm *current_tm, time_t current_time)
{
	 // Tarih aralığı kontrolü
	alarm_date_t current_date ;
    // Saat ve dakika kontrolü
    if (current_tm->tm_hour != alarm->hour || 
        current_tm->tm_min != alarm->minute) {
        return false;
    }
    
    // Saniye toleransı (0-5 saniye arası)
    if (current_tm->tm_sec > 5) {
        return false;
    }
    
    // Tekrar moduna göre kontrol
    switch (alarm->repeat_mode) {
        case ALARM_ONCE:
            // Tarih kontrolü yap
            if (alarm->start_date.year > 0 || alarm->start_date.month > 0 || alarm->start_date.day > 0) {
                if (!checkDateMatch(alarm, current_tm)) {
                    return false;
                }
            }
            break;
            
        case ALARM_DAILY:
            // Her gün çalar, ek kontrol yok
            break;
            
        case ALARM_WEEKDAYS:
            // Pazartesi-Cuma (1-5)
            if (current_tm->tm_wday == 0 || current_tm->tm_wday == 6) {
                return false;
            }
            break;
            
        case ALARM_WEEKEND:
            // Cumartesi-Pazar (0,6)
            if (current_tm->tm_wday >= 1 && current_tm->tm_wday <= 5) {
                return false;
            }
            break;
            
        case ALARM_WEEKLY:
        case ALARM_CUSTOM_DAYS:
            // Hafta günü maskesi kontrolü
            if (!checkWeekdayMatch(alarm, current_tm->tm_wday)) {
                return false;
            }
            break;
            
        case ALARM_MONTHLY:
            // Ayın belirli günü
            if (alarm->start_date.day > 0 && current_tm->tm_mday != alarm->start_date.day) {
                return false;
            }
            break;
            
        case ALARM_YEARLY:
            // Yılın belirli günü
            if ((alarm->start_date.month > 0 && (current_tm->tm_mon + 1) != alarm->start_date.month) ||
                (alarm->start_date.day > 0 && current_tm->tm_mday != alarm->start_date.day)) {
                return false;
            }
            break;
            
        case ALARM_DATE_RANGE:
            // Tarih aralığı kontrolü
            
			current_date.day = current_tm->tm_mday;
			current_date.month = current_tm->tm_mon + 1;
			current_date.year = current_tm->tm_year + 1900;
         
            
            if (!alarmIsDateInRange(current_date, alarm->start_date, alarm->end_date)) {
                return false;
            }
            
            // Hafta günü kontrolü
            if (!checkWeekdayMatch(alarm, current_tm->tm_wday)) {
                return false;
            }
            break;
    }
    
    return true;
}

/**
 * @brief Alarm süresinin dolup dolmadığını kontrol eder
 */
static bool isAlarmExpired(alarm_t *alarm, struct tm *current_tm)
{
    // Sadece tek seferlik veya tarih aralığı olan alarmlar expire olabilir
    if (alarm->repeat_mode != ALARM_ONCE && alarm->repeat_mode != ALARM_DATE_RANGE) {
        return false;
    }
    
    alarm_date_t current_date = {
        .day = current_tm->tm_mday,
        .month = current_tm->tm_mon + 1,
        .year = current_tm->tm_year + 1900
    };
    
    if (alarm->repeat_mode == ALARM_ONCE) {
        // Tek seferlik alarm - tarih geçtiyse expire
        if (alarm->start_date.year > 0) {
            return alarmCompareDates(current_date, alarm->start_date) > 0;
        }
    } else if (alarm->repeat_mode == ALARM_DATE_RANGE) {
        // Tarih aralığı - bitiş tarihini geçtiyse expire
        return alarmCompareDates(current_date, alarm->end_date) > 0;
    }
    
    return false;
}

/**
 * @brief Hafta günü eşleşmesini kontrol eder
 */
static bool checkWeekdayMatch(alarm_t *alarm, int weekday)
{
    return (alarm->weekday_mask & (1 << weekday)) != 0;
}

/**
 * @brief Tarih eşleşmesini kontrol eder
 */
static bool checkDateMatch(alarm_t *alarm, struct tm *current_tm)
{
    if (alarm->start_date.year > 0 && (current_tm->tm_year + 1900) != alarm->start_date.year) {
        return false;
    }
    if (alarm->start_date.month > 0 && (current_tm->tm_mon + 1) != alarm->start_date.month) {
        return false;
    }
    if (alarm->start_date.day > 0 && current_tm->tm_mday != alarm->start_date.day) {
        return false;
    }
    return true;
}



/**
 * @brief Bir sonraki alarm zamanını döndürür
 */
time_t alarmGetNextAlarmTime(void)
{
    time_t current_time = time(NULL);
    time_t next_alarm = 0;
    struct tm current_tm = *localtime(&current_time);
    
    for (int i = 0; i < MAX_ALARMS; i++) {
        alarm_t *alarm = &alarm_mgr.alarms[i];
        
        if (alarm->state != ALARM_STATE_ENABLED) {
            continue;
        }
        
        // Bugünkü alarm zamanını hesapla
        struct tm alarm_tm = current_tm;
        alarm_tm.tm_hour = alarm->hour;
        alarm_tm.tm_min = alarm->minute;
        alarm_tm.tm_sec = alarm->second;
        
        time_t alarm_time = mktime(&alarm_tm);
        
        // Eğer alarm zamanı geçmişse, tekrar moduna göre sonraki zamanı bul
        if (alarm_time <= current_time) {
            switch (alarm->repeat_mode) {
                case ALARM_DAILY:
                    alarm_time += 86400; // +1 gün
                    break;
                case ALARM_WEEKLY:
                    alarm_time += 604800; // +7 gün
                    break;
                case ALARM_MONTHLY:
                    // Bir sonraki ay
                    alarm_tm.tm_mon++;
                    alarm_time = mktime(&alarm_tm);
                    break;
                case ALARM_YEARLY:
                    // Bir sonraki yıl
                    alarm_tm.tm_year++;
                    alarm_time = mktime(&alarm_tm);
                    break;
                case ALARM_ONCE:
                    // Tek seferlik alarmlar için tarih kontrolü
                    if (alarm->start_date.year > 0) 
					{
                        alarm_date_t alarm_date = alarm->start_date;
                        alarm_time = alarmDateToTime(alarm_date, alarm->hour, alarm->minute);
                        if (alarm_time <= current_time) 
						{
                            continue; // Bu alarm geçmiş
                        }
                    }
                    break;
                case ALARM_DATE_RANGE:
                    // Tarih aralığında ise bir sonraki uygun günü bul
                    for (int day = 1; day <= 7; day++) 
					{
                        alarm_tm.tm_mday++;
                        alarm_time = mktime(&alarm_tm);
                        struct tm *check_tm = localtime(&alarm_time);
                        
                        alarm_date_t check_date = 
						{
                            .day = check_tm->tm_mday,
                            .month = check_tm->tm_mon + 1,
                            .year = check_tm->tm_year + 1900
                        };
                        
                        if (alarmIsDateInRange(check_date, alarm->start_date, alarm->end_date) &&
                            checkWeekdayMatch(alarm, check_tm->tm_wday)) {
                            break;
                        }
                    }
                    break;
                default:
                    // Diğer modlar için daha karmaşık hesaplama gerekli
                    continue;
            }
        }
        
        // En yakın alarmı bul
        if (next_alarm == 0 || alarm_time < next_alarm) {
            next_alarm = alarm_time;
        }
    }
    
    return next_alarm;
}

/**
 * @brief Herhangi bir aktif alarm var mı kontrol eder
 */
bool alarmIsAnyActive(void)
{
    for (int i = 0; i < MAX_ALARMS; i++) {
        if (alarm_mgr.alarms[i].state == ALARM_STATE_TRIGGERED ||
            alarm_mgr.alarms[i].state == ALARM_STATE_SNOOZED) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Hafta günü maskesini ayarlar
 */
bool alarmSetWeekdayMask(uint8_t alarm_id, uint8_t mask)
{
    alarm_t *alarm = getAlarmById(alarm_id);
    if (alarm) {
        alarm->weekday_mask = mask;
        return true;
    }
    return false;
}

/**
 * @brief Erteleme ayarlarını yapar
 */
bool alarmSetSnooze(uint8_t alarm_id, uint8_t minutes, uint8_t max_count)
{
    alarm_t *alarm = getAlarmById(alarm_id);
    if (alarm) 
	{
        alarm->snooze_minutes = minutes;
        alarm->max_snooze_count = max_count;
        return true;
    }
    return false;
}

/**
 * @brief Alarm önceliğini ayarlar
 */
bool alarmSetPriority(uint8_t alarm_id, alarm_priority_t priority)
{
    alarm_t *alarm = getAlarmById(alarm_id);
    if (alarm) 
	{
        alarm->priority = priority;
        return true;
    }
    return false;
}

/**
 * @brief Bir sonraki boş ID'yi bulur
 */
static uint8_t getNextFreeId(void)
{
    static uint8_t next_id = 1;
    uint8_t start_id = next_id;
    
    do {
        bool id_used = false;
        for (int i = 0; i < MAX_ALARMS; i++) {
            if (alarm_mgr.alarms[i].id == next_id) {
                id_used = true;
                break;
            }
        }
        
        if (!id_used) {
            uint8_t ret = next_id;
            next_id = (next_id % 254) + 1; // 1-254 arası döner
            return ret;
        }
        
        next_id = (next_id % 254) + 1;
    } while (next_id != start_id);
    
    return 0xFF; // Tüm ID'ler dolu
}

/**
 * @brief Global callback fonksiyonlarını ayarlar
 */
void alarmSetGlobalCallback(void (*on_any)(uint8_t), void (*on_missed)(uint8_t), void (*on_expired)(uint8_t))
{
    alarm_mgr.on_any_alarm = on_any;
    alarm_mgr.on_alarm_missed = on_missed;
    alarm_mgr.on_alarm_expired = on_expired;
}

/**
 * @brief ID'ye göre alarm bulur
 */
static alarm_t* getAlarmById(uint8_t alarm_id)
{
    if (alarm_id == 0xFF) {
        return NULL;
    }
    
    for (int i = 0; i < MAX_ALARMS; i++) {
        if (alarm_mgr.alarms[i].id == alarm_id) {
            return &alarm_mgr.alarms[i];
        }
    }
    return NULL;
}

/**
 * @brief Alarm durumunu yazdırır (debug)
 */
void alarmPrintStatus(void)
{
    printf("Alarm Manager Status:\n");
    printf("Total alarms: %d/%d\n", alarm_mgr.alarm_count, MAX_ALARMS);
    printf("Initialized: %s\n", alarm_mgr.initialized ? "Yes" : "No");
    
    for (int i = 0; i < MAX_ALARMS; i++) {
        if (alarm_mgr.alarms[i].id != 0xFF) {
            alarmPrint(alarm_mgr.alarms[i].id);
        }
    }
}

/**
 * @brief Tek bir alarmın detaylarını yazdırır
 */
void alarmPrint(uint8_t alarm_id)
{
    alarm_t *alarm = getAlarmById(alarm_id);
    if (alarm) {
        printf("Alarm #%d: %s\n", alarm->id, alarm->name);
        printf("  Time: %02d:%02d:%02d\n", alarm->hour, alarm->minute, alarm->second);
        printf("  State: %d, Priority: %d\n", alarm->state, alarm->priority);
        printf("  Repeat: %d, Snooze: %d min x %d\n", 
               alarm->repeat_mode, alarm->snooze_minutes, alarm->max_snooze_count);
        
        // Tarih bilgileri
        if (alarm->start_date.year > 0 || alarm->start_date.month > 0 || alarm->start_date.day > 0) {
            printf("  Start Date: %02d/%02d/%04d\n", 
                   alarm->start_date.day, alarm->start_date.month, alarm->start_date.year);
        }
        
        if (alarm->repeat_mode == ALARM_DATE_RANGE) {
            printf("  End Date: %02d/%02d/%04d\n", 
                   alarm->end_date.day, alarm->end_date.month, alarm->end_date.year);
        }
        
        // Hafta günü maskesi
        if (alarm->weekday_mask != EVERYDAY) {
            printf("  Days: ");
            const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
            for (int i = 0; i < 7; i++) {
                if (alarm->weekday_mask & (1 << i)) {
                    printf("%s ", days[i]);
                }
            }
            printf("\n");
        }
    }
}

/**
 * @brief Gelecek N gün içindeki alarmları yazdırır
 */
void alarmPrintUpcoming(uint8_t days_ahead)
{
    time_t current_time = time(NULL);
    time_t end_time = current_time + (days_ahead * 24 * 60 * 60);
    
    printf("\n=== Upcoming Alarms (%d days) ===\n", days_ahead);
    
    for (int i = 0; i < MAX_ALARMS; i++) {
        alarm_t *alarm = &alarm_mgr.alarms[i];
        if (alarm->state == ALARM_STATE_ENABLED) {
            // Basit kontrol - geliştirilmeye açık
            printf("[%d] %s - %02d:%02d\n", 
                   alarm->id, alarm->name, alarm->hour, alarm->minute);
        }
    }
    printf("=========================\n");
}
