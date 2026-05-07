/**
 * @file    param_valve_ctrl.c
 * @brief   Parametric valve controller with auto-calibration
 *
 * Çalışma prensibi:
 */

#include "param_valve_ctrl.h"
#include <string.h>
#include <math.h>

/* ========== PRIVATE FUNCTIONS ========== */

/**
 * @brief   LUT'u sıcaklığa göre artan sırada sırala (insertion sort)
 * @param   ctrl  Kontrolör yapısı pointer
 */
static void sortLutByTemp(pvc_ctrl_t *ctrl)
{
   for (uint8_t i = 1; i < ctrl->lut_count; i++)
   {
      pvc_lut_entry_t key = ctrl->lut[i];
      int8_t j = i - 1;

      while (j >= 0 && ctrl->lut[j].temp > key.temp)
      {
         ctrl->lut[j + 1] = ctrl->lut[j];
         j--;
      }
      ctrl->lut[j + 1] = key;
   }
}

/**
 * @brief   Step değerini limitlere sınırla
 * @param   ctrl  Kontrolör yapısı pointer
 * @param   step  Sınırlanacak step
 * @return  Sınırlanmış step
 */
static int32_t clampStep(pvc_ctrl_t *ctrl, int32_t step)
{
   if (step < ctrl->step_min) return ctrl->step_min;
   if (step > ctrl->step_max) return ctrl->step_max;
   return step;
}

/* ========== PUBLIC FUNCTIONS ========== */

int8_t pvcInit(pvc_ctrl_t *ctrl)
{
   if (ctrl == NULL)
   {
      return -1;
   }

   memset(ctrl, 0, sizeof(pvc_ctrl_t));

   /* Kalibrasyon varsayılanları */
   ctrl->calib_step_start  = PVC_CALIB_STEP_START;
   ctrl->calib_step_end    = PVC_CALIB_STEP_END;
   ctrl->calib_step_inc    = PVC_CALIB_STEP_INC;
   ctrl->calib_settle_time = PVC_CALIB_SETTLE_TIME;
   ctrl->calib_stable_delta = PVC_CALIB_STABLE_DELTA;

   /* Feedback varsayılanları */
   ctrl->fb_deadband    = PVC_FB_DEADBAND;
   ctrl->fb_settle_time = PVC_FB_SETTLE_TIME;
   ctrl->fb_step_size   = PVC_FB_STEP_SIZE;
   ctrl->fb_settle_time_2   = PVC_FB_SETTLE_TIME_2;
   ctrl->fb_settle_thresh_2 = PVC_FB_SETTLE_THRESH_2;

   /* İlk atlama bekleme süresi */
   ctrl->jump_settle_time   = PVC_JUMP_SETTLE_TIME;
   ctrl->jump_settle_active = 0;

   /* Rapid recovery varsayılanları */
   ctrl->rapid_threshold  = PVC_RAPID_THRESHOLD;
   ctrl->rapid_step_size  = PVC_RAPID_STEP_SIZE;
   ctrl->rapid_settle_time = PVC_RAPID_SETTLE_TIME;
   ctrl->rapid_active      = 0;
   ctrl->rapid_temp_before = 0.0f;

   /* Akış sensörü */
   ctrl->flow_active  = 0;
   ctrl->flow_prev    = 0;
   ctrl->no_flow_step = PVC_NO_FLOW_STEP;

   /* Acil durum varsayılanları */
   ctrl->emergency_temp = PVC_EMERGENCY_TEMP;
   ctrl->emergency_step = PVC_EMERGENCY_STEP;
   ctrl->emergency_hyst = PVC_EMERGENCY_HYST;

   /* Step limitleri (kalibrasyon aralığıyla aynı) */
   ctrl->step_min = PVC_CALIB_STEP_START;
   ctrl->step_max = PVC_CALIB_STEP_END;

   ctrl->state = PVC_STATE_IDLE;
   ctrl->lut_valid = 0;
   ctrl->lut_count = 0;
   ctrl->prev_setpoint = 0.0f;

   return 0;
}

int8_t pvcStartCalibration(pvc_ctrl_t *ctrl)
{
   if (ctrl == NULL)
   {
      return -1;
   }

   /* LUT'u temizle */
   ctrl->lut_count = 0;
   ctrl->lut_valid = 0;

   /* Kalibrasyonu başlangıç step'inden başlat */
   ctrl->calib_current_step = ctrl->calib_step_start;
   ctrl->calib_timer = 0;
   ctrl->calib_temp_start = 0.0f;

   ctrl->state = PVC_STATE_CALIB_MOVE;

   return 0;
}

