/**
 * @file    trace_log.h
 * @brief   Genel amacli, bloklamayan iz (trace) kaydi: printf-tarzi satirlari
 *          RAM'deki bir halka (ring) tampona yazar, sonra istenildiginde topluca
 *          UART'a doker.
 * @author  Emrah Duatepe
 *
 * Neden halka tampon?
 *   Uyku/aktif arasi gidip gelen cihazda UART cogu zaman kapali (deinit) ve
 *   dogrudan printf ya kayip ya da pahali olur. traceLog() sadece RAM'e yazar
 *   (bloklamaz, UART'a dokunmaz); olay gectikten sonra UART acikken traceLogDump()
 *   ile son N olayi geriye donuk incelersin. Tampon dolunca en eski kayit ustune
 *   yazilir; kac kaydin ezildigini traceLogDropped() verir.
 *
 * Kullanim:
 *   traceLog("wake mode=%d thief=%d", app_mode, thief_detected);
 *   ...
 *   traceLogDump();   // UART acikken ( or. aktif modda) cagir
 */
#ifndef TRACE_LOG_H
#define TRACE_LOG_H

#include "stdint.h"

/* ---- Yapilandirma (gerekirse projeye gore degistir) ---------------------- */
#ifndef TRACE_LOG_SLOTS
#define TRACE_LOG_SLOTS     32u   /**< Halkadaki kayit sayisi.            */
#endif
#ifndef TRACE_LOG_MSG_LEN
#define TRACE_LOG_MSG_LEN   64u   /**< Bir kaydin en fazla karakteri (NUL dahil). */
#endif

/**
 * @brief printf-tarzi bir satiri halka tampona yazar. Bloklamaz, UART'a dokunmaz.
 *        Tampon dolunca en eski kaydin ustune yazar. ISR'den de cagrilabilir.
 * @param fmt printf format dizgesi (sonrasinda degiskenler)
 */
void traceLog(const char *fmt, ...);

/**
 * @brief Biriken tum kayitlari (eskiden yeniye) printf ile UART'a doker.
 *        UART'in ACIK oldugu bir anda (or. aktif mod) cagrilmali.
 */
void traceLogDump(void);

/** @brief Halkayi bosaltir (kayit ve drop sayaclari sifirlanir). */
void traceLogClear(void);

/** @brief Su an tamponda bekleyen gecerli kayit sayisi. */
uint32_t traceLogCount(void);

/** @brief Tampon dolulugu nedeniyle uzerine yazilarak kaybedilen kayit sayisi. */
uint32_t traceLogDropped(void);

/**
 * @brief Her kayda basilacak zaman damgasini (ms) uretir.
 *        Varsayilan zayif (weak) tanim projedeki `systick`'i dondurur; kendi
 *        guclu tanimini yazarak RTC vb. baska bir kaynaga baglayabilirsin.
 */
uint32_t traceLogNow(void);

#endif /* TRACE_LOG_H */
