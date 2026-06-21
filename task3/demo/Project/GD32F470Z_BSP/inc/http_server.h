#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "queue.h"

#define HTTP_REQ_ROOT       0
#define HTTP_REQ_HISTORY    1
#define HTTP_REQ_DATA       2
#define HTTP_REQ_UNKNOWN    3

int http_parse_request(const char *request);
void http_build_response_html(char *response, int req_type, 
                               float temp, float humi,
                               s8_para *history, int history_count);

#endif