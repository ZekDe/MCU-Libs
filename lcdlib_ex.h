#ifndef __LCDLIB_EX_H
#define __LCDLIB_EX_H

#include "stdint.h"

void lcdPutChar(uint32_t u32Zone, char ch);
void lcdPutString(uint32_t startZone, const char *str);
void lcdSetSymbol(uint32_t symbol, uint32_t onOff);

#endif