int8_t pvcUpdate(pvc_ctrl_t *ctrl, float temperature, float setpoint,
                 uint8_t flow, int32_t *out_step)
{
   if (ctrl == NULL || out_step == NULL)
   {
      return -1;
   }

   ctrl->setpoint = setpoint;

   /* Akış edge tespiti */
   uint8_t flow_rising  = (flow == 1 && ctrl->flow_prev == 0) ? 1 : 0;
   uint8_t flow_falling = (flow == 0 && ctrl->flow_prev == 1) ? 1 : 0;
   ctrl->flow_prev = flow;
   ctrl->flow_active = flow;

   /* Setpoint değişim tespiti (gerek kalmadı)*/
   //uint8_t sp_changed = (fabsf(setpoint - ctrl->prev_setpoint) > 0.1f) ? 1 : 0;
   ctrl->prev_setpoint = setpoint;

   /* ---- Acil durum kontrolü (kalibrasyon hariç her durumda) ---- */
   if (ctrl->state != PVC_STATE_CALIB_MOVE &&
       ctrl->state != PVC_STATE_CALIB_SETTLE)
   {
      if (temperature >= ctrl->emergency_temp)
      {
         ctrl->state = PVC_STATE_EMERGENCY;
         ctrl->current_target_step = ctrl->emergency_step;
         *out_step = ctrl->emergency_step;
         return 0;
      }

      if (ctrl->state == PVC_STATE_EMERGENCY)
      {
         if (temperature < (ctrl->emergency_temp - ctrl->emergency_hyst))
         {
            /* Acil durumdan çıkış → LUT'tan yeniden atlama */
            ctrl->state = PVC_STATE_RUNNING_JUMP;
         }
         else
         {
            *out_step = ctrl->emergency_step;
            return 0;
         }
      }
   }

   /* ---- Akış durdu → kapalı pozisyona git ---- */
   if (flow_falling)
   {
      if (ctrl->state != PVC_STATE_CALIB_MOVE &&
          ctrl->state != PVC_STATE_CALIB_SETTLE)
      {
         ctrl->state = PVC_STATE_NO_FLOW;
         ctrl->current_target_step = ctrl->no_flow_step;
         *out_step = ctrl->no_flow_step;
         return 2;
      }
   }

   /* ---- Akış yok durumu ---- */
   if (flow == 0 && ctrl->state != PVC_STATE_CALIB_MOVE &&
       ctrl->state != PVC_STATE_CALIB_SETTLE &&
       ctrl->state != PVC_STATE_IDLE)
   {
      ctrl->state = PVC_STATE_NO_FLOW;
      ctrl->current_target_step = ctrl->no_flow_step;
      *out_step = ctrl->no_flow_step;
      return 2;
   }

   /* ---- Akış başladı → LUT'tan atlama ---- */
   if ((flow_rising) &&
       ctrl->lut_valid &&
       ctrl->state != PVC_STATE_CALIB_MOVE &&
       ctrl->state != PVC_STATE_CALIB_SETTLE)
   {
      ctrl->state = PVC_STATE_RUNNING_JUMP;
   }

   /* ---- Durum makinesi ---- */
   switch (ctrl->state)
   {
      case PVC_STATE_IDLE:
      case PVC_STATE_NO_FLOW:
         *out_step = ctrl->current_target_step;
         return (ctrl->state == PVC_STATE_NO_FLOW) ? 2 : 0;

      /* =================== KALİBRASYON =================== */

      case PVC_STATE_CALIB_MOVE:
         ctrl->current_target_step = ctrl->calib_current_step;
         *out_step = ctrl->calib_current_step;

         ctrl->calib_timer = 0;
         ctrl->calib_temp_start = temperature;
         ctrl->state = PVC_STATE_CALIB_SETTLE;
         return 1;

      case PVC_STATE_CALIB_SETTLE:
         *out_step = ctrl->calib_current_step;
         ctrl->calib_timer++;

         if (ctrl->calib_timer >= ctrl->calib_settle_time)
         {
            float temp_change = fabsf(temperature - ctrl->calib_temp_start);

            /* Sıcaklığı kaydet */
            if (ctrl->lut_count < PVC_LUT_MAX_SIZE)
            {
               ctrl->lut[ctrl->lut_count].step = ctrl->calib_current_step;
               ctrl->lut[ctrl->lut_count].temp = temperature;
               ctrl->lut_count++;
            }

            /* Sonraki step */
            ctrl->calib_current_step += ctrl->calib_step_inc;

            if (ctrl->calib_current_step > ctrl->calib_step_end)
            {
               /* Kalibrasyon tamamlandı */
               sortLutByTemp(ctrl);
               ctrl->lut_valid = 1;
               ctrl->state = PVC_STATE_CALIB_DONE;
               return 0;
            }
            else
            {
               ctrl->state = PVC_STATE_CALIB_MOVE;
               return 1;
            }
         }
         return 1;

      case PVC_STATE_CALIB_DONE:
         ctrl->state = PVC_STATE_RUNNING_JUMP;
         /* Fall through */

      /* =================== NORMAL ÇALIŞMA =================== */

      case PVC_STATE_RUNNING_JUMP:
      {
         if (!ctrl->lut_valid || ctrl->lut_count < 2)
         {
            *out_step = ctrl->current_target_step;
            return -1;
         }

         /* LUT'tan feedforward atlama */
         int32_t lut_step = 0;
         if (pvcLookupStep(ctrl, setpoint, &lut_step) == 0)
         {
			// todo: ilk anda lookUpTable ile belli bir seviyede başlanması istenirse,
			// alt satır yorumdan çıkarılır. Bunun için kalibrasyon yapmak gerek.
            //ctrl->current_target_step = clampStep(ctrl, lut_step);
        	 ctrl->current_target_step = 20;
         }

         /* Bekleme sayacını başlat (ilk atlama → uzun bekleme) */
         ctrl->fb_timer = 0;
         ctrl->jump_settle_active = 1;
         ctrl->state = PVC_STATE_RUNNING_SETTLE;

         *out_step = ctrl->current_target_step;
         return 0;
      }

      case PVC_STATE_RUNNING_SETTLE:
         *out_step = ctrl->current_target_step;
         ctrl->fb_timer++;

         {

        	 uint16_t active_settle;
        	 if (ctrl->jump_settle_active)
        	    active_settle = ctrl->jump_settle_time;
        	 else if (ctrl->rapid_active)
        	    active_settle = ctrl->rapid_settle_time;
        	 else
        	 {
        	    float abs_err = fabsf(temperature - setpoint);
        	    active_settle = (abs_err > ctrl->fb_settle_thresh_2) ?
        	       ctrl->fb_settle_time_2 : ctrl->fb_settle_time;
        	 }


            if (ctrl->fb_timer >= active_settle)
            {
               /* İlk atlama beklemesi bittiyse flag'i kapat */
               ctrl->jump_settle_active = 0;

               /* Rapid düzeltme etki kontrolü */
               if (ctrl->rapid_active)
               {
                  float temp_diff = temperature - ctrl->rapid_temp_before;
                  float error_dir = temperature - setpoint;
                  uint8_t effective = ((error_dir > 0) && (temp_diff < -PVC_RAPID_EFFECT_DELTA)) ||
                                      ((error_dir < 0) && (temp_diff > PVC_RAPID_EFFECT_DELTA));

                  if (effective)
                  {
                     /* Step etkili oldu → ekstra bekleme ver, düzeltme yapma */
                     ctrl->fb_timer = 0;
                     ctrl->rapid_temp_before = temperature;
                     *out_step = ctrl->current_target_step;
                     return 0;
                  }
                  /* Etki yok → dead zone, bekleme yapmadan düzeltmeye devam */
               }


               float error = temperature - setpoint;
               float abs_error = fabsf(error);

               if (abs_error > ctrl->rapid_threshold)
               {
                  /* Büyük sapma → agresif düzeltme (rapid_step_size) */
                  int32_t step_delta = (error > 0) ?
                     -ctrl->rapid_step_size : ctrl->rapid_step_size;
                  ctrl->current_target_step = clampStep(ctrl,
                     ctrl->current_target_step + step_delta);
                  ctrl->fb_timer = 0;
                  ctrl->rapid_active = 1;
                  ctrl->rapid_temp_before = temperature;
                  /* SETTLE'da kal, tekrar bekle */
               }
               else if (abs_error > ctrl->fb_deadband)
               {
                  /* Orta sapma → kademeli düzeltme (fb_step_size) */
                  int32_t step_delta = (error > 0) ?
                     -ctrl->fb_step_size : ctrl->fb_step_size;
                  ctrl->current_target_step = clampStep(ctrl,
                     ctrl->current_target_step + step_delta);
                  ctrl->fb_timer = 0;
                  /* SETTLE'da kal, tekrar bekle */
               }
               else
               {
                  /* Deadband içinde → yerinde kal */
            	   ctrl->rapid_active = 0;
                  ctrl->state = PVC_STATE_RUNNING_HOLD;
               }
            }
         }

         *out_step = ctrl->current_target_step;
         return 0;

      case PVC_STATE_RUNNING_HOLD:
      {
         float error = temperature - setpoint;
         float abs_error = fabsf(error);

         /* Deadband dışına çıktı mı kontrol et */
         if (abs_error > ctrl->fb_deadband)
         {
            /* Deadband'den çıktı → düzeltmeye başla */
            ctrl->fb_timer = 0;
            ctrl->state = PVC_STATE_RUNNING_SETTLE;
         }

         *out_step = ctrl->current_target_step;
         return 0;
      }

      case PVC_STATE_EMERGENCY:
         *out_step = ctrl->emergency_step;
         return 0;

      default:
         return -1;
   }
}

