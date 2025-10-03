#include "string.h"
#include "ctype.h"

void trim(char* str)
{
    if (str == NULL || *str == '\0')
        return;
    
    // Bastaki bosluklari bul
    char* begin = str;
    while (isspace((unsigned char)*begin))
        begin++;
    
    // Eger tüm string bosluksa
    if (*begin == '\0') {
        *str = '\0';
        return;
    }
    
    // Sondaki bosluklari bul
    char* end = str + strlen(str) - 1;
    while (end >= begin && isspace((unsigned char)*end))
        end--;
    
    // Yeni uzunluk
    size_t newlen = end + 1 - begin;
    
    // Eger basta bosluk varsa, içerigi öne tasi
    if (begin > str)
        memmove(str, begin, newlen);
    
    // Null terminator ekle
    str[newlen] = '\0';
}

void toUpper(char* str)
{
	for (char* p = str; *p; p++) {
		*p = toupper(*p);
	}
}