/**
 * @file    ntc_manager.c
 * @brief   MCU-bagimsiz NTC sicaklik modulu.
 *          Hicbir vendor header'ina bagimli degildir; ADC okumayi cagiran
 *          kullanicinin verdigi callback yapar.
 */

#include "ntc_manager.h"

#include <math.h>
#include <string.h>

#define NTC_SAMPLE_COUNT   32
#define NTC_TRIM_COUNT     4
#define NTC_IIR_ALPHA      0.20f

static int trimmedMean(int *v, int n, int trim)
{
    for (int i = 0; i < n - 1; ++i)
    {
        for (int j = 0; j < n - 1 - i; ++j)
        {
            if (v[j] > v[j + 1])
            {
                int t = v[j]; v[j] = v[j + 1]; v[j + 1] = t;
            }
        }
    }
    long sum = 0;
    int cnt = 0;
    for (int i = trim; i < n - trim; ++i)
    {
        sum += v[i];
        ++cnt;
    }
    return (int)(sum / cnt);
}

int8_t ntcInit(ntc_t *ntc, const ntc_config_t *cfg)
{
    if (ntc == NULL || cfg == NULL)        return -1;
    if (cfg->adc_read == NULL)             return -2;
    if (cfg->adc_max <= 0)                 return -3;
    if (cfg->r_series_ohm <= 0.0f ||
        cfg->r0_ohm       <= 0.0f ||
        cfg->beta         <= 0.0f ||
        cfg->t0_kelvin    <= 0.0f)         return -4;

    memcpy(&ntc->cfg, cfg, sizeof(ntc_config_t));
    ntc->tc_filt = NAN;
    ntc->ready   = 1;
    return 0;
}

int8_t ntcReadRaw(ntc_t *ntc, int *raw_out)
{
    if (ntc == NULL || !ntc->ready || raw_out == NULL) return -1;

    int samples[NTC_SAMPLE_COUNT];
    for (int i = 0; i < NTC_SAMPLE_COUNT; ++i)
    {
        if (ntc->cfg.adc_read(&samples[i]) != 0) return -2;
    }
    *raw_out = trimmedMean(samples, NTC_SAMPLE_COUNT, NTC_TRIM_COUNT);
    return 0;
}

float ntcReadTempC(ntc_t *ntc)
{
    int raw;
    if (ntcReadRaw(ntc, &raw) != 0) return NAN;

    const int margin = ntc->cfg.adc_max / 1000;     /* %0.1 */
    if (raw < margin || raw > (ntc->cfg.adc_max - margin))
    {
        /* NTC kopuk veya kisa devre */
        ntc->tc_filt = NAN;
        return NAN;
    }

    const float adc_max_f = (float)ntc->cfg.adc_max;
    float rntc = ntc->cfg.r_series_ohm * (float)raw / (adc_max_f - (float)raw);

    float invT = (1.0f / ntc->cfg.t0_kelvin)
               + (1.0f / ntc->cfg.beta) * logf(rntc / ntc->cfg.r0_ohm);
    float tc = (1.0f / invT) - 273.15f;

    if (isnan(ntc->tc_filt))
        ntc->tc_filt = tc;
    else
        ntc->tc_filt = NTC_IIR_ALPHA * tc + (1.0f - NTC_IIR_ALPHA) * ntc->tc_filt;

    return ntc->tc_filt;
}
