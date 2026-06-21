#ifndef E3_H
#define E3_H

#include "i2c.h"

typedef struct
{
    i2c_addr_def curtain_addr[4];
}e3_addr_def;

// 函数声明
i2c_addr_def e3_init(uint8_t address);
void e3_all_init(e3_addr_def *e3_address, uint8_t e3_addr);
void e3_set_position(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t position);
uint8_t e3_get_position(uint32_t i2c_periph, uint8_t i2c_addr);
void e3_stop(uint32_t i2c_periph, uint8_t i2c_addr);  // 新增急停函数

#endif