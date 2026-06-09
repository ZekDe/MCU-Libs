/**
 * @file    ntc_manager.h
 * @brief   MCU-bagimsiz NTC termistor sicaklik okuma modulu.
 *
 *  Topoloji: VCC --[Rseries]-- ADCpin --[NTC]-- GND
 *  (NTC alt tarafta; sicaklik artinca ADC raw degeri DUSER.)
 *
 *  Kullanim:
 *   - ADC pin / kanal init'ini siz yapin (CubeMX, ESP-IDF adc_oneshot, Nuvoton BSP...).
 *   - `ntc_config_t.adc_read` alanina tek-ornek okuma fonksiyonunuzu yazin.
 *   - Birden fazla NTC: her biri icin ayri `ntc_t` ve farkli adc_read fonksiyonu.
 */

#ifndef NTC_MANAGER_H
#define NTC_MANAGER_H

#include <stdint.h>

/**
 * @brief Tek bir ham ADC ornegi okur.
 * @param raw_out cikis (0..adc_max)
 * @return 0 ok, !=0 hata
 */
typedef int (*ntc_adc_read_fn)(int *raw_out);

typedef struct {
    /* --- Donanim koprusu --- */
    ntc_adc_read_fn adc_read;       /**< zorunlu, kullanici doldurur        */
    int             adc_max;        /**< 12-bit:4095, 10-bit:1023, ...      */

    /* --- NTC fiziksel parametreler --- */
    float           r_series_ohm;   /**< seri direnc (ohm)                  */
    float           r0_ohm;         /**< NTC nominal direnc @ T0            */
    float           beta;           /**< Beta sabiti                        */
    float           t0_kelvin;      /**< referans sicaklik (K), or. 298.15  */
} ntc_config_t;

typedef struct {
    ntc_config_t cfg;
    float        tc_filt;
    uint8_t      ready;
} ntc_t;

/**
 * @brief NTC instance'ini hazirla. ADC donanim init'i kullanicinin sorumlulugunda.
 * @return 0 ok, <0 hata
 */
int8_t ntcInit(ntc_t *ntc, const ntc_config_t *cfg);

/**
 * @brief Sicakligi Celsius olarak oku (trim-mean + IIR low-pass).
 *        Acik/kisa devre tespitinde NAN doner.
 */
float  ntcReadTempC(ntc_t *ntc);

/**
 * @brief Filtrelenmis ham ADC degerini doner (debug icin).
 * @return 0 ok, <0 hata
 */
int8_t ntcReadRaw(ntc_t *ntc, int *raw_out);

#endif /* NTC_MANAGER_H */
