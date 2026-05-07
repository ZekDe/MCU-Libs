/**
 * @file    menu_manager.c
 * @brief   Sayfa tabanli menu yoneticisi implementasyonu.
 */
#include "menu_manager.h"

static const menu_page_t *current_page = 0;
static const menu_page_t *next_page    = 0;
static const menu_page_t *root_page    = 0;

static const menu_page_t *stack[MENU_STACK_DEPTH];
static uint8_t stack_size = 0;

static void stackPush(const menu_page_t *p)
{
    if (stack_size < MENU_STACK_DEPTH) {
        stack[stack_size++] = p;
    }
    // Stack tasarsa en eski entry'yi atip kaydirma yapilabilir; simdilik
    // sessizce dustur. MENU_STACK_DEPTH'i artir gerekirse.
}

static const menu_page_t *stackPop(void)
{
    if (stack_size == 0) return 0;
    return stack[--stack_size];
}

void menuInit(const menu_page_t *root)
{
    root_page    = root;
    current_page = 0;
    next_page    = root;
    stack_size   = 0;
}

void menuGoTo(const menu_page_t *page)
{
    if (!page || page == current_page) return;
    if (current_page) {
        stackPush(current_page);
    }
    next_page = page;
}

void menuBack(void)
{
    const menu_page_t *p = stackPop();
    if (p) {
        next_page = p;
    }
}

void menuHome(void)
{
    stack_size = 0;
    next_page  = root_page;
}

void menuFeedButton(uint8_t btn_id, button_event_t evt)
{
    if (evt == BTN_EVT_NONE) return;
    if (current_page && current_page->onButton) {
        current_page->onButton(btn_id, evt);
    }
}

void menuProcess(uint32_t now)
{
    // Bekleyen sayfa gecisi varsa uygula
    if (next_page != current_page) {
        if (current_page && current_page->onExit) {
            current_page->onExit();
        }
        current_page = next_page;
        if (current_page && current_page->onEnter) {
            current_page->onEnter();
        }
    }

    if (current_page && current_page->onTick) {
        current_page->onTick(now);
    }
}

const menu_page_t *menuCurrentPage(void)
{
    return current_page;
}
