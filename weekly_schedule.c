#include "weekly_schedule.h"
#include <string.h>
#include <stdio.h>

// Global haftalık program
weekly_schedule_t g_weekly_schedule = {0};

/**
 * @brief Haftalık program sistemini başlatır
 */
void weeklyScheduleInit(void)
{
    memset(&g_weekly_schedule, 0, sizeof(weekly_schedule_t));
    g_weekly_schedule.current_mode = SCHEDULE_MODE_ONOFF;
    g_weekly_schedule.is_active = 1;
}

/**
 * @brief Haftalık program sistemini kapatır
 */
void weeklyScheduleDeInit(void)
{
	memset(&g_weekly_schedule, 0, sizeof(weekly_schedule_t));
    g_weekly_schedule.current_mode = SCHEDULE_MODE_NONE;
    g_weekly_schedule.is_active = 0;
}

/**
 * @brief Program modunu ayarlar
 */
void weeklyScheduleSetMode(schedule_mode_t mode)
{
    g_weekly_schedule.current_mode = mode;
    // Mod değiştiğinde tüm değerleri temizle
    weeklyScheduleClearAll();
}

uint8_t weeklyScheduleIsActive(void)
{
	return g_weekly_schedule.is_active;
}
/**
 * @brief Mevcut saat index'ini hesaplar (0-167)
 */
uint8_t weeklyScheduleGetCurrentHourIndex(void)
{
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    
    // tm_wday: 0=Pazar, 1=Pazartesi, ..., 6=Cumartesi
    // tm_hour: 0-23
    uint8_t day = tm_now->tm_wday;
    uint8_t hour = tm_now->tm_hour;
    
    return weeklyScheduleDayHourToIndex(day, hour);
}

/**
 * @brief Gün ve saat bilgisini index'e çevirir
 */
uint8_t weeklyScheduleDayHourToIndex(uint8_t day, uint8_t hour)
{
    if (day >= 7 || hour >= 24) return 0;
    return (day * 24) + hour;
}

/**
 * @brief Index'i gün ve saat bilgisine çevirir
 */
void weeklyScheduleIndexToDayHour(uint8_t index, uint8_t *day, uint8_t *hour)
{
    if (index >= HOURS_IN_WEEK) {
        *day = 0;
        *hour = 0;
        return;
    }
    
    *day = index / 24;
    *hour = index % 24;
}

/**
 * @brief Mevcut saatin değerini döndürür
 */
uint16_t weeklyScheduleGetCurrentValue(void)
{
    uint8_t current_index = weeklyScheduleGetCurrentHourIndex();
    return g_weekly_schedule.schedule_hours[current_index];
}

/**
 * @brief Belirtilen saatin değerini ayarlar
 */
void weeklyScheduleSetHour(uint8_t hour_index, uint16_t value)
{
    if (hour_index >= HOURS_IN_WEEK) return;
    
    // Mod kontrolü ve değer sınırlaması
    switch (g_weekly_schedule.current_mode) {
        case SCHEDULE_MODE_ONOFF:
            // Mode 1: Sadece 0 veya 1
            g_weekly_schedule.schedule_hours[hour_index] = (value > 0) ? 1 : 0;
            break;
            
        case SCHEDULE_MODE_LEVEL:
            // Mode 2: 0-3 arası
            if (value > 3) value = 3;
            g_weekly_schedule.schedule_hours[hour_index] = value;
            break;
            
        case SCHEDULE_MODE_TEMP:
            // Mode 3: Sıcaklık değeri (0-100°C arası varsayıyoruz)
            if (value > 100) value = 100;
            g_weekly_schedule.schedule_hours[hour_index] = value;
            break;
		
		default:
			break;
    }
}

/**
 * @brief Belirtilen saatin değerini döndürür
 */
uint16_t weeklyScheduleGetHour(uint8_t hour_index)
{
    if (hour_index >= HOURS_IN_WEEK) return 0;
    return g_weekly_schedule.schedule_hours[hour_index];
}

/**
 * @brief Mevcut saatin programını işler, periodik call
 */
void weeklyScheduleProcessCurrentHour(void)
{
    if (!g_weekly_schedule.is_active) return;
    
    uint16_t current_value = weeklyScheduleGetCurrentValue();
    
    switch (g_weekly_schedule.current_mode) {
        case SCHEDULE_MODE_ONOFF:
            // Mode 1: On/Off kontrolü
            if (current_value == 1) {
                // Cihazı aç
                // setRelay(1);
                // setHeating(1);
                //printf("Weekly Schedule: Device ON\n");
            } else {
                // Cihazı kapat
                // setRelay(0);
                // setHeating(0);
                //printf("Weekly Schedule: Device OFF\n");
            }
            break;
            
        case SCHEDULE_MODE_LEVEL:
            // Mode 2: Seviye kontrolü (0-3)
           // printf("Weekly Schedule: Level %d\n", current_value);
            // switch (current_value) {
            //     case 0: // Kapalı
            //     case 1: // Düşük seviye
            //     case 2: // Orta seviye  
            //     case 3: // Yüksek seviye
            // }
            break;
            
        case SCHEDULE_MODE_TEMP:
            // Mode 3: Sıcaklık kontrolü
            //printf("Weekly Schedule: Target temp %d°C\n", current_value);
            // setTargetTemperature(current_value);
            break;
		default:
			break;
    }
}

/**
 * @brief Tüm saatleri temizler
 */
void weeklyScheduleClearAll(void)
{
    memset(g_weekly_schedule.schedule_hours, 0, sizeof(g_weekly_schedule.schedule_hours));
}

/**
 * @brief Belirtilen günün tüm saatlerini ayarlar
 */
void weeklyScheduleSetDay(uint8_t day, uint16_t value)
{
    if (day >= 7) return;
    
    for (uint8_t hour = 0; hour < 24; hour++) {
        uint8_t index = weeklyScheduleDayHourToIndex(day, hour);
        weeklyScheduleSetHour(index, value);
    }
}

/**
 * @brief Haftalık programın tamamını yazdırır
 */
void weeklySchedulePrintWeek(void)
{
    const char *day_names[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    
    printf("\n=== WEEKLY SCHEDULE (Mode: %d) ===\n", g_weekly_schedule.current_mode);
    
    for (uint8_t day = 0; day < 7; day++) {
        printf("%s: ", day_names[day]);
        for (uint8_t hour = 0; hour < 24; hour++) {
            uint8_t index = weeklyScheduleDayHourToIndex(day, hour);
            printf("%2d ", g_weekly_schedule.schedule_hours[index]);
            
            // 6'şar saat grupla
            if ((hour + 1) % 6 == 0) printf("| ");
        }
        printf("\n");
    }
    printf("================================\n");
}

/**
 * @brief Belirtilen günün programını yazdırır
 */
void weeklySchedulePrintDay(uint8_t day)
{
    const char *day_names[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    
    if (day >= 7) return;
    
    printf("\n=== %s SCHEDULE ===\n", day_names[day]);
    
    for (uint8_t hour = 0; hour < 24; hour++) {
        uint8_t index = weeklyScheduleDayHourToIndex(day, hour);
        uint16_t value = g_weekly_schedule.schedule_hours[index];
        printf("%02d:00 -> %d\n", hour, value);
    }
    printf("==================\n");
}
