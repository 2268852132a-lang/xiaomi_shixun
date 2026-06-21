#include "gd32f4xx.h"
#include "gd32f470z_eval.h"
#include "s1.h"
#include "s2.h"
#include "e1.h"
#include "c3.h"
#include "http_server.h"
#include "../inc/task_config.h"
#include "task_control.h"
#include <stdio.h>
#include <string.h>

// ==================== 全局变量 ====================
static volatile uint16_t delay_count = 0;
static volatile uint16_t time_count = 0;
static volatile uint8_t second_flag = 0;

// I2C 设备结构体
i2c_addr_def s1_dev;
i2c_addr_def s2_dev;
i2c_addr_def e1_dev;

// 调试打印缓冲区
uint8_t print_buffer[100];

// 当前温湿度数据（供 HTTP 服务器使用）
extern s8_para current_data;

// ==================== 函数声明 ====================
void timer3_init(void);
void delay_ms(uint16_t mstime);
void debug_printf(uint32_t usart_periph, char *string);
static void http_server_task(void);
static void i2c_scan(void);

// ==================== I2C 扫描 ====================
static void i2c_scan(void)
{
    debug_printf(EVAL_COM0, "Scanning I2C bus...\r\n");
    for(uint8_t addr = 0x08; addr < 0x80; addr++) {
        if(i2c_addr_poll(I2C0, addr)) {
            char buf[50];
            sprintf((char*)buf, "  Found 0x%02X on I2C0\r\n", addr);
            debug_printf(EVAL_COM0, buf);
        }
        if(i2c_addr_poll(I2C1, addr)) {
            char buf[50];
            sprintf((char*)buf, "  Found 0x%02X on I2C1\r\n", addr);
            debug_printf(EVAL_COM0, buf);
        }
    }
    debug_printf(EVAL_COM0, "I2C scan complete.\r\n");
}

// ==================== HTTP 服务器任务 ====================
static void http_server_task(void)
{
    int client = c3_check_client();
    
    if(client != -1) {
        char http_request[512];
        if(c3_receive_http(http_request, sizeof(http_request))) {
            int req_type = http_parse_request(http_request);
            char response[2048];
            http_build_response_html(response, req_type,
                                      current_data.temperature,
                                      current_data.humidity,
                                      NULL,
                                      history_get_count());
            c3_send_http_response(response);
        }
        c3_close_client();
    }
}

// ==================== 主函数 ====================
int main(void)
{
    char tmp_buffer[100];
    
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
    gd_eval_com_init(EVAL_COM0);
    timer3_init();
    init_i2c();
    gd_eval_led_init(LED1);
    
    // ========== I2C 扫描（调试） ==========
    i2c_scan();
    
    // 初始化历史记录队列
    history_clear();

    debug_printf(EVAL_COM0, "=== Smart Thermometer (task3) ===\r\n");

    // 初始化 S1 按键
    s1_dev = s1_init(HT16K33_ADDRESS_S1);
    if(!s1_dev.flag) {
        debug_printf(EVAL_COM0, "ERROR: S1 not found!\r\n");
        while(1);
    }
    debug_printf(EVAL_COM0, "S1 keypad found\r\n");

    // 初始化 S2 光照+温湿度合体传感器
    s2_dev = s2_init(S1_ADDRESS_S2);
    if(!s2_dev.flag) {
        debug_printf(EVAL_COM0, "WARNING: S2 not found!\r\n");
    } else {
        debug_printf(EVAL_COM0, "S2 sensor found\r\n");
        s2_sht35_init(s2_dev.periph, 0x88);
        debug_printf(EVAL_COM0, "SHT35 initialized\r\n");
    }

    // 初始化 E1 数码管 + 强制测试
    e1_dev = e1_init(0xE0);
    if(!e1_dev.flag) {
        debug_printf(EVAL_COM0, "WARNING: E1 not found!\r\n");
    } else {
        debug_printf(EVAL_COM0, "E1 display found\r\n");
        // 强制显示 8888 测试
        e1_digital_display(e1_dev.periph, e1_dev.addr, 8, 8, 8, 8);
        debug_printf(EVAL_COM0, "E1 test: 8888 should be displayed\r\n");
        delay_ms(2000);  // 显示2秒
    }

    // 初始化 C3 WiFi 模块
    if(c3_init()) {
        debug_printf(EVAL_COM0, "C3 module OK\r\n");
        if(c3_connect_wifi()) {
            debug_printf(EVAL_COM0, "WiFi connected\r\n");
            if(c3_start_server()) {
                sprintf(tmp_buffer, "HTTP server started on port %s\r\n", SERVER_PORT);
                debug_printf(EVAL_COM0, tmp_buffer);
                sprintf(tmp_buffer, "Visit http://<U1P_IP>:%s in browser\r\n", SERVER_PORT);
                debug_printf(EVAL_COM0, tmp_buffer);
            } else {
                debug_printf(EVAL_COM0, "WARNING: Failed to start server\r\n");
            }
        } else {
            debug_printf(EVAL_COM0, "WARNING: WiFi connection failed\r\n");
        }
    } else {
        debug_printf(EVAL_COM0, "WARNING: C3 not found!\r\n");
    }

    debug_printf(EVAL_COM0, "System ready.\r\n");

    while(1)
    {
        if(second_flag) {
            second_flag = 0;
            task_normal_update();
        }
        
        task_key_scan_with_long_press();
        http_server_task();
    }
}

// ==================== 定时器初始化 ====================
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
    buffer[len] = '\0';
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