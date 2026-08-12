/**
 * @file    adc_manager.c
 * @brief   MCU-bagimsiz genel amacli ADC modulu.
 *          Ham ADC okumayi cagiran kullanicinin verdigi callback yapar;
 *          trimmed-mean + IIR filtre ile mV uretir.
 */

#include "adc_manager.h"

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

int8_t adcInit(adc_t *adc, const adc_config_t *cfg)
{
    if (adc == NULL || cfg == NULL)        return -1;
    if (cfg->adc_read == NULL)             return -2;
    if (cfg->adc_max == 0)                 return -3;
    if (cfg->vref_mv       <= 0.0f ||
        cfg->divider_ratio <= 0.0f ||
        cfg->iir_alpha     <  0.0f ||
        cfg->iir_alpha     >  1.0f)        return -4;
    if (cfg->sample_count == 0 ||
        cfg->sample_count > ADC_SAMPLE_MAX ||
        (uint32_t)cfg->trim_count * 2u >= cfg->sample_count) return -5;

    memcpy(&adc->cfg, cfg, sizeof(adc_config_t));
    adc->filt_init = 0;
    adc->ready     = 1;
    return 0;
}

int8_t adcReadRaw(adc_t *adc, uint16_t *raw_out)
{
    if (adc == NULL || !adc->ready || raw_out == NULL) return -1;

    uint16_t samples[ADC_SAMPLE_MAX];
    const int32_t n = (int32_t)adc->cfg.sample_count;
    for (int32_t i = 0; i < n; ++i)
    {
        if (adc->cfg.adc_read(&samples[i]) != 0) return -2;
    }
    *raw_out = trimmedMean(samples, n, (int32_t)adc->cfg.trim_count);
    return 0;
}

float adcReadMv(adc_t *adc)
{
    uint16_t raw;
    if (adcReadRaw(adc, &raw) != 0) return ADC_MV_ERROR;

    float mv = (float)raw * adc->cfg.vref_mv / (float)adc->cfg.adc_max
             * adc->cfg.divider_ratio;

    if (!adc->filt_init)
    {
        adc->mv_filt   = mv;
        adc->filt_init = 1;
    }
    else
        adc->mv_filt = adc->cfg.iir_alpha * mv
                     + (1.0f - adc->cfg.iir_alpha) * adc->mv_filt;

    return adc->mv_filt;
}

int8_t adcSetVref(adc_t *adc, float vref_mv)
{
    if (adc == NULL || !adc->ready) return -1;
    if (vref_mv <= 0.0f)            return -2;

    adc->cfg.vref_mv = vref_mv;
    return 0;
}
