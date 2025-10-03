#include "buzzer_nonblocking.h"
#include "SW2023.h"
#include "systemtick.h"

// Melody tablosu (mevcut kodundan)
const uint16_t melody[] = {
    261, 277, 294, 311, 329, 349, 370, 392, 415, 440, 466, 494, // Oktav 4
    523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988, // Oktav 5
    1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976, // Oktav 6
    2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951, // Oktav 7
    4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902  // Oktav 8
};
#define MELODY_SIZE (sizeof(melody)/sizeof(melody[0]))

// Global buzzer control
static buzzer_control_t buzzer_ctrl;

// Iç fonksiyonlar
static void setBuzzerHardware(uint16_t frequency, uint8_t volume);

/**
 * @brief Buzzer sistemini baslatir
 */
void buzzerInit(void)
{
    buzzer_ctrl.state = BUZZER_IDLE;
    buzzer_ctrl.pattern = NULL;
    buzzer_ctrl.pattern_length = 0;
    buzzer_ctrl.current_note = 0;
    buzzer_ctrl.repeat_count = 0;
    buzzer_ctrl.current_repeat = 0;
    buzzer_ctrl.volume = 50;
    buzzer_ctrl.note_start_time = 0;
}

/**
 * @brief Buzzer güncelleme fonksiyonu - main loop'ta sürekli çagrilmali
 * @param current_time: systick degeri
 */
void buzzerUpdate(uint32_t current_time)
{
    if (buzzer_ctrl.state == BUZZER_IDLE) {
        return;
    }

    // Mevcut note'un süresi doldu mu?
    uint32_t elapsed = current_time - buzzer_ctrl.note_start_time;
    uint32_t note_duration = buzzer_ctrl.pattern[buzzer_ctrl.current_note].duration;
    
    if (elapsed >= note_duration) {
        // Sonraki note'a geç
        buzzer_ctrl.current_note++;
        
        // Pattern sonu mu?
        if (buzzer_ctrl.current_note >= buzzer_ctrl.pattern_length) {
            buzzer_ctrl.current_note = 0;
            buzzer_ctrl.current_repeat++;
            
            // Tekrar sayisi doldu mu?
            if (buzzer_ctrl.repeat_count > 0 && 
                buzzer_ctrl.current_repeat >= buzzer_ctrl.repeat_count) {
                buzzerStop();
                return;
            }
        }
        
        // Yeni note'u çal
        uint16_t freq = buzzer_ctrl.pattern[buzzer_ctrl.current_note].frequency;
        if (freq == 0) {
            setBuzzerHardware(0, 0);  // Sessizlik
        } else {
            setBuzzerHardware(freq, buzzer_ctrl.volume);
        }
        
        buzzer_ctrl.note_start_time = current_time;
    }
}

/**
 * @brief Pattern çalmaya baslar
 * @param pattern: Çalinacak note dizisi
 * @param length: Pattern uzunlugu
 * @param volume: Ses seviyesi (0-100)
 * @param repeat: Tekrar sayisi (0 = sonsuz)
 */
void buzzerPlayPattern(const buzzer_note_t *pattern, uint8_t length, 
                      uint8_t volume, uint8_t repeat)
{
    if (pattern == NULL || length == 0) {
        return;
    }
    
    buzzer_ctrl.pattern = pattern;
    buzzer_ctrl.pattern_length = length;
    buzzer_ctrl.current_note = 0;
    buzzer_ctrl.repeat_count = repeat;
    buzzer_ctrl.current_repeat = 0;
    buzzer_ctrl.volume = volume;
    buzzer_ctrl.state = BUZZER_PLAYING;
    buzzer_ctrl.note_start_time = systick;
    
    // Ilk note'u çal
    uint16_t freq = pattern[0].frequency;
    if (freq == 0) {
        setBuzzerHardware(0, 0);
    } else {
        setBuzzerHardware(freq, volume);
    }
}

/**
 * @brief Buzzer'i durdurur
 */
void buzzerStop(void)
{
    buzzer_ctrl.state = BUZZER_IDLE;
    setBuzzerHardware(0, 0);  // Sessizlik
}

/**
 * @brief Buzzer çaliyor mu kontrol eder
 * @return 1 = çaliyor, 0 = durgun
 */
uint8_t buzzerIsPlaying(void)
{
    return (buzzer_ctrl.state != BUZZER_IDLE);
}

// ========== Iç Fonksiyonlar ==========

/**
 * @brief Hardware buzzer kontrolü
 */
static void setBuzzerHardware(uint16_t frequency, uint8_t volume)
{
    if (frequency == 0 || volume == 0) {
        BPWM_ConfigOutputChannel(BPWM0, 3, 0, 0);
        return;
    }
    
    BPWM_ConfigOutputChannel(BPWM0, 3, frequency, volume);
}