int8_t pvcLookupStep(pvc_ctrl_t *ctrl, float target_temp, int32_t *out_step)
{
   if (ctrl == NULL || out_step == NULL || !ctrl->lut_valid || ctrl->lut_count < 2)
   {
      return -1;
   }

   /* Alt sınırın altında */
   if (target_temp <= ctrl->lut[0].temp)
   {
      *out_step = ctrl->lut[0].step;
      return 0;
   }

   /* Üst sınırın üstünde */
   if (target_temp >= ctrl->lut[ctrl->lut_count - 1].temp)
   {
      *out_step = ctrl->lut[ctrl->lut_count - 1].step;
      return 0;
   }

   /* İki nokta arası lineer interpolasyon */
   for (uint8_t i = 0; i < ctrl->lut_count - 1; i++)
   {
      if (target_temp >= ctrl->lut[i].temp && target_temp <= ctrl->lut[i + 1].temp)
      {
         float t1 = ctrl->lut[i].temp;
         float t2 = ctrl->lut[i + 1].temp;
         int32_t s1 = ctrl->lut[i].step;
         int32_t s2 = ctrl->lut[i + 1].step;

         float temp_range = t2 - t1;
         if (temp_range < 0.01f)
         {
            *out_step = s1;
            return 0;
         }

         float ratio = (target_temp - t1) / temp_range;
         float step_f = (float)s1 + ratio * (float)(s2 - s1);

         *out_step = (int32_t)(step_f + 0.5f);
         return 0;
      }
   }

   return -1;
}

