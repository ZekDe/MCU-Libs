/**
 * @file    buzzer_nonblocking.c
 * @brief   Devreli (aktif) buzzer non-blocking pattern calici (bkz. .h).
 */
#include "active_buzzer_nonblocking.h"
#include "M254SE3AE.h"        /* PA3 */
#include "device_context.h"   /* BUZZER(x) makrosu (PA3 = x) */
#include "systemtick.h"       /* systick */

#include <stddef.h>

typedef enum { BUZZER_IDLE = 0, BUZZER_PLAYING } buzzer_state_t;

static struct {
    buzzer_state_t       state;
    const buzzer_note_t *pattern;
    uint8_t              length;
    uint8_t              idx;
    uint8_t              repeat;     /* 0 = sonsuz */
    uint8_t              rep_done;
    uint32_t             note_start;
} bz;

static void applyNote(uint8_t on)
{
    BUZZER(on ? 1 : 0);
}

void buzzerInit(void)
{
    bz.state = BUZZER_IDLE;
    BUZZER(0);
}

void buzzerPlayPattern(const buzzer_note_t *pattern, uint8_t length, uint8_t repeat)
{
    if (pattern == NULL || length == 0) return;

    bz.pattern    = pattern;
    bz.length     = length;
    bz.idx        = 0;
    bz.repeat     = repeat;
    bz.rep_done   = 0;
    bz.state      = BUZZER_PLAYING;
    bz.note_start = systick;
    applyNote(pattern[0].on);
}

void buzzerUpdate(uint32_t now)
{
    if (bz.state != BUZZER_PLAYING) return;

    if ((now - bz.note_start) < bz.pattern[bz.idx].duration) return;   /* note suresi dolmadi */

    /* sonraki note */
    bz.idx++;
    if (bz.idx >= bz.length)
    {
        bz.idx = 0;
        bz.rep_done++;
        if (bz.repeat != 0 && bz.rep_done >= bz.repeat)
        {
            buzzerStop();
            return;
        }
    }

    applyNote(bz.pattern[bz.idx].on);
    bz.note_start = now;
}

void buzzerStop(void)
{
    bz.state = BUZZER_IDLE;
    BUZZER(0);
}

uint8_t buzzerIsPlaying(void)
{
    return (bz.state != BUZZER_IDLE);
}
