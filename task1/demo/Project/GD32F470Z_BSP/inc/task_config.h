#ifndef TASK_CONFIG_H
#define TASK_CONFIG_H

#include "s1.h"

// 按键功能映射（需要和 s1.h 中的宏定义匹配）
#define KEY_NONE          SWN

#define KEY_LED_ONOFF     SW1
#define KEY_LED_COLOR     SW2
#define KEY_FAN_ONOFF     SW3
#define KEY_FAN_SPEED     SW4
#define KEY_CURTAIN_ONOFF SW5

// I2C 设备地址
#define E1_I2C_ADDR       0xC0
#define E2_I2C_ADDR       0xC8
#define CURTAIN_ADDRESS_E3 0x38

// 灯颜色
#define COLOR_WHITE       0
#define COLOR_RED         1
#define COLOR_GREEN       2
#define COLOR_BLUE        3

// 风扇转速百分比
#define FAN_SPEED_SLOW    20
#define FAN_SPEED_MID     50
#define FAN_SPEED_FAST    90

#endif