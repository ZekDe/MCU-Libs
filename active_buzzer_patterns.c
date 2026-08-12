/**
* @Author: Emrah Duatepe
* @brief   Devreli (aktif) buzzer icin on/off pattern'ler.
*/

#include "active_buzzer_patterns.h"
#include "active_buzzer_nonblocking.h"
#include "device_context.h"
/* Morse SOS:  ... --- ...
   dot=200ms, dash=600ms (3xdot), eleman araligi=200ms, harf araligi=600ms,
   tekrar arasi=1400ms. repeat=0 -> sonsuz (imdat sinyali). */
static const buzzer_note_t pattern_sos[] =
{
    {1,200},{0,200}, {1,200},{0,200}, {1,200},{0,600},   /* S */
    {1,600},{0,200}, {1,600},{0,200}, {1,600},{0,600},   /* O */
    {1,200},{0,200}, {1,200},{0,200}, {1,200},{0,1400},  /* S + tekrar oncesi uzun bosluk */
};

void playSOS(void)
{
    if (!buzzerIsPlaying())
        buzzerPlayPattern(pattern_sos, sizeof(pattern_sos)/sizeof(pattern_sos[0]), 0);
}

/* Tek kisa bip (sure parametreli). */
static buzzer_note_t beep_pattern[] = { {1, 100}, {0, 1} };

void playBuzzerBeep(uint32_t ms)
{
	if(notification_level < 3) return;
    beep_pattern[0].duration = ms;
    if (!buzzerIsPlaying())
        buzzerPlayPattern(beep_pattern, 2, 1);
}

static buzzer_note_t beep3_pattern[] = { {1, 100}, {0, 1}, {1, 100}, {0, 1}, {1, 100}, {0, 1}};

void playBuzzer3Beep(uint32_t active_ms, uint32_t inactive_ms)
{
	if(notification_level < 3) return;
    beep3_pattern[0].duration = active_ms;
	beep3_pattern[1].duration = inactive_ms;
	beep3_pattern[2].duration = active_ms;
	beep3_pattern[3].duration = inactive_ms;
	beep3_pattern[4].duration = active_ms;

    if (!buzzerIsPlaying())
        buzzerPlayPattern(beep3_pattern, sizeof(beep3_pattern)/sizeof(beep3_pattern[0]), 1);
}

/* Tek kisa bip (sure parametreli). */
static buzzer_note_t beepbeep_pattern[] = { {1, 100}, {0, 1}, {1, 100}, {0, 1} };

void playBuzzerBeepBeep(uint32_t active_ms, uint32_t inactive_ms)
{
	if(notification_level < 3) return;
    beepbeep_pattern[0].duration = active_ms;
	beepbeep_pattern[1].duration = inactive_ms;
	beepbeep_pattern[2].duration = active_ms;
    if (!buzzerIsPlaying())
        buzzerPlayPattern(beepbeep_pattern, sizeof(beepbeep_pattern)/sizeof(beepbeep_pattern[0]), 1);
}

/* Basit alarm: bip-bip, sonsuz. */
static const buzzer_note_t alarm_pattern[] =
{
    {1,400},{0,200}, {1,400},{0,800}
};

void playBuzzerAlarm(void)
{
	if(notification_level < 2) return;
    if (!buzzerIsPlaying())
        buzzerPlayPattern(alarm_pattern, sizeof(alarm_pattern)/sizeof(alarm_pattern[0]), 0);
}

static buzzer_note_t findmydevice_pattern[] = { {1, 500}, {0, 500} };

void playBuzzerFindMyDevice(void)
{
    if (!buzzerIsPlaying())
        buzzerPlayPattern(findmydevice_pattern, 2, 0);
}

static buzzer_note_t child_lock_pattern[] = { {1,50}, {0,50}, {1,50}, 
{0,50}, {1,50}, {0,700}};

void playBuzzerChildLock(void)
{
	if(notification_level < 3) return;
    if (!buzzerIsPlaying())
        buzzerPlayPattern(child_lock_pattern, 6, 1);
}