#ifndef TASK_CONTROL_H
#define TASK_CONTROL_H

#include "gd32f4xx.h"

// task2 函数声明（3个按键）
void task_curtain_open_10(void);
void task_curtain_close_10(void);
void task_auto_mode_toggle(void);
void task_auto_update(void);

#endif