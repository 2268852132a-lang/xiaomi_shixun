#include "gd32f4xx.h"
#include "gd32f470z_eval.h"
#include "s1.h"
#include "s2.h"
#include "e3.h"
#include "i2c.h"
#include "../../GD32F470Z_BSP/inc/task_config.h"   // ← 必须有这一行
#include "task_control.h"
#include <stdio.h>
#include <string.h>

// 全局变量
static volatile uint16_t delay_count = 0;
static volatile uint16_t time_count = 0;
static volatile uint8_t second_flag = 0;

i2c_addr_def s1_dev;
i2c_addr_def s2_dev;
i2c_addr_def e3_dev;

uint8_t print_buffer[100];

void timer3_init(void);
void delay_ms(uint16_t mstime);
void debug_printf(uint32_t usart_periph, char *string);

int main(void)
{
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
    gd_eval_com_init(EVAL_COM0);
    timer3_init();
    init_i2c();

    debug_printf(EVAL_COM0, "=== Smart Curtain System (task2) ===\r\n");

    // 初始化 S1 按键
    s1_dev = s1_init(HT16K33_ADDRESS_S1);
    if(!s1_dev.flag) {
        debug_printf(EVAL_COM0, "ERROR: S1 not found!\r\n");
        while(1);
    }

    // 初始化 S2 光照传感器
    s2_dev = s2_init(0x46);
    if(!s2_dev.flag) {
        debug_printf(EVAL_COM0, "WARNING: S2 not found! Auto mode disabled.\r\n");
    } else {
        debug_printf(EVAL_COM0, "S2 light sensor found\r\n");
    }

    // 初始化 E3 窗帘
    e3_dev = e3_init(0x38);
    if(!e3_dev.flag) {
        debug_printf(EVAL_COM0, "ERROR: E3 not found!\r\n");
        while(1);
    }

    // 初始状态：窗帘关闭
    e3_set_position(e3_dev.periph, e3_dev.addr, 0);
    debug_printf(EVAL_COM0, "System ready. Press keys to control.\r\n");

    uint8_t last_key = KEY_NONE;
    uint8_t key;

    while(1)
    {
        if(second_flag) {
            second_flag = 0;
            
            // 自适应模式自动更新
            task_auto_update();
            
            // 按键扫描
            if(s1_dev.flag) {
                key = s1_key_scan(s1_dev.periph, s1_dev.addr);
                
                if(key != KEY_NONE && key != last_key) {
                    delay_ms(20);
                    
                    if(s1_key_scan(s1_dev.periph, s1_dev.addr) == key) {
                        switch(key) {
                            case KEY_CURTAIN_OPEN_10:
                                task_curtain_open_10();
                                break;
                            case KEY_CURTAIN_CLOSE_10:
                                task_curtain_close_10();
                                break;
                            case KEY_AUTO_MODE_TOGGLE:
                                task_auto_mode_toggle();
                                break;
                            default:
                                sprintf((char*)print_buffer, "Unknown key: %d\r\n", key);
                                debug_printf(EVAL_COM0, (char*)print_buffer);
                                break;
                        }
                        while(s1_key_scan(s1_dev.periph, s1_dev.addr) == key);
                    }
                }
                last_key = key;
            }
        }
    }
}

void timer3_init(void)
{
    timer_parameter_struct timer_init_struct;
    rcu_periph_clock_enable(RCU_TIMER3);
    timer_deinit(TIMER3);
    timer_init_struct.prescaler = 4199;
    timer_init_struct.period = 20;
    timer_init_struct.alignedmode = TIMER_COUNTER_EDGE;
    timer_init_struct.counterdirection = TIMER_COUNTER_UP;
    timer_init_struct.clockdivision = TIMER_CKDIV_DIV1;
    timer_init_struct.repetitioncounter = 0;
    timer_init(TIMER3, &timer_init_struct);
    nvic_irq_enable(TIMER3_IRQn, 1, 1);
    timer_interrupt_enable(TIMER3, TIMER_INT_UP);
    timer_enable(TIMER3);
}

void delay_ms(uint16_t mstime)
{
    delay_count = mstime;
    while(delay_count);
}

void debug_printf(uint32_t usart_periph, char *string)
{
    uint8_t buffer[100];
    uint16_t len = strlen(string);
    strncpy((char*)buffer, string, len);
    for(uint8_t i = 0; i < len; i++) {
        while(usart_flag_get(usart_periph, USART_FLAG_TC) == RESET);
        usart_data_transmit(usart_periph, buffer[i]);
    }
    while(usart_flag_get(usart_periph, USART_FLAG_TC) == RESET);
}

void TIMER3_IRQHandler(void)
{
    if(timer_interrupt_flag_get(TIMER3, TIMER_INT_FLAG_UP)) {
        timer_interrupt_flag_clear(TIMER3, TIMER_INT_FLAG_UP);
        if(delay_count > 0) delay_count--;
        if(time_count++ >= 700) {
            time_count = 0;
            second_flag = 1;
        }
    }
}