int8_t pvcAddLutEntry(pvc_ctrl_t *ctrl, int32_t step, float temp)
{
   if (ctrl == NULL)
   {
      return -2;
   }

   if (ctrl->lut_count >= PVC_LUT_MAX_SIZE)
   {
      return -1;
   }

   ctrl->lut[ctrl->lut_count].step = step;
   ctrl->lut[ctrl->lut_count].temp = temp;
   ctrl->lut_count++;

   sortLutByTemp(ctrl);

   if (ctrl->lut_count >= 2)
   {
      ctrl->lut_valid = 1;
   }

   return 0;
}

pvc_state_e pvcGetState(pvc_ctrl_t *ctrl)
{
   if (ctrl == NULL)
   {
      return PVC_STATE_IDLE;
   }
   return ctrl->state;
}

uint8_t pvcGetCalibProgress(pvc_ctrl_t *ctrl)
{
   if (ctrl == NULL)
   {
      return 0;
   }

   int32_t total_steps = (ctrl->calib_step_end - ctrl->calib_step_start) / ctrl->calib_step_inc;
   if (total_steps <= 0)
   {
      return 100;
   }

   int32_t completed = (ctrl->calib_current_step - ctrl->calib_step_start) / ctrl->calib_step_inc;

   uint8_t progress = (uint8_t)((completed * 100) / total_steps);
   if (progress > 100)
   {
      progress = 100;
   }
   return progress;
}

void pvcClearLut(pvc_ctrl_t *ctrl)
{
   if (ctrl == NULL)
   {
      return;
   }

   ctrl->lut_count = 0;
   ctrl->lut_valid = 0;
   memset(ctrl->lut, 0, sizeof(ctrl->lut));
}
