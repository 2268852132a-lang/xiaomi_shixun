#include "task_control.h"
#include "e1.h"
#include "e2.h"
#include "e3.h"
#include "task_config.h"
#include "gd32f470z_eval.h"
#include <stdio.h>
#include <string.h>

extern i2c_addr_def e1_dev;
extern i2c_addr_def e2_dev;
extern i2c_addr_def e3_dev;
extern volatile uint8_t led_state;
extern volatile uint8_t led_color;
extern volatile uint8_t fan_state;
extern volatile uint8_t fan_speed;
extern volatile uint8_t curtain_state;
extern uint8_t print_buffer[];
extern void debug_printf(uint32_t usart_periph, char *string);

#define FAN_SPEED_SLOW    20   // 慢速 20%
#define FAN_SPEED_MID     50   // 中速 50%
#define FAN_SPEED_FAST    90   // 快速 90%

// 1. 灯的开关
void task_led_onoff(void)
{
    led_state = !led_state;
    if(led_state) {
        switch(led_color) {
            case COLOR_WHITE: e1_rgb_control(e1_dev.periph, e1_dev.addr, 255, 255, 255); break;
            case COLOR_RED:   e1_rgb_control(e1_dev.periph, e1_dev.addr, 255, 0, 0); break;
            case COLOR_GREEN: e1_rgb_control(e1_dev.periph, e1_dev.addr, 0, 255, 0); break;
            case COLOR_BLUE:  e1_rgb_control(e1_dev.periph, e1_dev.addr, 0, 0, 255); break;
        }
        sprintf((char*)print_buffer, "LED ON, color=%d\r\n", led_color);
    } else {
        e1_rgb_control(e1_dev.periph, e1_dev.addr, 0, 0, 0);
        sprintf((char*)print_buffer, "LED OFF\r\n");
    }
    debug_printf(EVAL_COM0, (char*)print_buffer);
}

// 2. 灯的颜色切换
void task_led_color(void)
{
    led_color = (led_color + 1) % 4;
    if(led_state) {
        switch(led_color) {
            case COLOR_WHITE: e1_rgb_control(e1_dev.periph, e1_dev.addr, 255, 255, 255); break;
            case COLOR_RED:   e1_rgb_control(e1_dev.periph, e1_dev.addr, 255, 0, 0); break;
            case COLOR_GREEN: e1_rgb_control(e1_dev.periph, e1_dev.addr, 0, 255, 0); break;
            case COLOR_BLUE:  e1_rgb_control(e1_dev.periph, e1_dev.addr, 0, 0, 255); break;
        }
    }
    sprintf((char*)print_buffer, "LED color changed to %d\r\n", led_color);
    debug_printf(EVAL_COM0, (char*)print_buffer);
}

// 3. 风扇的开关
void task_fan_onoff(void)
{
    if(!e2_dev.flag) {
        sprintf((char*)print_buffer, "E2 not found!\r\n");
        debug_printf(EVAL_COM0, (char*)print_buffer);
        return;
    }
    
    fan_state = !fan_state;
    if(fan_state) {
        uint8_t speed = FAN_SPEED_SLOW;
        switch(fan_speed) {
            case 0: speed = FAN_SPEED_SLOW; break;
            case 1: speed = FAN_SPEED_MID; break;
            case 2: speed = FAN_SPEED_FAST; break;
        }
        e2_speed_control(e2_dev.periph, e2_dev.addr, speed);
        sprintf((char*)print_buffer, "FAN ON, speed=%d%%\r\n", speed);
    } else {
        e2_speed_control(e2_dev.periph, e2_dev.addr, 0);
        sprintf((char*)print_buffer, "FAN OFF\r\n");
    }
    debug_printf(EVAL_COM0, (char*)print_buffer);
}

// 4. 风扇的转速控制（慢→中→快→慢）
void task_fan_speed(void)
{
    if(!e2_dev.flag) {
        sprintf((char*)print_buffer, "E2 not found!\r\n");
        debug_printf(EVAL_COM0, (char*)print_buffer);
        return;
    }
    
    if(!fan_state) {
        // 如果风扇是关的，先打开
        task_fan_onoff();
        return;
    }
    
    fan_speed = (fan_speed + 1) % 3;
    uint8_t speed = FAN_SPEED_SLOW;
    switch(fan_speed) {
        case 0: speed = FAN_SPEED_SLOW; break;
        case 1: speed = FAN_SPEED_MID; break;
        case 2: speed = FAN_SPEED_FAST; break;
    }
    e2_speed_control(e2_dev.periph, e2_dev.addr, speed);
    sprintf((char*)print_buffer, "FAN speed changed to %d%%\r\n", speed);
    debug_printf(EVAL_COM0, (char*)print_buffer);
}

// 5. 窗帘的开关
void task_curtain_onoff(void)
{
    curtain_state = !curtain_state;
    if(curtain_state) {
        e3_set_position(e3_dev.periph, e3_dev.addr, 100);
        sprintf((char*)print_buffer, "CURTAIN OPEN\r\n");
    } else {
        e3_set_position(e3_dev.periph, e3_dev.addr, 0);
        sprintf((char*)print_buffer, "CURTAIN CLOSE\r\n");
    }
    debug_printf(EVAL_COM0, (char*)print_buffer);
}