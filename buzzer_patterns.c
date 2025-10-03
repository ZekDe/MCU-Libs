#include "buzzer_patterns.h"
#include "buzzer_nonblocking.h"

typedef enum {
    NOTE_C4  = 262,  NOTE_CS4 = 277,  NOTE_D4  = 294,  NOTE_DS4 = 311,  NOTE_E4  = 330,
    NOTE_F4  = 349,  NOTE_FS4 = 370,  NOTE_G4  = 392,  NOTE_GS4 = 415,  NOTE_A4  = 440,
    NOTE_AS4 = 466,  NOTE_B4  = 494,  NOTE_C5  = 523,  NOTE_CS5 = 554,  NOTE_D5  = 587,
    NOTE_DS5 = 622,  NOTE_E5  = 659,  NOTE_F5  = 698,  NOTE_FS5 = 740,  NOTE_G5  = 784,
    NOTE_GS5 = 831,  NOTE_A5  = 880,  NOTE_AS5 = 932,  NOTE_B5  = 988,  NOTE_C6  = 1047,
    NOTE_REST = 0
} musical_note_t;




const buzzer_note_t pattern_whatsapp[] = 
{
	{NOTE_E5, 100},  
	{NOTE_REST, 50}, 
	{NOTE_C5, 150},  
	{NOTE_REST, 100}  
};
const uint8_t pattern_whatsapp_length = sizeof(pattern_whatsapp)/sizeof(pattern_whatsapp[0]);


const buzzer_note_t pattern_startup[] = {
    {NOTE_C4, 150}, {NOTE_E4, 150}, {NOTE_G4, 150}, {NOTE_C5, 300}, {NOTE_REST, 100}
};
const uint8_t pattern_startup_length = sizeof(pattern_startup)/sizeof(pattern_startup[0]);

const buzzer_note_t pattern_shutdown[] = {
    {NOTE_C5, 150}, {NOTE_G4, 150}, {NOTE_E4, 150}, {NOTE_C4, 300}, {NOTE_REST, 100}
};
const uint8_t pattern_shutdown_length = sizeof(pattern_shutdown)/sizeof(pattern_shutdown[0]);

const buzzer_note_t pattern_error[] = {
    {NOTE_A4, 200}, {NOTE_REST, 100}, {NOTE_A4, 200}, {NOTE_REST, 100}, {NOTE_A4, 200}, {NOTE_REST, 300}
};
const uint8_t pattern_error_length = sizeof(pattern_error)/sizeof(pattern_error[0]);

const buzzer_note_t pattern_warning[] = {
    {NOTE_F4, 100}, {NOTE_REST, 50}, {NOTE_F4, 100}, {NOTE_REST, 200}
};
const uint8_t pattern_warning_length = sizeof(pattern_warning)/sizeof(pattern_warning[0]);

// Notification sesleri
const buzzer_note_t pattern_notification[] = {
    {NOTE_E5, 100}, {NOTE_REST, 50}, {NOTE_E5, 100}, {NOTE_REST, 100}
};
const uint8_t pattern_notification_length = sizeof(pattern_notification)/sizeof(pattern_notification[0]);

const buzzer_note_t pattern_timer_finish[] = {
    {NOTE_G5, 200}, {NOTE_G5, 200}, {NOTE_G5, 200}, {NOTE_C6, 600}, {NOTE_REST, 200}
};
const uint8_t pattern_timer_finish_length = sizeof(pattern_timer_finish)/sizeof(pattern_timer_finish[0]);


const buzzer_note_t pattern_super_mario[] = {
    {NOTE_E5, 150}, {NOTE_E5, 150}, {NOTE_REST, 150}, {NOTE_E5, 150}, {NOTE_REST, 150}, {NOTE_C5, 150}, {NOTE_E5, 150},
    {NOTE_G5, 600}, {NOTE_REST, 600}, {NOTE_G4, 600}, {NOTE_REST, 600},
    {NOTE_C5, 450}, {NOTE_G4, 150}, {NOTE_REST, 450}, {NOTE_E4, 450}, {NOTE_A4, 300}, {NOTE_B4, 300}, {NOTE_AS4, 150}, {NOTE_A4, 450},
    {NOTE_G4, 200}, {NOTE_E5, 200}, {NOTE_G5, 200}, {NOTE_A5, 450}, {NOTE_F5, 150}, {NOTE_G5, 150}, {NOTE_REST, 150}, {NOTE_E5, 450}, {NOTE_C5, 150}, {NOTE_D5, 150}, {NOTE_B4, 450},
    {NOTE_REST, 200}
};
const uint8_t pattern_super_mario_length = sizeof(pattern_super_mario)/sizeof(pattern_super_mario[0]);



