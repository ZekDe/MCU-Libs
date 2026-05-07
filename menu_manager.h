/**
 * @file    menu_manager.h
 * @brief   Sayfa tabanli menu yoneticisi (page navigation router).
 *
 * Manager hicbir display/parametre/render mantigi tutmaz; sadece:
 *   - su an hangi sayfa aktif?
 *   - bir gecmis stack'i (menuBack icin)
 *   - aktif sayfanin callback'lerine event/tick yonlendirme
 *
 * Her sayfa bir menu_page_t struct'i ile tanimlanir, callback'ler kullanici
 * tarafindan yazilir. Sayfalar arasi gezinme menuGoTo / menuBack / menuHome ile.
 *
 * Kullanim:
 *   const menu_page_t home_page = { .onTick = home_tick, .onButton = home_btn };
 *   menuInit(&home_page);   // root page = home
 *   // main loop:
 *   menuFeedButton(0, btn1_evt);
 *   menuFeedButton(1, btn2_evt);
 *   menuFeedButton(2, btn3_evt);
 *   menuProcess(systick);
 */
#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H

#include <stdint.h>
#include "button_gesture.h"

#ifndef MENU_STACK_DEPTH
#define MENU_STACK_DEPTH 8   // maksimum hiyerarsi derinligi
#endif

typedef struct menu_page_s {
    void (*onEnter)(void);                              // sayfaya girildiginde 1 kez
    void (*onTick)(uint32_t now);                       // her main loop tick'inde
    void (*onButton)(uint8_t btn_id, button_event_t evt);
    void (*onExit)(void);                               // sayfadan cikilirken 1 kez
} menu_page_t;

/**
 * @brief Manager'i baslatir. root_page menuHome() icin "ana ekran" referansi olur.
 *        root_page'in onEnter'i bir sonraki menuProcess cagrisinda calisir.
 */
void menuInit(const menu_page_t *root_page);

/**
 * @brief Yeni sayfaya gec. Mevcut sayfa stack'e push edilir (menuBack icin).
 *        Gercek gecis bir sonraki menuProcess cagrisinda olur (deferred).
 */
void menuGoTo(const menu_page_t *page);

/**
 * @brief Stack'ten bir onceki sayfaya don. Stack bossa hicbir sey yapmaz.
 */
void menuBack(void);

/**
 * @brief Stack'i tamamen temizleyip root_page'e atla.
 */
void menuHome(void);

/**
 * @brief Aktif sayfanin onButton'ina event yonlendir.
 */
void menuFeedButton(uint8_t btn_id, button_event_t evt);

/**
 * @brief Bekleyen sayfa gecisini uygula (onExit -> onEnter), sonra aktif sayfanin
 *        onTick'ini cagir. Main loop'ta her tick cagrilmali.
 */
void menuProcess(uint32_t now);

/**
 * @brief Aktif sayfanin pointer'i (debug/diagnostic icin).
 */
const menu_page_t *menuCurrentPage(void);

#endif /* MENU_MANAGER_H */
