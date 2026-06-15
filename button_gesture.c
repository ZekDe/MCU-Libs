/**
 * @file    button_gesture.c
 * @brief   Single / multi click and long-press detector for a single button.
 * @author  Emrah Duatepe
 */

#include "button_gesture.h"

void buttonGestureInit(button_gesture_t *obj,
                       uint32_t debounce_ms,
                       uint32_t long_press_ms,
                       uint32_t multi_click_window_ms,
                       uint32_t repeat_start_ms,
                       uint32_t repeat_end_ms,
                       uint32_t repeat_ramp_ms)
{
    obj->debounce_ms           = debounce_ms;
    obj->long_press_ms         = long_press_ms;
    obj->multi_click_window_ms = multi_click_window_ms;
    obj->repeat_start_ms       = repeat_start_ms;
    obj->repeat_end_ms         = repeat_end_ms;
    obj->repeat_ramp_ms        = repeat_ramp_ms;
}

button_event_t buttonGestureProcess(button_gesture_t *obj,
                                    uint8_t raw_pressed,
                                    uint32_t now,
                                    uint32_t *click_count_out)
{
    uint8_t stable     = TON(&obj->ton_debounce, raw_pressed, now, obj->debounce_ms);
    uint8_t long_level = TON(&obj->ton_long,     stable,      now, obj->long_press_ms);
    uint8_t rise       = edgeDetection(&obj->ed_rise, stable);
    uint8_t long_edge  = edgeDetection(&obj->ed_long, long_level);

    obj->btn_stable = stable;

    // Tekrar-basis zorunlulugu: buton birakilana kadar olaylari yut.
    // Edge dedektorleri yukarida guncellendi (bayat edge patlamasin diye),
    // burada sadece olay uretimini bastiriyoruz.
    if (obj->ignore_until_release)
    {
        if (!stable)
        {
            obj->ignore_until_release = 0;   // birakildi → tekrar basisa hazir
        }
        else
        {
            return BTN_EVT_NONE;             // hala basili → yut
        }
    }

    if (rise)
    {
        if (!obj->window_active)
        {
            obj->window_active = 1;
            obj->click_count   = 1;
            obj->window_start  = now;
            obj->long_fired    = 0;
        }
        else
        {
            obj->click_count++;
        }
    }

    if (long_edge)
    {
        obj->long_fired      = 1;
        obj->click_count     = 0;
        obj->window_active   = 0;
        obj->last_repeat     = now;
        obj->long_started_at = now;

        if (click_count_out != 0)
        {
            *click_count_out = 0;
        }
        return BTN_EVT_LONG;
    }

    // Long press devam ediyorsa, ivmelendirilmis periyotta BTN_EVT_LONG_REPEAT dondur
    if (obj->long_fired && stable && obj->repeat_start_ms > 0)
    {
        // Su anki periyodu hesapla: start_ms -> end_ms arasinda dogrusal interpolasyon
        uint32_t period;
        uint32_t elapsed = now - obj->long_started_at;

        if (obj->repeat_ramp_ms == 0 || elapsed >= obj->repeat_ramp_ms)
        {
            period = obj->repeat_end_ms;
        }
        else if (obj->repeat_start_ms >= obj->repeat_end_ms)
        {
            uint32_t delta = obj->repeat_start_ms - obj->repeat_end_ms;
            period = obj->repeat_start_ms - (delta * elapsed) / obj->repeat_ramp_ms;
        }
        else
        {
            // start < end (yavaslama) - nadir ama destekle
            uint32_t delta = obj->repeat_end_ms - obj->repeat_start_ms;
            period = obj->repeat_start_ms + (delta * elapsed) / obj->repeat_ramp_ms;
        }

        if ((now - obj->last_repeat) >= period)
        {
            obj->last_repeat = now;
            return BTN_EVT_LONG_REPEAT;
        }
    }

    // Buton birakildiginda long state'ini sifirla
    if (obj->long_fired && !stable)
    {
        obj->long_fired = 0;
    }

    if (obj->window_active
        && ((now - obj->window_start) >= obj->multi_click_window_ms)
        && (stable == 0))
    {
        button_event_t event = BTN_EVT_NONE;
        uint32_t       count = obj->click_count;

        if (!obj->long_fired)
        {
            if (count == 1)
            {
                event = BTN_EVT_SINGLE;
            }
            else if (count >= 2)
            {
                event = BTN_EVT_MULTI;
            }
        }

        obj->window_active = 0;
        obj->click_count   = 0;
        obj->long_fired    = 0;

        if (click_count_out != 0)
        {
            *click_count_out = count;
        }
        return event;
    }

    return BTN_EVT_NONE;
}

void buttonGestureRequireRepress(button_gesture_t *obj)
{
    obj->ignore_until_release = 1;
    obj->window_active        = 0;
    obj->click_count          = 0;
    obj->long_fired           = 0;
}
