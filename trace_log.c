/**
 * @file    trace_log.c
 * @brief   Halka tampon tabanli iz (trace) kaydi. Bkz. trace_log.h.
 * @author  Emrah Duatepe
 */

#include "trace_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Kritik bolge: head/count ilerletmeyi ana dongu ile ISR yarisindan korur.
   NuMicro/CMSIS __disable_irq/__enable_irq kullanir. Baska bir cekirdekte veya
   tek-kanaldan cagriyorsan TRACE_LOG_NO_CRITICAL tanimlayarak kapatabilirsin. */
#define TRACE_LOG_NO_CRITICAL
#ifndef TRACE_LOG_NO_CRITICAL
#include "NuMicro.h"
#define TRACE_ENTER_CRITICAL()   uint32_t _tl_pm = __get_PRIMASK(); __disable_irq()
#define TRACE_EXIT_CRITICAL()    do { if (!_tl_pm) __enable_irq(); } while (0)
#else
#define TRACE_ENTER_CRITICAL()   ((void)0)
#define TRACE_EXIT_CRITICAL()    ((void)0)
#endif

#define DEBUG_PRINT_TRACE_LOG 1
typedef struct
{
   uint32_t timestamp;                 /**< traceLogNow() anindaki degeri (ms). */
   uint32_t seq;                       /**< Monoton kayit numarasi (kayip tespiti). */
   char     msg[TRACE_LOG_MSG_LEN];    /**< NUL sonlu, bicimlenmis satir.        */
} trace_entry_t;

static trace_entry_t s_ring[TRACE_LOG_SLOTS];
static uint32_t s_head;      /* bir sonraki YAZILACAK slot indeksi          */
static uint32_t s_count;     /* tamponda bekleyen gecerli kayit (<= SLOTS)  */
static uint32_t s_seq;       /* toplam uretilen kayit numarasi              */
static uint32_t s_dropped;   /* uzerine yazilarak kaybedilen kayit sayisi   */

void traceLog(const char *fmt, ...)
{
   char    tmp[TRACE_LOG_MSG_LEN];
   va_list ap;
   uint32_t now;

   /* Bicimlemeyi kritik bolge DISINDA, yerel yigin tamponuna yap: vsnprintf
      uzun surebilir, IRQ'yi bu sure boyunca kilitlemeyelim. */
   va_start(ap, fmt);
   vsnprintf(tmp, sizeof(tmp), fmt, ap);
   va_end(ap);

   now = traceLogNow();

   /* Sonra tek slotu atomik olarak isle (head/count/seq tutarli kalsin). */
   TRACE_ENTER_CRITICAL();
   {
      trace_entry_t *e = &s_ring[s_head];
      e->timestamp = now;
      e->seq       = s_seq++;
      memcpy(e->msg, tmp, sizeof(e->msg));   /* tmp NUL sonlu; tam kopya guvenli */

      s_head = (s_head + 1u) % TRACE_LOG_SLOTS;
      if (s_count < TRACE_LOG_SLOTS)
         s_count++;
      else
         s_dropped++;                        /* dolu: en eski kayit ezildi */
   }
   TRACE_EXIT_CRITICAL();
}

void traceLogDump(void)
{
   uint32_t count;
   uint32_t dropped;
   uint32_t idx;
   uint32_t i;

   /* En eski kaydin indeksini ve o anki sayaclari atomik oku. */
   TRACE_ENTER_CRITICAL();
   count   = s_count;
   dropped = s_dropped;
   idx     = (s_head + TRACE_LOG_SLOTS - count) % TRACE_LOG_SLOTS;
   TRACE_EXIT_CRITICAL();
#if DEBUG_PRINT_TRACE_LOG 
   printf("---- TRACE (%lu kayit, %lu drop) ----\n",
          (unsigned long)count, (unsigned long)dropped);

   /* Kayitlari eskiden yeniye yaz. Not: cok yogun loglamada dokum sirasinda bir
      kaydin uzerine yazilabilir; teshis icin kabul edilebilir, genelde bosta/aktif
      modda dokum alinir. */
   for (i = 0; i < count; i++)
   {
      trace_entry_t *e = &s_ring[idx];
      printf("[%8lu] #%lu %s\n",
             (unsigned long)e->timestamp,
             (unsigned long)e->seq,
             e->msg);
      idx = (idx + 1u) % TRACE_LOG_SLOTS;
   }

   printf("-------------------------------------\n");
#endif
}

void traceLogClear(void)
{
   TRACE_ENTER_CRITICAL();
   s_head    = 0;
   s_count   = 0;
   s_dropped = 0;
   /* s_seq kasitli sifirlanmaz: numaralar oturum boyu monoton kalsin. */
   TRACE_EXIT_CRITICAL();
}

uint32_t traceLogCount(void)
{
   uint32_t c;
   TRACE_ENTER_CRITICAL();
   c = s_count;
   TRACE_EXIT_CRITICAL();
   return c;
}

uint32_t traceLogDropped(void)
{
   uint32_t d;
   TRACE_ENTER_CRITICAL();
   d = s_dropped;
   TRACE_EXIT_CRITICAL();
   return d;
}

/* Zayif (weak) varsayilan: projedeki systick'i (ms) dondurur. Kendi guclu
   traceLogNow() tanimini yazarsan (or. RTC) bu devre disi kalir. */
#if defined(__GNUC__) || defined(__ARMCC_VERSION)
__attribute__((weak))
#endif
uint32_t traceLogNow(void)
{
   extern volatile uint32_t systick;   /* systemtick.h */
   return systick;
}
