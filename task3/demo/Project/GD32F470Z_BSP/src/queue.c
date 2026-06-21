#include "queue.h"
#include <string.h>

static s8_para buffer[HISTORY_SIZE];
static uint16_t head = 0;
static uint16_t count = 0;

void history_push(s8_para *data)
{
    buffer[head] = *data;
    head = (head + 1) % HISTORY_SIZE;
    if(count < HISTORY_SIZE) count++;
}

uint8_t history_get(uint16_t index, s8_para *data)
{
    if(index >= count) return 0;
    uint16_t pos = (head - 1 - index + HISTORY_SIZE) % HISTORY_SIZE;
    *data = buffer[pos];
    return 1;
}

uint16_t history_get_count(void)
{
    return count;
}

void history_clear(void)
{
    head = 0;
    count = 0;
    memset(buffer, 0, sizeof(buffer));
}