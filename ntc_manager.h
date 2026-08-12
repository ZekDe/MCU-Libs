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

/** Gecersiz okuma / acik-kisa devre durumunda donen sentinel sicaklik. */
#define NTC_TEMP_ERROR  (-1000.0f)

/** sample_count icin derleme-zamani ust sinir (ornek tamponu boyutu). */
#define NTC_SAMPLE_MAX  64

/**
 * @brief Tek bir ham ADC ornegi okur.
 * @param raw_out cikis (0..adc_max), isaretsiz 16-bit
 * @return 0 ok, !=0 hata
 */
typedef int8_t (*ntc_adc_read_fn)(uint16_t *raw_out);

typedef struct {
    /* --- Donanim koprusu --- */
    ntc_adc_read_fn adc_read;       /**< zorunlu, kullanici doldurur        */
    uint16_t        adc_max;        /**< 12-bit:4095, 10-bit:1023, ...      */

    /* --- Ornekleme --- */
    uint16_t        sample_count;   /**< toplam ornek (1..NTC_SAMPLE_MAX)   */
    uint16_t        trim_count;     /**< her uctan atilan; 2*trim < sample  */

    /* --- NTC fiziksel parametreler --- */
    float           r_series_ohm;   /**< seri direnc (ohm)                  */
    float           r0_ohm;         /**< NTC nominal direnc @ T0            */
    float           beta;           /**< Beta sabiti                        */
    float           t0_kelvin;      /**< referans sicaklik (K), or. 298.15  */

    /* --- Filtre --- */
    float           iir_alpha;      /**< 0..1; kucuk deger = daha yumusak filtre */
} ntc_config_t;

typedef struct {
    ntc_config_t cfg;
    float        tc_filt;
    uint8_t      filt_init;   /**< 0: tc_filt henuz set edilmedi, 1: set edildi */
    uint8_t      ready;
} ntc_t;

/**
 * @brief NTC instance'ini hazirla. ADC donanim init'i kullanicinin sorumlulugunda.
 * @return 0 ok, <0 hata
 */
int8_t ntcInit(ntc_t *ntc, const ntc_config_t *cfg);

/**
 * @brief Sicakligi Celsius olarak oku (trim-mean + IIR low-pass).
 *        Acik/kisa devre tespitinde NTC_TEMP_ERROR doner.
 */
float  ntcReadTempC(ntc_t *ntc);

/**
 * @brief Filtrelenmis ham ADC degerini doner (debug icin).
 * @return 0 ok, <0 hata
 */
int8_t ntcReadRaw(ntc_t *ntc, uint16_t *raw_out);

#endif /* NTC_MANAGER_H */
