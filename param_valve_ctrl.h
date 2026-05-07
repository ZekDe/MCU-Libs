/**
 * @file    param_valve_ctrl.h
 * @brief   Parametric valve controller with auto-calibration
 *
 * Sıcaklık-step LUT tabanlı vana kontrolü.
 *   - Auto-kalibrasyon ile LUT otomatik oluşturulur
 *   - LUT feedforward: setpoint veya akış değişiminde hızlı yaklaşma
 *   - Kademeli feedback: gerçek sıcaklığa göre step düzeltme
 *   - Rapid recovery: büyük sapmalarda LUT'tan hızlı geri dönüş
 *   - Akış sensörü entegrasyonu (dijital 1/0)
 *   - Acil durum koruması (aşırı sıcaklık)
 
Akış zinciri:

Sıcaklık 8°C'dan fazla saparsa → rapid mode: agresif step uygula (rapid_step_size)
rapid_settle_time boyunca bekle (sıcaklığın düzelmesi için fırsat)
Süre bitince → "etki gerçekten oldu mu?" sorusu
İşte burada PVC_RAPID_EFFECT_DELTA devreye giriyor:

Sıcaklık fazlaydı (error_dir > 0) → vana kapanmış olmalı → sıcaklığın en az 1.5°C düşmesi beklenir → düştüyse "effective"
Sıcaklık azdı (error_dir < 0) → vana açılmış olmalı → sıcaklığın en az 1.5°C artması beklenir → arttıysa "effective"

Notlar:
ctrl->current_target_step = clampStep(ctrl, lut_step); bu satır LUT'u devreye almak için
ihtiyaç olmadığı için yorum satırı yapıldı. 

SETTLE_TIME içeren #define tanımlamaları
Hazne sıcaklığının yeni step'te oturması için bekleme (saniye) — 
motor hareket süresi değil 

*/

#ifndef PARAM_VALVE_CTRL_H
#define PARAM_VALVE_CTRL_H

#include <stdint.h>

/* ========== CONFIGURATION ========== */

#define PVC_LUT_MAX_SIZE         40   /* Maksimum LUT girişi */

/* Kalibrasyon varsayılanları */
#define PVC_CALIB_STEP_START     0     /* Kapalı taraf (soğuk) */
#define PVC_CALIB_STEP_END       40    /* Açık taraf (sıcak)  */
#define PVC_CALIB_STEP_INC       1     /* Her adımda kaç step */
#define PVC_CALIB_SETTLE_TIME    15    /* Stabilizasyon bekleme süresi (saniye) */
#define PVC_CALIB_STABLE_DELTA   1.5f  /* Stabil sayılma eşiği (°C) */

/* Feedback varsayılanları */
#define PVC_FB_DEADBAND          2.0f  /* Düzeltme yapılmayacak bant (°C) */
#define PVC_FB_SETTLE_TIME       5     /* Düzeltme sonrası bekleme süresi (saniye) */
#define PVC_FB_STEP_SIZE         1     /* Her düzeltmede kaç step kayma */
#define PVC_FB_SETTLE_TIME_2     10    /* Orta sapma bekleme süresi (saniye) */
#define PVC_FB_SETTLE_THRESH_2   4.0f  /* Orta sapma eşiği (°C) */

/* İlk atlama bekleme süresi */
#define PVC_JUMP_SETTLE_TIME     10    /* LUT atlaması sonrası ilk bekleme (saniye) */

/* Rapid recovery varsayılanları */
#define PVC_RAPID_THRESHOLD      8.0f  /* Agresif düzeltme sapma eşiği (°C) */
#define PVC_RAPID_SETTLE_TIME    5
#define PVC_RAPID_STEP_SIZE      2     /* Büyük sapmada step düzeltme adımı */
#define PVC_RAPID_EFFECT_DELTA   1.5f  /* Rapid düzeltme etki algılama eşiği (°C) -> 
yani step sonrası sıcaklık değişti mi, etki oldu mu, olduysa bekle,olmadıysa tekrar step ver  */
/* Akış sensörü */
#define PVC_NO_FLOW_STEP         0     /* Akış yokken vana pozisyonu (kapalı) */

/* Acil durum varsayılanları */
#define PVC_EMERGENCY_TEMP       59.5f /* Acil durum sıcaklık eşiği (°C) */
#define PVC_EMERGENCY_STEP       0     /* Acil durum pozisyonu (tam kapalı) */
#define PVC_EMERGENCY_HYST       3.0f  /* Acil durumdan çıkış histerezisi (°C) */

/* ========== DATA STRUCTURES ========== */

/**
 * @brief   Kontrol durumu
 */
typedef enum
{
   PVC_STATE_IDLE = 0,           /* Başlangıç, LUT bekleniyor */
   PVC_STATE_NO_FLOW,            /* Akış yok, vana kapalı bekliyor */
   PVC_STATE_CALIB_MOVE,         /* Kalibrasyon: yeni pozisyona git */
   PVC_STATE_CALIB_SETTLE,       /* Kalibrasyon: stabilizasyon bekle */
   PVC_STATE_CALIB_DONE,         /* Kalibrasyon tamamlandı */
   PVC_STATE_RUNNING_JUMP,       /* LUT'tan hızlı atlama yap */
   PVC_STATE_RUNNING_SETTLE,     /* Atlama sonrası stabilizasyon bekle */
   PVC_STATE_RUNNING_HOLD,       /* Deadband içinde, yerinde kal */
   PVC_STATE_EMERGENCY           /* Acil durum, güvenli pozisyonda */
} pvc_state_e;

