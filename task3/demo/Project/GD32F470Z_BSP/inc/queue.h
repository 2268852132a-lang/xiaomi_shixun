#ifndef QUEUE_H
#define QUEUE_H

#include <stdint.h>
#include <stddef.h>

// 温湿度数据结构（与 s2.c 中的 s2_data_t 兼容）
typedef struct {
    float temperature;
    float humidity;
} s8_para;

#define HISTORY_SIZE    100

void history_push(s8_para *data);
uint8_t history_get(uint16_t index, s8_para *data);
uint16_t history_get_count(void);
void history_clear(void);
uint16_t history_get_count(void);

#endif