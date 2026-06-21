#include "gd32f4xx.h"
#include "gd32f470z_eval.h"
#include "s1.h"
#include "e1.h"
#include "e2.h"
#include "s6.h"
#include "s7.h"
#include "s8.h"
#include "task_config.h"
#include "task_control.h"
#include <stdio.h>
#include <string.h>

// ==================== 全局设备句柄 ====================
i2c_addr_def s1_dev;
i2c_addr_def e1_dev;
i2c_addr_def e2_dev;
i2c_addr_def s6_dev;
i2c_addr_def s7_dev;
i2c_addr_def s8_dev;

// ==================== 定时器变量 ====================
static volatile uint16_t delay_count = 0;
static volatile uint16_t ms_ticks = 0;
static volatile uint8_t flag_100ms = 0;
static volatile uint16_t tick_100ms = 0;
static volatile uint8_t flag_1s = 0;

uint8_t print_buffer[128];

// ==================== 函数声明 ====================
void timer3_init(void);
void delay_ms(uint16_t mstime);
void debug_printf(uint32_t usart_periph, char *string);

// ==================== 主函数 ====================
int main(void)
{
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
    gd_eval_com_init(EVAL_COM0);
    timer3_init();
    init_i2c();

    debug_printf(EVAL_COM0, "=== Smart Fan (task4) ===\r\n");

    // -------- S1 按键 --------
    s1_dev = s1_init(HT16K33_ADDRESS_S1);
    if(!s1_dev.flag) {
        debug_printf(EVAL_COM0, "ERROR: S1 keypad not found!\r\n");
        while(1);
    }
    debug_printf(EVAL_COM0, "S1 keypad OK\r\n");

    // -------- E1 数码管 + RGB --------
    e1_dev = e1_init(HT16K33_ADDRESS_E1);
    if(!e1_dev.flag) {
        debug_printf(EVAL_COM0, "WARNING: E1 display not found!\r\n");
    } else {
        debug_printf(EVAL_COM0, "E1 display OK\r\n");
        e1_digital_display(e1_dev.periph, e1_dev.addr, 8, 8, 8, 8);
        delay_ms(1000);
    }

    // -------- E2 风扇(PCA9685) --------
    e2_dev = e2_init(PCA9685_ADDRESS_E2);
    if(!e2_dev.flag) {
        debug_printf(EVAL_COM0, "WARNING: E2 fan not found!\r\n");
    } else {
        debug_printf(EVAL_COM0, "E2 fan OK\r\n");
        e2_speed_control(e2_dev.periph, e2_dev.addr, 0);
    }

    // -------- S6 超声波测距 --------
    s6_dev = s6_init(GD32F330_ADDRESS_S6);
    if(!s6_dev.flag) {
        debug_printf(EVAL_COM0, "WARNING: S6 ultrasonic not found!\r\n");
    } else {
        debug_printf(EVAL_COM0, "S6 ultrasonic OK\r\n");
    }

    // -------- S7 人体红外 --------
    s7_dev = s7_init(PCA9557_ADDRESS_S7);
    if(!s7_dev.flag) {
        debug_printf(EVAL_COM0, "WARNING: S7 IR sensor not found!\r\n");
    } else {
        debug_printf(EVAL_COM0, "S7 IR sensor OK\r\n");
    }

    // -------- S8 温湿度(SHT30) --------
    s8_dev = s8_init(TH_ADDRESS_S8);
    if(!s8_dev.flag) {
        debug_printf(EVAL_COM0, "WARNING: S8 temp/hum sensor not found!\r\n");
    } else {
        debug_printf(EVAL_COM0, "S8 temp/hum sensor OK\r\n");
    }

    debug_printf(EVAL_COM0, "System ready. Press KEY1 to start.\r\n");

    // ==================== 主循环 ====================
    while(1)
    {
        if(flag_100ms) {
            flag_100ms = 0;
            task_key_scan_100ms();
        }

        if(flag_1s) {
            flag_1s = 0;
            task_1s_update();
        }
    }
}

// ==================== 定时器3初始化(1ms) ====================
void timer3_init(void)
{
    timer_parameter_struct timer_init_struct;
    rcu_periph_clock_enable(RCU_TIMER3);
    timer_deinit(TIMER3);
    timer_init_struct.prescaler = 8399;
    timer_init_struct.period = 9;
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
    uint16_t len = strlen(string);
    for(uint16_t i = 0; i < len; i++) {
        while(usart_flag_get(usart_periph, USART_FLAG_TC) == RESET);
        usart_data_transmit(usart_periph, string[i]);
    }
    while(usart_flag_get(usart_periph, USART_FLAG_TC) == RESET);
}

void TIMER3_IRQHandler(void)
{
    if(timer_interrupt_flag_get(TIMER3, TIMER_INT_FLAG_UP)) {
        timer_interrupt_flag_clear(TIMER3, TIMER_INT_FLAG_UP);
        if(delay_count > 0) delay_count--;
        ms_ticks++;
        tick_100ms++;
        if(tick_100ms >= 100) {
            tick_100ms = 0;
            flag_100ms = 1;
        }
        if((ms_ticks % 1000) == 0) {
            flag_1s = 1;
        }
    }
}
