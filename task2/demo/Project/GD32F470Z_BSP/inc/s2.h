#ifndef S2_H
#define S2_H

#include "i2c.h"

#define S1_ADDRESS_S2          0x46
#define S2_ADDRESS_S2          0xB8

i2c_addr_def s2_init(uint8_t address);
float s2_read_bh1750_value(uint32_t i2c_periph, uint8_t i2c_addr);

#endif