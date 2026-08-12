/**
 * @file    hysteresis.h
 * @brief   Iki esikli (Schmitt-trigger) histerezis karsilastirici.
 *
 *  Tek esik etrafinda gurultu/titremeye karsi saglam ikili karar uretir.
 *  Genel amaclidir; anlam cagirana baittir:
 *    - Isik:  HYST_HIGH = aydinlik, HYST_LOW = karanlik
 *    - Pil:   HYST_HIGH = normal,   HYST_LOW = dusuk pil
 *
 *  Gecis kurali (th_low < th_high):
 *    LOW  iken value >= th_high  (confirm_count kez ardarda) -> HIGH
 *    HIGH iken value <= th_low   (confirm_count kez ardarda) -> LOW
 *    arada (th_low..th_high)     -> durum korunur (histerezis bandi)
 *    ardardalik bozulursa        -> onay sayaci sifirlanir
 *
 *  Not: girdi dogrudan kullanilir; gecersiz/hata degerlerini (or. ADC
 *  hatasi) cagiran taraf filtrelemeli.
 */

#ifndef HYSTERESIS_H
#define HYSTERESIS_H

#include <stdint.h>
#include "ton.h"

typedef enum {
    HYST_LOW  = 0,
    HYST_HIGH = 1
} hyst_state_t;

typedef struct {
    float        th_low;
    float        th_high;
    hyst_state_t state;
    uint16_t     confirm_count_high;   /* gecis icin gereken ardarda esik-asma sayisi */
	uint16_t     confirm_count_low; 
    uint16_t     counter;         /* ayni yonde ardarda gozlem sayaci (runtime) */
	ton_t ton_obj;
} hysteresis_t;

/**
 * @brief Karsilastiriciyi hazirla. th_low < th_high olmali.
 * @param initial       baslangic durumu
 * @param confirm_count_high/low durum degistirmek icin degerin esigin otesinde kac kez
 *                      ARDARDA gozlenmesi gerektigi. Arada tek bir ters ornek
 *                      sayaci sifirlar (ani/gecici sicramalar filtrelenir).
 *                      0 verilirse 1 kabul edilir (aninda gecis).
 * @return 0 ok, <0 hata
 */
int8_t hysteresisInit(hysteresis_t *h, float th_low, float th_high,
                      hyst_state_t initial, uint16_t confirm_count_low, uint16_t confirm_count_high);

/**
 * @brief Yeni deger ile durumu guncelle.
 * @param now         sistem tick (ms). preset_time==0 ise kullanilmaz.
 * @param preset_time sayaci en fazla bu periyotta (ms) bir artir; hizli dongude
 *                    sayacin aninda dolmasini onler. 0 verilirse TON devre disi
 *                    kalir ve HER cagride islenir (or. 1 Hz dongu icin (now=0,
 *                    preset_time=0) kullan).
 * @return guncel kararli durum (HYST_LOW / HYST_HIGH)
 */
hyst_state_t hysteresisUpdate(hysteresis_t *h, float value, uint32_t now, uint32_t preset_time);

#endif /* HYSTERESIS_H */
