/**
 * @file    hysteresis.c
 * @brief   Iki esikli histerezis karsilastirici (bkz. hysteresis.h).
 */

#include "hysteresis.h"
#include <stddef.h>


int8_t hysteresisInit(hysteresis_t *h, float th_low, float th_high,
                      hyst_state_t initial, uint16_t confirm_count_low, uint16_t confirm_count_high)
{
    if (th_low >= th_high)     return -2;   /* histerezis bandi gecersiz */
    h->th_low        = th_low;
    h->th_high       = th_high;
    h->state         = initial;
    h->confirm_count_high = (confirm_count_high == 0u) ? 1u : confirm_count_high;
	h->confirm_count_low = (confirm_count_low == 0u) ? 1u : confirm_count_low;
    h->counter       = 0u;
    return 0;
}

hyst_state_t hysteresisUpdate(hysteresis_t *h, float value, uint32_t now, uint32_t preset_time)
{
	/* preset_time > 0: sayaci en fazla preset_time'da bir artir (hizli dongude
	   aninda dolmasin). preset_time == 0: TON devre disi, HER cagride isle. */
	if (preset_time != 0u)
	{
		if (!TON(&h->ton_obj, 1, now, preset_time))
			return h->state;   /* pencere dolmadi -> durumu koru */
		h->ton_obj.aux = 0;    /* doldu -> isle ve pencereyi yeniden baslat */
	}

    if (h->state == HYST_LOW)
    {
        /* HIGH'a gecis: value >= th_high, confirm_count kez ARDARDA */
        if (value >= h->th_high)
        {
            if (++h->counter >= h->confirm_count_high)
            {
                h->state   = HYST_HIGH;
                h->counter = 0u;
            }
        }
        else
        {
            h->counter = 0u;   /* ardardalik bozuldu -> sayaci sifirla */
        }
    }
    else /* HYST_HIGH */
    {
        /* LOW'a gecis: value <= th_low, confirm_count kez ARDARDA */
        if (value <= h->th_low)
        {
            if (++h->counter >= h->confirm_count_low)
            {
                h->state   = HYST_LOW;
                h->counter = 0u;
            }
        }
        else
        {
            h->counter = 0u;
        }
    }
    return h->state;
}