/**
 * @brief   Kalibrasyon sonucu: sıcaklık-step çifti
 */
typedef struct
{
   int32_t  step;
   float    temp;
} pvc_lut_entry_t;

/**
 * @brief   Kontrolör ana yapısı
 */
typedef struct
{
   /* LUT */
   pvc_lut_entry_t  lut[PVC_LUT_MAX_SIZE];
   uint8_t          lut_count;
   uint8_t          lut_valid;

   /* Kalibrasyon parametreleri */
   int32_t          calib_step_start;
   int32_t          calib_step_end;
   int32_t          calib_step_inc;
   uint16_t         calib_settle_time;
   float            calib_stable_delta;

   /* Kalibrasyon çalışma durumu */
   int32_t          calib_current_step;
   uint16_t         calib_timer;
   float            calib_temp_start;

   /* Feedback parametreleri */
   float            fb_deadband;
   uint16_t         fb_settle_time;
   int32_t          fb_step_size;
   uint16_t         fb_settle_time_2;   /* Orta sapma bekleme süresi (sn) */
   float            fb_settle_thresh_2; /* Orta sapma eşiği (°C) */

   /* Feedback çalışma durumu */
   uint16_t         fb_timer;
   float            prev_setpoint;
   uint16_t         jump_settle_time;   /* İlk LUT atlaması bekleme süresi (sn) */
   uint8_t          jump_settle_active; /* 1: ilk atlama beklemesi aktif */

   /* Rapid recovery parametreleri */
   float            rapid_threshold;    /* Agresif düzeltme eşiği (°C) */
   int32_t          rapid_step_size;    /* Büyük sapmada step adımı */
   uint16_t         rapid_settle_time;  /* Rapid düzeltme sonrası bekleme (sn) */
   uint8_t          rapid_active;       /* 1: rapid düzeltme yapıldı, uzun bekle */
   float            rapid_temp_before;  /* Rapid düzeltme öncesi sıcaklık */

   /* Akış sensörü */
   uint8_t          flow_active;        /* Güncel akış durumu (1/0) */
   uint8_t          flow_prev;          /* Önceki akış durumu (edge tespiti) */
   int32_t          no_flow_step;       /* Akış yokken pozisyon */

   /* Acil durum parametreleri */
   float            emergency_temp;
   int32_t          emergency_step;
   float            emergency_hyst;

   /* Çalışma durumu */
   pvc_state_e      state;
   int32_t          current_target_step;
   float            setpoint;

   /* Step limitleri */
   int32_t          step_min;           /* Minimum step (kapalı taraf) */
   int32_t          step_max;           /* Maksimum step (açık taraf) */

} pvc_ctrl_t;


/* ========== FUNCTION PROTOTYPES ========== */

/**
 * @brief   Kontrolörü varsayılan parametrelerle başlat
 * @param   ctrl  Kontrolör yapısı pointer
 * @return  0: başarılı, -1: hata
 */
int8_t pvcInit(pvc_ctrl_t *ctrl);

/**
 * @brief   Auto-kalibrasyonu başlat
 * @param   ctrl  Kontrolör yapısı pointer
 * @return  0: başarılı, -1: hata
 */
int8_t pvcStartCalibration(pvc_ctrl_t *ctrl);

/**
 * @brief   Her saniye çağrılacak ana güncelleme fonksiyonu
 * @param   ctrl         Kontrolör yapısı pointer
 * @param   temperature  Güncel sıcaklık (°C)
 * @param   setpoint     Hedef sıcaklık (°C)
 * @param   flow         Akış sensörü durumu (1: akış var, 0: yok)
 * @param   out_step     Çıkış: hedef step pozisyonu
 * @return  0: normal, 1: kalibrasyon devam, 2: akış yok, -1: hata
 */
int8_t pvcUpdate(pvc_ctrl_t *ctrl, float temperature, float setpoint,
                 uint8_t flow, int32_t *out_step);

/**
 * @brief   LUT'tan setpoint'e en uygun step'i bul (lineer interpolasyon)
 * @param   ctrl         Kontrolör yapısı pointer
 * @param   target_temp  Hedef sıcaklık (°C)
 * @param   out_step     Çıkış: hesaplanan step
 * @return  0: başarılı, -1: LUT geçersiz
 */
int8_t pvcLookupStep(pvc_ctrl_t *ctrl, float target_temp, int32_t *out_step);

/**
 * @brief   Manuel LUT girişi ekle
 * @param   ctrl  Kontrolör yapısı pointer
 * @param   step  Step pozisyonu
 * @param   temp  O pozisyondaki sıcaklık
 * @return  0: başarılı, -1: tablo dolu, -2: hata
 */
int8_t pvcAddLutEntry(pvc_ctrl_t *ctrl, int32_t step, float temp);

/**
 * @brief   Mevcut durumu sorgula
 * @param   ctrl  Kontrolör yapısı pointer
 * @return  Durum enum değeri
 */
pvc_state_e pvcGetState(pvc_ctrl_t *ctrl);

/**
 * @brief   Kalibrasyon ilerlemesini al
 * @param   ctrl  Kontrolör yapısı pointer
 * @return  İlerleme yüzdesi (0-100)
 */
uint8_t pvcGetCalibProgress(pvc_ctrl_t *ctrl);

/**
 * @brief   LUT tablosunu sıfırla
 * @param   ctrl  Kontrolör yapısı pointer
 */
void pvcClearLut(pvc_ctrl_t *ctrl);

#endif /* PARAM_VALVE_CTRL_H */
