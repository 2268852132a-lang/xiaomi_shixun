#include "task_control.h"
#include "../inc/e3.h"
#include "../inc/s2.h"
#include "../inc/task_config.h"
#include "gd32f470z_eval.h"
#include <stdio.h>
#include <string.h>

// 外部变量声明
extern i2c_addr_def e3_dev;
extern i2c_addr_def s2_dev;
extern uint8_t print_buffer[];
extern void debug_printf(uint32_t usart_periph, char *string);

// 窗帘状态变量
static uint8_t target_position = 0;
static uint8_t auto_mode = 0;
static float last_lux = 0;

/*********************************************************************************************************
函数名:     set_curtain_position
描述:      设置窗帘目标位置
**********************************************************************************************************/
static void set_curtain_position(uint8_t position)
{
    if(position > 100) position = 100;
    
    target_position = position;
    e3_set_position(e3_dev.periph, e3_dev.addr, position);
    
    sprintf((char*)print_buffer, "Curtain moving to: %d%%\r\n", position);
    debug_printf(EVAL_COM0, (char*)print_buffer);
}

/*********************************************************************************************************
函数名:     update_curtain_by_light
描述:      根据光照值自动调整窗帘位置
**********************************************************************************************************/
static void update_curtain_by_light(float lux)
{
    uint8_t new_position;
    
    if(lux < LUX_DARK_THRESHOLD) {
        new_position = CURTAIN_OPEN;
        sprintf((char*)print_buffer, "Light: DARK (%.1f lux) -> Curtain OPEN\r\n", lux);
    }
    else if(lux > LUX_BRIGHT_THRESHOLD) {
        new_position = CURTAIN_CLOSE;
        sprintf((char*)print_buffer, "Light: BRIGHT (%.1f lux) -> Curtain CLOSE\r\n", lux);
    }
    else {
        new_position = CURTAIN_HALF;
        sprintf((char*)print_buffer, "Light: NORMAL (%.1f lux) -> Curtain HALF (50%%)\r\n", lux);
    }
    
    if(new_position != target_position) {
        set_curtain_position(new_position);
    }
}

/*********************************************************************************************************
函数名:     task_curtain_open_10
描述:      按键1：窗帘开10%
**********************************************************************************************************/
void task_curtain_open_10(void)
{
    if(auto_mode) {
        debug_printf(EVAL_COM0, "Auto mode ON, manual control disabled\r\n");
        return;
    }
    
    int new_pos = target_position + CURTAIN_STEP;
    if(new_pos > 100) new_pos = 100;
    set_curtain_position(new_pos);
}

/*********************************************************************************************************
函数名:     task_curtain_close_10
描述:      按键2：窗帘关10%
**********************************************************************************************************/
void task_curtain_close_10(void)
{
    if(auto_mode) {
        debug_printf(EVAL_COM0, "Auto mode ON, manual control disabled\r\n");
        return;
    }
    
    int new_pos = target_position - CURTAIN_STEP;
    if(new_pos < 0) new_pos = 0;
    set_curtain_position(new_pos);
}

/*********************************************************************************************************
函数名:     task_auto_mode_toggle
描述:      按键3：环境自适应模式开关
**********************************************************************************************************/
void task_auto_mode_toggle(void)
{
    auto_mode = !auto_mode;
    
    if(auto_mode) {
        debug_printf(EVAL_COM0, "Auto mode ENABLED\r\n");
        if(s2_dev.flag) {
            last_lux = s2_read_bh1750_value(s2_dev.periph, s2_dev.addr);
            update_curtain_by_light(last_lux);
        }
    } else {
        debug_printf(EVAL_COM0, "Auto mode DISABLED\r\n");
    }
}

/*********************************************************************************************************
函数名:     task_auto_update
描述:      自适应模式下定期更新（每1秒调用一次）
**********************************************************************************************************/
void task_auto_update(void)
{
    if(auto_mode && s2_dev.flag) {
        float lux = s2_read_bh1750_value(s2_dev.periph, s2_dev.addr);
        
        if(lux > last_lux + 20.0f || lux < last_lux - 20.0f) {
            last_lux = lux;
            update_curtain_by_light(lux);
        }
    }
}