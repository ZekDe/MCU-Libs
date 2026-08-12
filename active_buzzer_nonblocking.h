/**
 * @file    buzzer_nonblocking.h
 * @brief   Devreli (aktif) buzzer icin non-blocking pattern calici.
 *
 *  Aktif buzzer sadece AC/KAPA edilir (kendi osilatoru var), frekans yok.
 *  Pattern = {on, sure(ms)} ciftleri:
 *      const buzzer_note_t p[] = { {1,100}, {0,50}, {1,150}, {0,100} };
 *  buzzerPlayPattern(p, len, repeat) ile baslat; main loop'ta buzzerUpdate(systick).
 */
#ifndef ACTIVE_BUZZER_NONBLOCKING_H
#define ACTIVE_BUZZER_NONBLOCKING_H

#include "stdint.h"

typedef struct {
    uint8_t  on;        /**< 1 = buzzer acik, 0 = sessiz */
    uint32_t duration;  /**< ms */
} buzzer_note_t;

void    buzzerInit(void);
/* repeat: kac kez calinacak (0 = sonsuz) */
void    buzzerPlayPattern(const buzzer_note_t *pattern, uint8_t length, uint8_t repeat);
void    buzzerUpdate(uint32_t now);   /* main loop'ta her tur cagrilmali */
void    buzzerStop(void);
uint8_t buzzerIsPlaying(void);

#endif /* BUZZER_NONBLOCKING_H */
