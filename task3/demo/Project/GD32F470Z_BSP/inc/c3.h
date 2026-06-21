#ifndef C3_H
#define C3_H

#include "gd32f4xx.h"      // 提供 uint16_t、uint8_t 等类型
#include "uart.h"

#define out

// WiFi 配置（连接到手机热点）
#define WIFI_SSID       "Xiaomi_AIoT"
#define WIFI_PASSWORD   "AIoT@31415926"

// TCP Server 配置
#define SERVER_PORT     "8080"      // 监听端口

// 函数声明
uint8_t c3_init(void);
uint8_t c3_connect_wifi(void);
uint8_t c3_start_server(void);
int c3_check_client(void);
uint8_t c3_receive_http(char *buffer, uint16_t buffer_size);
uint8_t c3_send_http_response(const char *response);
void c3_close_client(void);

// 保留旧函数（兼容性，可选）
int c3_wifi_tcp_init(int wifi_run_state, char *rec_data);
void c3_wifi_tcp_lead(int *wifi_run_state, int *tcp_status);
uint8_t c3_wifi_tcp_send(char *send_data);
uint16_t c3_wifi_tcp_receive(char *rec_data, uint16_t wait_time);

#endif