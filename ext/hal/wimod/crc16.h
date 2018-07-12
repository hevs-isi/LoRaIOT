#ifndef    __CRC16_H__
#define    __CRC16_H__

#include <kernel.h>

//------------------------------------------------------------------------------
//
//  Section Defines & Declarations
//
//------------------------------------------------------------------------------

#define CRC16_INIT_VALUE    0xFFFF    // initial value for CRC algorithem
#define CRC16_GOOD_VALUE    0x0F47    // constant compare value for check
#define CRC16_POLYNOM       0x8408    // 16-BIT CRC CCITT POLYNOM

//------------------------------------------------------------------------------
//
//  Function Prototypes
//
//------------------------------------------------------------------------------

// Calc CRC16
u16_t CRC16_Calc  (u8_t* data, u16_t length, u16_t initVal);

// Calc & Check CRC16
bool CRC16_Check (u8_t* data, u16_t length, u16_t initVal);


#endif // __CRC16_H__
//------------------------------------------------------------------------------
// end of file
//------------------------------------------------------------------------------
