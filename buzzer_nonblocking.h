#ifndef BUZZER_NONBLOCKING_H
#define BUZZER_NONBLOCKING_H

#include "stdint.h"

// Buzzer durumlari
typedef enum {
    BUZZER_IDLE = 0,
    BUZZER_PLAYING,
    BUZZER_PAUSE
} buzzer_state_t;

// Pattern yapisi - frekans ve süre çiftleri
typedef struct {
    uint16_t frequency;  // Hz cinsinden frekans (0 = sessizlik)
    uint32_t duration;   // ms cinsinden süre
} buzzer_note_t;

// Buzzer kontrol yapisi
typedef struct {
    buzzer_state_t state;
    const buzzer_note_t *pattern;  // Çalinacak pattern
    uint8_t pattern_length;        // Pattern'deki note sayisi
    uint8_t current_note;          // Su anki note index'i
    uint8_t repeat_count;          // Kaç kez tekrarlanacak (0 = sonsuz)
    uint8_t current_repeat;        // Su anki tekrar sayisi
    uint8_t volume;                // Ses seviyesi (0-100)
    uint32_t note_start_time;      // Note baslama zamani
} buzzer_control_t;

// Public API
void buzzerInit(void);
void buzzerUpdate(uint32_t current_time);
void buzzerPlayPattern(const buzzer_note_t *pattern, uint8_t length, uint8_t volume, uint8_t repeat);
void buzzerStop(void);
uint8_t buzzerIsPlaying(void);


#endif
