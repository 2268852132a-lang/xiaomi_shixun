#include "uart.h"
#include "stdio.h"
#include "string.h"
#include "c3.h"

// ==================== UART 封装函数 ====================
static int Uart_write(char *sendbuf, int len)
{
    return uart_send_bytes(USART5, (uint8_t *)sendbuf, len);
}

static int Uart_read(char *recvbuf, int len, int timeout)
{
    return uart_rece_bytes(USART5, (uint8_t *)recvbuf, len, timeout);
}

// ==================== C3 功能函数 ====================

/*********************************************************************************************
函数名:    c3_init
功能:      初始化 C3 模块，检测 AT 指令
**********************************************************************************************/
uint8_t c3_init(void)
{
    int i;
    int recv_cnt = 0;
    char recv_buf[64] = {0};
    
    for(i = 0; i < 5; i++) {
        Uart_write("AT\r\n", strlen("AT\r\n"));
        recv_cnt += Uart_read(&recv_buf[recv_cnt], sizeof(recv_buf) - recv_cnt, 100);
        if(strstr(recv_buf, "OK") != NULL) {
            return 1;
        }
    }
    return 0;
}

/*********************************************************************************************
函数名:    c3_connect_wifi
功能:      连接 WiFi（Station 模式）
**********************************************************************************************/
uint8_t c3_connect_wifi(void)
{
    char recv_buf[128];
    int recv_cnt;
    char cmd[128];
    
    // 设置 Station 模式
    Uart_write("AT+CWMODE=1\r\n", strlen("AT+CWMODE=1\r\n"));
    delay_ms(500);
    
    // 连接 WiFi
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PASSWORD);
    Uart_write(cmd, strlen(cmd));
    delay_ms(5000);
    
    // 查询 IP
    Uart_write("AT+CIFSR\r\n", strlen("AT+CIFSR\r\n"));
    recv_cnt = Uart_read(recv_buf, sizeof(recv_buf), 2000);
    
    if(strstr(recv_buf, "OK") != NULL) {
        return 1;
    }
    return 0;
}

/*********************************************************************************************
函数名:    c3_start_server
功能:      启动 TCP Server
**********************************************************************************************/
uint8_t c3_start_server(void)
{
    char cmd[64];
    char recv_buf[64];
    
    // 启用多连接模式
    Uart_write("AT+CIPMUX=1\r\n", strlen("AT+CIPMUX=1\r\n"));
    delay_ms(500);
    Uart_read(recv_buf, sizeof(recv_buf), 500);
    
    // 启动 Server
    sprintf(cmd, "AT+CIPSERVER=1,%s\r\n", SERVER_PORT);
    Uart_write(cmd, strlen(cmd));
    delay_ms(500);
    Uart_read(recv_buf, sizeof(recv_buf), 500);
    
    if(strstr(recv_buf, "OK") != NULL) {
        return 1;
    }
    return 0;
}

/*********************************************************************************************
函数名:    c3_check_client
功能:      检查是否有客户端连接
返回值:    客户端 ID（0-4），-1 表示无连接
**********************************************************************************************/
int c3_check_client(void)
{
    char recv_buf[64];
    int recv_cnt;
    
    recv_cnt = Uart_read(recv_buf, sizeof(recv_buf), 100);
    if(recv_cnt > 0) {
        char *p = strstr(recv_buf, "+IPD,");
        if(p != NULL) {
            int client_id;
            sscanf(p, "+IPD,%d,", &client_id);
            return client_id;
        }
    }
    return -1;
}

/*********************************************************************************************
函数名:    c3_receive_http
功能:      接收 HTTP 请求内容
**********************************************************************************************/
uint8_t c3_receive_http(char *buffer, uint16_t buffer_size)
{
    char recv_buf[512];
    int recv_cnt;
    int client_id;
    int data_len;
    
    recv_cnt = Uart_read(recv_buf, sizeof(recv_buf), 500);
    if(recv_cnt > 0) {
        char *p = strstr(recv_buf, "+IPD,");
        if(p != NULL) {
            sscanf(p, "+IPD,%d,%d:", &client_id, &data_len);
            char *data_start = strchr(p, ':');
            if(data_start != NULL) {
                data_start++;
                int copy_len = data_len < buffer_size ? data_len : buffer_size - 1;
                memcpy(buffer, data_start, copy_len);
                buffer[copy_len] = '\0';
                return 1;
            }
        }
    }
    return 0;
}

/*********************************************************************************************
函数名:    c3_send_http_response
功能:      发送 HTTP 响应
**********************************************************************************************/
uint8_t c3_send_http_response(const char *response)
{
    char send_buffer[200];
    char recv_buf[32];
    uint16_t len = strlen(response);
    
    sprintf(send_buffer, "AT+CIPSEND=0,%d\r\n", len);
    Uart_write(send_buffer, strlen(send_buffer));
    delay_ms(200);
    
    Uart_write((char*)response, len);
    delay_ms(500);
    
    Uart_read(recv_buf, sizeof(recv_buf), 500);
    if(strstr(recv_buf, "SEND OK") != NULL) {
        return 1;
    }
    return 0;
}

/*********************************************************************************************
函数名:    c3_close_client
功能:      关闭客户端连接
**********************************************************************************************/
void c3_close_client(void)
{
    Uart_write("AT+CIPCLOSE=0\r\n", strlen("AT+CIPCLOSE=0\r\n"));
    delay_ms(200);
}

// ==================== 保留旧函数（兼容性）====================
int c3_wifi_tcp_init(int wifi_run_state, char *rec_data)
{
    return -1;
}

void c3_wifi_tcp_lead(int *wifi_run_state, int *tcp_status)
{
}

uint8_t c3_wifi_tcp_send(char *send_data)
{
    return 0;
}

uint16_t c3_wifi_tcp_receive(char *rec_data, uint16_t wait_time)
{
    return 0;
}