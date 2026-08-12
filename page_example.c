#include "page_child_lock.h"
#include "menu_manager.h"



#define NO_TOUCH_MS 30000   /* etkilesim sonrasi bu kadar bosta kalinca uykuya */

static ton_t ton_notouch, ton_blink;
static uint8_t blink;



static void onTick(uint32_t now)
{
	if (TON(&ton_blink, 1, now, blink ? 800 : 200))
	{
		ton_blink.aux = 0;
		blink = !blink;
		
		if(blink) //on
		else      //off
	}
	
	
	if (TON(&ton_notouch, power, now, NO_TOUCH_MS))
	{
		ton_notouch.aux = 0;
		menuHome();
	}
}

static void onButton(uint8_t btn, button_event_t evt)
{
	ton_notouch.aux = 0;
	
	if (btn == BTN_MODE && evt == BTN_EVT_SINGLE)
	{
		
	}
	else if (btn == BTN_POWER && evt == BTN_EVT_SINGLE)
	{
		menuBack();
	}
	else if (btn == BTN_MENU && evt == BTN_EVT_SINGLE)
	{

	}

}

static void onEnter(void)
{

}

static void onExit(void)
{

}

const menu_page_t page_example = 
{
    .onTick   = onTick,
    .onButton = onButton,
	.onEnter = onEnter,
	.onExit = onExit,
};