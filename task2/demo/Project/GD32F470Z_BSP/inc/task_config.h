#ifndef TASK_CONFIG_H
#define TASK_CONFIG_H

#include "s1.h"

// ==================== 按键映射（3个按键）====================
#define KEY_NONE              SWN

#define KEY_CURTAIN_OPEN_10   SW1    // 按键1：开10%
#define KEY_CURTAIN_CLOSE_10  SW2    // 按键2：关10%
#define KEY_AUTO_MODE_TOGGLE  SW3    // 按键3：自适应模式开关

// ==================== I2C 地址 ====================
#define S2_I2C_ADDR           0x46   // 光照传感器地址
#define CURTAIN_ADDRESS_E3    0x38   // 窗帘电机地址

// ==================== 光照阈值 ====================
#define LUX_DARK_THRESHOLD     30.0f   // 完全遮住 < 30 lux
#define LUX_BRIGHT_THRESHOLD   500.0f  // 强光 > 500 lux

// ==================== 窗帘位置 ====================
#define CURTAIN_CLOSE          0
#define CURTAIN_HALF           50
#define CURTAIN_OPEN           100
#define CURTAIN_STEP           10

#endif