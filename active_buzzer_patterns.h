#ifndef ACTIVE_BUZZER_PATTERNS_H
#define ACTIVE_BUZZER_PATTERNS_H

#include "stdint.h"

void playSOS(void);                /* sonsuz SOS  ( ... --- ... )  */
void playBuzzerBeep(uint32_t ms);  /* tek kisa bip                 */
void playBuzzerBeepBeep(uint32_t active_ms, uint32_t inactive_ms);
void playBuzzer3Beep(uint32_t active_ms, uint32_t inactive_ms);
void playBuzzerAlarm(void);        /* sonsuz basit alarm (bip-bip) */
void playBuzzerFindMyDevice(void);
void playBuzzerChildLock(void);
#endif
