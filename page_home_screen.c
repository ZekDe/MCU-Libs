#include "page_home_screen.h"
#include "display7seg.h"
#include "device_context.h"
#include "NuMicro.h"
#include "stdio.h"
#include "ton.h"
#include "edge_detection.h"
#include "systemtick.h"
#include "stdint.h"
#include "page_setpoint.h"
#include "page_p1.h"
#include "page_navigation.h"

#define REFRESH_HOME_SCREEN_MS 1000

static ton_t ton_screen_refresh;
static edge_detection_t ed_screen_refresh;

//uint8_t screen_refresh_pulse;

static void onTick(uint32_t now) 
{
	if (TON(&ton_screen_refresh, 1, now, REFRESH_HOME_SCREEN_MS))
	{
		ton_screen_refresh.aux = 0;
		print_display("%.1f*", dev_data.temperature);
		displaySetLeds(LED1 | LED3);
		//screen_refresh_pulse = !screen_refresh_pulse;
	}
//	screen_refresh_pulse = edgeDetection(&ed_screen_refresh, screen_refresh_pulse);

//	if(screen_refresh_pulse)
//	{
//		print_display("%.1f*", dev_data.temperature);
//		displaySetLeds(0x01);
//	}
    
}
static void onButton(uint8_t btn, button_event_t evt) 
{
	if (btn == 1 && evt == BTN_EVT_SINGLE)
	{
		menuGoTo(&setpoint_page);
	}
	else if (btn == 2 && evt == BTN_EVT_SINGLE)
	{
		menuGoTo(&setpoint_page);
	}
	else if(btn == 3 && evt == BTN_EVT_SINGLE)
	{
		menuGoTo(&navigation_page);
	}

}

static void onEnter(void)
{
	ton_screen_refresh.aux = 1;
}

static void onExit(void)
{
	ton_screen_refresh.aux = 0;
}

const menu_page_t home_page = 
{
    .onTick   = onTick,
    .onButton = onButton,
	.onEnter = onEnter,
	.onExit = onExit,
};