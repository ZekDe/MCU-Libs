#ifndef BUZZER_PATTERN_H
#define BUZZER_PATTERN_H

#include "stdint.h"

void playStartupSound(void);
void playShutdownSound(void);
void playErrorSound(void);
void playWarningSound(void);
void playNotificationSound(void);
void playTimerFinishSound(void);
void playWhatsAppNotification(void);
void playSuperMario(void);

void playBuzzerBeep(uint32_t ms);
void playBuzzerOK(void);
void playBuzzerNOK(void);
void playBuzzerAlarm(void);
#endif