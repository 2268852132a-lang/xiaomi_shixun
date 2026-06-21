#include "http_server.h"
#include <string.h>
#include <stdio.h>

/*********************************************************************************************
函数名:    http_parse_request
功能:      解析 HTTP 请求，判断请求路径
返回值:    0=根路径(实时数据), 1=历史数据, 2=其他
**********************************************************************************************/
int http_parse_request(const char *request)
{
    // 查找请求行 "GET /xxx HTTP/1.1"
    if(strstr(request, "GET / ") != NULL || strstr(request, "GET /index.html") != NULL) {
        return HTTP_REQ_ROOT;      // 实时数据页面
    }
    else if(strstr(request, "GET /history") != NULL) {
        return HTTP_REQ_HISTORY;   // 历史数据页面
    }
    else if(strstr(request, "GET /data") != NULL) {
        return HTTP_REQ_DATA;      // JSON 数据接口
    }
    return HTTP_REQ_UNKNOWN;
}

/*********************************************************************************************
函数名:    http_build_response_html
功能:      构建 HTML 响应
**********************************************************************************************/
void http_build_response_html(char *response, int req_type, 
                               float temp, float humi,
                               s8_para *history, int history_count)
{
    if(req_type == HTTP_REQ_ROOT) {
        // 实时数据显示页面（自动刷新）
        sprintf(response,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Connection: close\r\n"
            "\r\n"
            "<!DOCTYPE html>"
            "<html><head><meta charset=UTF-8><meta http-equiv=refresh content=2>"
            "<title>智能温湿度计</title></head>"
            "<body style=text-align:center>"
            "<h1>🌡️ 智能温湿度计</h1>"
            "<h2>温度: %.1f °C</h2>"
            "<h2>湿度: %.1f %%</h2>"
            "<hr>"
            "<a href=/history>📊 查看历史记录</a>"
            "</body></html>",
            temp, humi);
    }
    else if(req_type == HTTP_REQ_HISTORY) {
        // 历史数据页面
        sprintf(response,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Connection: close\r\n"
            "\r\n"
            "<!DOCTYPE html>"
            "<html><head><meta charset=UTF-8>"
            "<title>历史记录</title></head>"
            "<body style=text-align:center>"
            "<h1>📊 历史温湿度记录</h1>"
            "<table border=1 style=margin:0 auto>"
            "<tr><th>序号</th><th>温度(°C)</th><th>湿度(%)</th></tr>");
        
        // 添加历史记录（最新10条）
        int start = history_count > 10 ? history_count - 10 : 0;
        for(int i = start; i < history_count; i++) {
            s8_para data;
            history_get(i, &data);
            char line[128];
            sprintf(line, "<tr><td>%d</td><td>%.1f</td><td>%.1f</td></tr>",
                    i + 1, data.temperature, data.humidity);
            strcat(response, line);
        }
        
        strcat(response,
            "</table><br>"
            "<a href=/>🏠 返回首页</a>"
            "</body></html>");
    }
    else if(req_type == HTTP_REQ_DATA) {
        // JSON 数据接口
        sprintf(response,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Connection: close\r\n"
            "\r\n"
            "{\"temperature\":%.1f,\"humidity\":%.1f}",
            temp, humi);
    }
}