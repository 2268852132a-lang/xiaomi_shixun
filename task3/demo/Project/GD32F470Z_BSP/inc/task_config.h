#ifndef TASK_CONFIG_H
#define TASK_CONFIG_H

#include "s1.h"

// ==================== 按键映射 ====================
#define KEY_NONE              SWN
#define KEY_MODE_TOGGLE       SW1    // 短按:开关 长按:历史模式
#define KEY_HISTORY_PREV      SW2    // 历史模式:后退
#define KEY_HISTORY_NEXT      SW3    // 历史模式:前进

// ==================== 长按时间 ====================
#define LONG_PRESS_TIME_MS    1000   // 1秒

// ==================== I2C 地址 ====================
#define E1_TUBE_ADDR          0xE0   // 数码管

// ==================== 队列大小 ====================
#define HISTORY_SIZE          100

// ==================== 显示模式 ====================
#define DISPLAY_MODE_NORMAL   0
#define DISPLAY_MODE_HISTORY  1

#endif