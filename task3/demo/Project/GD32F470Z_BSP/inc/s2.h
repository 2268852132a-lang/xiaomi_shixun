#ifndef S2_H
#define S2_H

#include "i2c.h"

// 光照传感器地址
#define S1_ADDRESS_S2          0x46
#define S2_ADDRESS_S2          0xB8

// 温湿度传感器地址（S2 合体板上的 SHT35）
#define S2_SHT35_ADDR          0x88

// 光照和温湿度数据结构
typedef struct {
    float temperature;  // 温度 (℃)
    float humidity;     // 湿度 (%)
    float light;        // 光照 (lux)
} s2_data_t;

// 函数声明
i2c_addr_def s2_init(uint8_t address);
float s2_read_bh1750_value(uint32_t i2c_periph, uint8_t i2c_addr);
s2_data_t s2_read_all(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t sht35_addr);

// 温湿度读取函数（从 s8.c 移植）
void s2_sht35_init(uint32_t i2c_periph, uint8_t i2c_addr);
s2_data_t s2_read_sht35(uint32_t i2c_periph, uint8_t i2c_addr);

#endif