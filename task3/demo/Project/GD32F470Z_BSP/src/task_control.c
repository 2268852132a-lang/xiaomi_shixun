#include "task_control.h"
#include "e1.h"
#include "s2.h"
#include "c3.h"
#include "queue.h"
#include "../inc/task_config.h"   // ← 必须有这一行，注意路径
#include "gd32f470z_eval.h"
#include <stdio.h>
#include <string.h>

// 外部变量
extern i2c_addr_def e1_dev;
extern i2c_addr_def s2_dev;
extern i2c_addr_def s1_dev;
extern uint8_t print_buffer[];
extern void debug_printf(uint32_t usart_periph, char *string);

// 状态变量（上电自动开启）
static uint8_t device_on = 1;
static uint8_t display_mode = DISPLAY_MODE_NORMAL;
static s8_para current_data;
static s8_para history_display_data;
static uint16_t history_index = 0;

// WiFi 状态机
static int wifi_run_state = 1;
static int tcp_status = -1;

// SHT35 的 I2C 地址
#define SHT35_ADDR  0x88

// 更新数码管显示
static void update_display(void)
{
    if(!device_on) {
        e1_digital_display(e1_dev.periph, e1_dev.addr, NODIS, NODIS, NODIS, NODIS);
        return;
    }
    
    if(display_mode == DISPLAY_MODE_NORMAL) {
        uint8_t temp = (uint8_t)(current_data.temperature + 0.5f);
        uint8_t hum = (uint8_t)(current_data.humidity + 0.5f);
        e1_digital_display(e1_dev.periph, e1_dev.addr,
                           temp / 10, temp % 10,
                           hum / 10, hum % 10);
    } else {
        uint8_t temp = (uint8_t)(history_display_data.temperature + 0.5f);
        uint8_t hum = (uint8_t)(history_display_data.humidity + 0.5f);
        e1_digital_display(e1_dev.periph, e1_dev.addr,
                           temp / 10, temp % 10,
                           hum / 10, hum % 10);
    }
}

// 构建并发送 HTML
static void send_html_data(void)
{
    char html_buf[256];
    sprintf(html_buf, 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<html><head><meta charset=utf-8><meta http-equiv=refresh content=2></head>"
        "<body><h1>%.1f°C</h1><h1>%.1f%%</h1></body></html>",
        current_data.temperature, current_data.humidity);
    
    c3_wifi_tcp_send(html_buf);
}

// 正常模式更新（每1秒）
void task_normal_update(void)
{
    static uint8_t first_run = 1;
    
    if(!device_on) return;
    if(!s2_dev.flag) return;
    
    if(first_run) {
        delay_ms(500);
        first_run = 0;
    }
    
    s2_data_t s2_data = s2_read_sht35(s2_dev.periph, SHT35_ADDR);
    current_data.temperature = s2_data.temperature;
    current_data.humidity = s2_data.humidity;
    
    history_push(&current_data);
    
    if(display_mode == DISPLAY_MODE_NORMAL) {
        update_display();
    }
    
    c3_wifi_tcp_lead(&wifi_run_state, &tcp_status);
    if(tcp_status == 0) {
        send_html_data();
    }
    
    sprintf((char*)print_buffer, "Temp: %.1f, Hum: %.1f\r\n", 
            current_data.temperature, current_data.humidity);
    debug_printf(EVAL_COM0, (char*)print_buffer);
}

void task_history_prev(void)
{
    if(display_mode == DISPLAY_MODE_HISTORY && history_index < history_get_count() - 1) {
        history_index++;
        history_get(history_index, &history_display_data);
        update_display();
    }
}

void task_history_next(void)
{
    if(display_mode == DISPLAY_MODE_HISTORY && history_index > 0) {
        history_index--;
        history_get(history_index, &history_display_data);
        update_display();
    }
}

void task_short_press_sw1(void)
{
    device_on = !device_on;
    if(device_on) {
        debug_printf(EVAL_COM0, "Thermometer ON\r\n");
        s2_data_t s2_data = s2_read_sht35(s2_dev.periph, SHT35_ADDR);
        current_data.temperature = s2_data.temperature;
        current_data.humidity = s2_data.humidity;
        history_push(&current_data);
    } else {
        debug_printf(EVAL_COM0, "Thermometer OFF\r\n");
    }
    update_display();
}

void task_long_press_sw1(void)
{
    if(display_mode == DISPLAY_MODE_NORMAL) {
        display_mode = DISPLAY_MODE_HISTORY;
        history_index = 0;
        history_get(history_index, &history_display_data);
        gd_eval_led_on(LED1);
        debug_printf(EVAL_COM0, "Enter HISTORY mode\r\n");
    } else {
        display_mode = DISPLAY_MODE_NORMAL;
        gd_eval_led_off(LED1);
        debug_printf(EVAL_COM0, "Exit HISTORY mode\r\n");
    }
    update_display();
}

// 按键扫描（长按检测）
void task_key_scan_with_long_press(void)
{
    static uint8_t last_key = KEY_NONE;
    static uint16_t key_press_counter = 0;
    static uint8_t key_handled = 0;
    
    uint8_t key = s1_key_scan(s1_dev.periph, s1_dev.addr);
    
    if(key != KEY_NONE && key == last_key) {
        if(!key_handled && key_press_counter++ >= LONG_PRESS_TIME_MS / 20) {
            if(key == KEY_MODE_TOGGLE) {
                task_long_press_sw1();
                key_handled = 1;
            }
            key_press_counter = LONG_PRESS_TIME_MS / 20 + 1;
        }
    } else if(key != KEY_NONE && key != last_key) {
        key_press_counter = 0;
        key_handled = 0;
        last_key = key;
    } else if(key == KEY_NONE && last_key != KEY_NONE) {
        if(!key_handled && key_press_counter < LONG_PRESS_TIME_MS / 20) {
            if(last_key == KEY_MODE_TOGGLE) {
                task_short_press_sw1();
            } else if(last_key == KEY_HISTORY_PREV) {
                task_history_prev();
            } else if(last_key == KEY_HISTORY_NEXT) {
                task_history_next();
            }
        }
        last_key = KEY_NONE;
        key_handled = 0;
        key_press_counter = 0;
    }
}

// 供外部获取当前温湿度
s8_para* task_get_current_data(void)
{
    return &current_data;
}