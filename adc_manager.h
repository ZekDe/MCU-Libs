/**
 * @file    adc_manager.h
 * @brief   MCU-bagimsiz genel amacli ADC kanal okuma modulu.
 *
 *  Ham ADC ornegini kullanicinin verdigi callback ile alir; outlier-temizleme
 *  (trimmed-mean) + IIR alcak-gecirgen filtre uygular ve dogrudan mV doner.
 *  Vendor header'ina bagimli degildir, <math.h> gerektirmez.
 *
 *  Kullanim:
 *   - ADC pin / kanal init'ini siz yapin.
 *   - `adc_config_t.adc_read` alanina tek-ornek okuma fonksiyonunuzu yazin.
 *   - Gerilim bolucu varsa `divider_ratio` ile gercek dugum gerilimini alin
 *     (orn. VBAT). Bolucu yoksa 1.0 verin (orn. LDR).
 *   - Birden fazla kanal: her biri icin ayri `adc_t` ve farkli adc_read fonksiyonu.
 */

#ifndef ADC_MANAGER_H
#define ADC_MANAGER_H

#include <stdint.h>

/** Okuma hatasi durumunda donen sentinel (gerilim negatif olamaz). */
#define ADC_MV_ERROR  (-1.0f)

/** sample_count icin derleme-zamani ust sinir (ornek tamponu boyutu). */
#define ADC_SAMPLE_MAX  64

/**
 * @brief Tek bir ham ADC ornegi okur.
 * @param raw_out cikis (0..adc_max), isaretsiz 16-bit
 * @return 0 ok, !=0 hata
 */
typedef int8_t (*adc_read_fn)(uint16_t *raw_out);

typedef struct {
    /* --- Donanim koprusu --- */
    adc_read_fn adc_read;       /**< zorunlu, kullanici doldurur            */
    uint16_t    adc_max;        /**< 12-bit:4095, 10-bit:1023, ...          */

    /* --- Ornekleme --- */
    uint16_t    sample_count;   /**< toplam ornek (1..ADC_SAMPLE_MAX)       */
    uint16_t    trim_count;     /**< her uctan atilan ornek; 2*trim < sample */

    /* --- Olcekleme / filtre --- */
    float       vref_mv;        /**< ADC referans gerilimi (mV), or. 3300.0 */
    float       divider_ratio;  /**< gercek = adc_ucu * ratio; bolucu yoksa 1.0 */
    float       iir_alpha;      /**< 0..1; kucuk deger = daha yumusak filtre */
} adc_config_t;

typedef struct {
    adc_config_t cfg;
    float        mv_filt;
    uint8_t      filt_init;     /**< 0: mv_filt henuz set edilmedi, 1: set edildi */
    uint8_t      ready;
} adc_t;

/**
 * @brief ADC instance'ini hazirla. ADC donanim init'i kullanicinin sorumlulugunda.
 * @return 0 ok, <0 hata
 */
int8_t adcInit(adc_t *adc, const adc_config_t *cfg);

/**
 * @brief Filtrelenmis ham ADC degerini doner (trim-mean, IIR yok; debug icin).
 * @return 0 ok, <0 hata
 */
int8_t adcReadRaw(adc_t *adc, uint16_t *raw_out);

/**
 * @brief Gerilimi mV olarak oku (trim-mean + IIR low-pass + divider_ratio).
 *        Okuma hatasinda ADC_MV_ERROR doner.
 */
float adcReadMv(adc_t *adc);

/**
 * @brief Calisma aninda referans gerilimini (vref_mv) gunceller.
 *        Band-gap'ten hesaplanan canli AVDD'yi beslemek icin kullanilir.
 *        Filtre durumunu (mv_filt) bozmaz.
 * @return 0 ok, <0 hata
 */
int8_t adcSetVref(adc_t *adc, float vref_mv);

#endif /* ADC_MANAGER_H */
