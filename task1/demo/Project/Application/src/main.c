#include "gd32f4xx.h"
#include "gd32f470z_eval.h"
#include "s1.h"
#include "e1.h"
#include "e2.h"
#include "e3.h"
#include "i2c.h"
#include "task_config.h"
#include "task_control.h"
#include <stdio.h>
#include <string.h>

// ==================== 全局变量 ====================
static volatile uint16_t delay_count = 0;
static volatile uint16_t time_count = 0;
static volatile uint8_t second_flag = 0;

// I2C 设备地址结构体
i2c_addr_def s1_dev;
i2c_addr_def e1_dev;
i2c_addr_def e2_dev;
i2c_addr_def e3_dev;

// 状态变量（在 task_control.c 中使用）
volatile uint8_t led_state = 0;
volatile uint8_t led_color = 0;
volatile uint8_t fan_state = 0;
volatile uint8_t fan_speed = 0;
volatile uint8_t curtain_state = 0;

uint8_t print_buffer[100];

// ==================== 函数声明 ====================
void timer3_init(void);
void delay_ms(uint16_t mstime);
uint16_t uart_print(uint32_t usart_periph, uint8_t *data, uint16_t len);
void debug_printf(uint32_t usart_periph, char *string);

// ==================== 主函数 ====================
int main(void)
{
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
    gd_eval_com_init(EVAL_COM0);
    timer3_init();
    init_i2c();

    sprintf((char*)print_buffer, "System init OK\r\n");
    debug_printf(EVAL_COM0, (char*)print_buffer);

    // 初始化 S1 按键板
    s1_dev = s1_init(HT16K33_ADDRESS_S1);
    if(!s1_dev.flag) {
        sprintf((char*)print_buffer, "ERROR: S1 not found!\r\n");
        debug_printf(EVAL_COM0, (char*)print_buffer);
        while(1);
    }

    // 初始化 E1 灯板
    e1_dev = e1_init(E1_I2C_ADDR);
    if(!e1_dev.flag) {
        sprintf((char*)print_buffer, "ERROR: E1 not found!\r\n");
        debug_printf(EVAL_COM0, (char*)print_buffer);
        while(1);
    }

    // 初始化 E2 风扇（使用 demo 的方式，自动扫描地址）
    e2_dev = e2_init(E2_I2C_ADDR);
    if(!e2_dev.flag) {
        sprintf((char*)print_buffer, "WARNING: E2 not found! Fan disabled.\r\n");
        debug_printf(EVAL_COM0, (char*)print_buffer);
    } else {
        sprintf((char*)print_buffer, "E2 found at addr=0x%X\r\n", e2_dev.addr);
        debug_printf(EVAL_COM0, (char*)print_buffer);
    }

    // 初始化 E3 窗帘
    e3_dev = e3_init(CURTAIN_ADDRESS_E3);
    if(!e3_dev.flag) {
        sprintf((char*)print_buffer, "WARNING: E3 not found!\r\n");
        debug_printf(EVAL_COM0, (char*)print_buffer);
    }

    // 初始状态
    e1_rgb_control(e1_dev.periph, e1_dev.addr, 0, 0, 0);
    if(e2_dev.flag) {
        e2_speed_control(e2_dev.periph, e2_dev.addr, 0);
    }
    if(e3_dev.flag) {
        e3_set_position(e3_dev.periph, e3_dev.addr, 0);
    }

    sprintf((char*)print_buffer, "All devices initialized\r\n");
    debug_printf(EVAL_COM0, (char*)print_buffer);

    uint8_t last_key = KEY_NONE;
    uint8_t key;

    while(1)
    {
        if(second_flag) {
            second_flag = 0;

            if(s1_dev.flag) {
                key = s1_key_scan(s1_dev.periph, s1_dev.addr);

                if(key != KEY_NONE && key != last_key) {
                    delay_ms(20);

                    if(s1_key_scan(s1_dev.periph, s1_dev.addr) == key) {
                        switch(key) {
                            case KEY_LED_ONOFF:  task_led_onoff(); break;
                            case KEY_LED_COLOR:  task_led_color(); break;
                            case KEY_FAN_ONOFF:  task_fan_onoff(); break;
                            case KEY_FAN_SPEED:  task_fan_speed(); break;
                            case KEY_CURTAIN_ONOFF: if(e3_dev.flag) task_curtain_onoff(); break;
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

uint16_t uart_print(uint32_t usart_periph, uint8_t *data, uint16_t len)
{
    uint8_t i;
    for(i = 0; i < len; i++) {
        while(usart_flag_get(usart_periph, USART_FLAG_TC) == RESET);
        usart_data_transmit(usart_periph, data[i]);
    }
    while(usart_flag_get(usart_periph, USART_FLAG_TC) == RESET);
    return len;
}

void debug_printf(uint32_t usart_periph, char *string)
{
    uint8_t buffer[100];
    uint16_t len;
    len = strlen(string);
    strncpy((char*)buffer, string, len);
    uart_print(usart_periph, buffer, len);
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