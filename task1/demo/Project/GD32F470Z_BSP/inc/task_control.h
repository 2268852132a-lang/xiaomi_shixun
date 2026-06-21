#ifndef TASK_CONTROL_H
#define TASK_CONTROL_H

#include "gd32f4xx.h"

void task_led_onoff(void);
void task_led_color(void);
void task_fan_onoff(void);
void task_fan_speed(void);
void task_curtain_onoff(void);

#endif