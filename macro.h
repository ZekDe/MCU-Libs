#ifndef MACRO_H
#define MACRO_H

/**
 * @def MOVE(en, out, in)
 * @brief To transfer the content of the in to the out in accordance the en
 * en state '1' is active
 */
 
 #include "stdint.h"
 
#define MOVE(en, out, in)		((en) ? ((out) = (in)): 0)

#define SET_BIT(reg, bit)	((reg) |= (bit))
#define RESET_BIT(reg, bit)	((reg) &= ~(bit))

#define SET_BIT_POS(reg, pos)		((reg) |= (1UL << (pos)))
#define RESET_BIT_POS(reg, pos)	((reg) &= (~(1UL << (pos))))
#define READ_BIT_POS(reg, pos)		(((reg) >> (pos)) & 1UL)
#define TOGGLE_BIT_POS(reg, pos)	((reg) ^= (1UL << (pos)))
#define MODIFY_BIT_POS(reg, pos, _0_1)	((reg) ^= (-_0_1 ^ (reg)) & (1UL << (pos)))

#define SIZEX(x)	(sizeof((x)) / sizeof((x[0])))
#define CLEAR_STRUCT(s)	memset(&s, 0, sizeof(s))
	
#define arr_end(a) a + sizeof(a)/sizeof(*a)
	
static inline uint8_t signum(int32_t x)
{
	return (x > 0) - (x < 0);
}
static inline uint8_t isOddNum(int32_t x)
{
	return (x & 1) != 0;
}

static inline void seal(uint8_t seal, uint8_t breakTheSeal, uint8_t *out)
{
	*out = (seal || *out) && !breakTheSeal;
}


	
#endif
