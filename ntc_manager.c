/**
 * @file    ntc_manager.c
 * @brief   MCU-bagimsiz NTC sicaklik modulu.
 *          Hicbir vendor header'ina bagimli degildir; ADC okumayi cagiran
 *          kullanicinin verdigi callback yapar.
 */

#include "ntc_manager.h"

#include <math.h>
#include <string.h>

static uint16_t trimmedMean(uint16_t *v, int32_t n, int32_t trim)
{
    for (int32_t i = 0; i < n - 1; ++i)
    {
        for (int32_t j = 0; j < n - 1 - i; ++j)
        {
            if (v[j] > v[j + 1])
            {
                uint16_t t = v[j]; v[j] = v[j + 1]; v[j + 1] = t;
            }
        }
    }
    uint32_t sum = 0;
    int32_t cnt = 0;
    for (int32_t i = trim; i < n - trim; ++i)
    {
        sum += v[i];
        ++cnt;
    }
    return (uint16_t)(sum / (uint32_t)cnt);
}

int8_t ntcInit(ntc_t *ntc, const ntc_config_t *cfg)
{
    if (ntc == NULL || cfg == NULL)        return -1;
    if (cfg->adc_read == NULL)             return -2;
    if (cfg->adc_max == 0)                 return -3;
    if (cfg->r_series_ohm <= 0.0f ||
        cfg->r0_ohm       <= 0.0f ||
        cfg->beta         <= 0.0f ||
        cfg->t0_kelvin    <= 0.0f)         return -4;
    if (cfg->iir_alpha < 0.0f ||
        cfg->iir_alpha > 1.0f)             return -5;
    if (cfg->sample_count == 0 ||
        cfg->sample_count > NTC_SAMPLE_MAX ||
        (uint32_t)cfg->trim_count * 2u >= cfg->sample_count) return -6;

    memcpy(&ntc->cfg, cfg, sizeof(ntc_config_t));
    ntc->filt_init = 0;
    ntc->ready     = 1;
    return 0;
}

int8_t ntcReadRaw(ntc_t *ntc, uint16_t *raw_out)
{
    if (ntc == NULL || !ntc->ready || raw_out == NULL) return -1;

    uint16_t samples[NTC_SAMPLE_MAX];
    const int32_t n = (int32_t)ntc->cfg.sample_count;
    for (int32_t i = 0; i < n; ++i)
    {
        if (ntc->cfg.adc_read(&samples[i]) != 0) return -2;
    }
    *raw_out = trimmedMean(samples, n, (int32_t)ntc->cfg.trim_count);
    return 0;
}

float ntcReadTempC(ntc_t *ntc)
{
    uint16_t raw;
    if (ntcReadRaw(ntc, &raw) != 0) return NTC_TEMP_ERROR;

    const uint16_t margin = ntc->cfg.adc_max / 1000;     /* %0.1 */
    if (raw < margin || raw > (ntc->cfg.adc_max - margin))
    {
        /* NTC kopuk veya kisa devre */
        ntc->filt_init = 0;
        return NTC_TEMP_ERROR;
    }

    const float adc_max_f = (float)ntc->cfg.adc_max;
    float rntc = ntc->cfg.r_series_ohm * (float)raw / (adc_max_f - (float)raw);

    float invT = (1.0f / ntc->cfg.t0_kelvin)
               + (1.0f / ntc->cfg.beta) * logf(rntc / ntc->cfg.r0_ohm);
    float tc = (1.0f / invT) - 273.15f;

    if (!ntc->filt_init)
    {
        ntc->tc_filt   = tc;
        ntc->filt_init = 1;
    }
    else
        ntc->tc_filt = ntc->cfg.iir_alpha * tc
                     + (1.0f - ntc->cfg.iir_alpha) * ntc->tc_filt;

    return ntc->tc_filt;
}