void playWhatsAppNotification(void) 
{
    if (!buzzerIsPlaying())
        buzzerPlayPattern(pattern_whatsapp, pattern_whatsapp_length, 30, 1);
}


void playStartupSound(void) {
    if (!buzzerIsPlaying()) {
        buzzerPlayPattern(pattern_startup, pattern_startup_length, 30, 1);
    }
}

void playShutdownSound(void) {
    if (!buzzerIsPlaying()) {
        buzzerPlayPattern(pattern_shutdown, pattern_shutdown_length, 30, 1);
    }
}

void playErrorSound(void) {
    if (!buzzerIsPlaying()) {
        buzzerPlayPattern(pattern_error, pattern_error_length, 30, 1);
    }
}

void playWarningSound(void) {
    if (!buzzerIsPlaying()) {
        buzzerPlayPattern(pattern_warning, pattern_warning_length, 30, 3); // 3 kez tekrar
    }
}

void playNotificationSound(void) {
    if (!buzzerIsPlaying()) {
        buzzerPlayPattern(pattern_notification, pattern_notification_length, 30, 1);
    }
}

void playTimerFinishSound(void) {
    if (!buzzerIsPlaying()) {
        buzzerPlayPattern(pattern_timer_finish, pattern_timer_finish_length, 30, 1);
    }
}

void playSuperMario(void) {
    if (!buzzerIsPlaying()) {
        buzzerPlayPattern(pattern_super_mario, pattern_super_mario_length, 30, 1);
    }
}




/**
 * @brief Kisa bip sesi
 */
void playBuzzerBeep(uint32_t ms)
{
	static buzzer_note_t beep_pattern[] = {
    {1760, 200},  // 1760 freq, 200ms
    {0, 5}      // Sessizlik, 5ms
};
	beep_pattern[0].duration = ms;

	if (!buzzerIsPlaying()) 
		buzzerPlayPattern(beep_pattern, sizeof(beep_pattern)/sizeof(beep_pattern[0]), 30, 1);
}

/**
 * @brief Basari sesi (yükselen ton)
 */
void playBuzzerOK(void)
{
	static const buzzer_note_t ok_pattern[] = {
		{659, 150},  // E5, 150ms
		{784, 150},  // G5, 150ms
		{0, 100}     // Sessizlik
	};

	if (!buzzerIsPlaying()) 
		buzzerPlayPattern(ok_pattern, sizeof(ok_pattern)/sizeof(ok_pattern[0]), 30, 1);
}

/**
 * @brief Hata sesi (düsen ton)
 */
void playBuzzerNOK(void)
{
	static const buzzer_note_t nok_pattern[] = {
		{784, 150},  // G5, 150ms
		{659, 150},  // E5, 150ms
		{0, 100}     // Sessizlik
	};

	if (!buzzerIsPlaying()) 
		buzzerPlayPattern(nok_pattern, sizeof(nok_pattern)/sizeof(nok_pattern[0]), 50, 1);
}

/**
 * @brief Alarm sesi (sürekli tekrar)
 */
void playBuzzerAlarm(void)
{
	static const buzzer_note_t alarm_pattern[] = {
    {880, 500},  // A5, 500ms
    {0, 200},    // Sessizlik, 200ms
    {880, 500},  // A5, 500ms
    {0, 500}     // Sessizlik, 500ms
};
	if (!buzzerIsPlaying()) 
		buzzerPlayPattern(alarm_pattern, sizeof(alarm_pattern)/sizeof(alarm_pattern[0]), 70, 0); // 0 = sonsuz
}