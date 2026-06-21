#ifndef TASK_CONFIG_H
#define TASK_CONFIG_H

#include "s1.h"

// ==================== 按键映射 ====================
#define KEY_NONE              SWN
#define KEY_POWER_TOGGLE      SW1    // 短按: 开机/关机
#define KEY_SPEED_UP          SW2    // 短按: 增速+10%
#define KEY_SPEED_DOWN        SW3    // 短按: 减速-10%
#define KEY_DISPLAY           SW4    // 短按: 显示风速3s  长按: 连续显示模式
#define KEY_DISTANCE_MODE     SW5    // 短按: 距离自适应模式
#define KEY_TEMP_MODE         SW6    // 短按: 温控模式
#define KEY_TEMP_CONFIRM      SW7    // 短按: 温控确认/退出

// ==================== 长按时间 ====================
#define LONG_PRESS_SCAN_COUNT   10   // 100ms扫描周期 × 10 = 1s

// ==================== I2C 地址 ====================
#define S8_SHT30_ADDR         0x88

// ==================== 显示模式 ====================
#define DISPLAY_MODE_NORMAL       0
#define DISPLAY_MODE_CONTINUOUS   1

// ==================== 控制模式 ====================
#define CTRL_MODE_MANUAL          0
#define CTRL_MODE_DISTANCE        1
#define CTRL_MODE_TEMP            2

// ==================== 温控阶段 ====================
#define TEMP_PHASE_INIT           0
#define TEMP_PHASE_CONTROL        1

#endif
