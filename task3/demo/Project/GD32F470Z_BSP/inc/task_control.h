#ifndef TASK_CONTROL_H
#define TASK_CONTROL_H

#include "gd32f4xx.h"
#include "queue.h"

// 任务函数
void task_normal_update(void);
void task_short_press_sw1(void);
void task_long_press_sw1(void);
void task_history_prev(void);
void task_history_next(void);
void task_key_scan_with_long_press(void);
s8_para* task_get_current_data(void);

#endif