#include "task_control.h"
#include "task_config.h"
#include "e1.h"
#include "e2.h"
#include "s6.h"
#include "s7.h"
#include "s8.h"
#include "gd32f470z_eval.h"
#include <stdio.h>

// ==================== 外部变量 ====================
extern i2c_addr_def e1_dev;
extern i2c_addr_def e2_dev;
extern i2c_addr_def s6_dev;
extern i2c_addr_def s7_dev;
extern i2c_addr_def s8_dev;
extern void debug_printf(uint32_t usart_periph, char *string);

// ==================== 系统状态 ====================
static uint8_t device_on = 0;
static uint8_t fan_speed = 50;
static uint8_t display_mode = DISPLAY_MODE_NORMAL;
static uint8_t display_timer = 0;
static uint8_t control_mode = CTRL_MODE_MANUAL;
static uint8_t temp_phase = TEMP_PHASE_INIT;
static uint8_t target_temp = 25;
static float current_temp = 25.0f;

// ==================== 本地函数 ====================
static void update_display(void);
static void power_off(void);
static void power_on(void);
static void handle_manual_speed_change(int8_t delta);
static void set_fan_and_log(uint8_t speed);

static void set_fan_and_log(uint8_t speed)
{
    if(speed > 100) speed = 100;
    e2_speed_control(e2_dev.periph, e2_dev.addr, speed);
    char buf[40];
    sprintf(buf, "Fan speed: %d%%\r\n", speed);
    debug_printf(EVAL_COM0, buf);
}

static void power_on(void)
{
    device_on = 1;
    fan_speed = 50;
    control_mode = CTRL_MODE_MANUAL;
    temp_phase = TEMP_PHASE_INIT;
    display_mode = DISPLAY_MODE_NORMAL;
    display_timer = 0;
    e2_speed_control(e2_dev.periph, e2_dev.addr, fan_speed);
    e1_rgb_control(e1_dev.periph, e1_dev.addr, 0, 255, 0);
    update_display();
    debug_printf(EVAL_COM0, "Power ON, fan 50%%, RGB green\r\n");
}

static void power_off(void)
{
    device_on = 0;
    e2_speed_control(e2_dev.periph, e2_dev.addr, 0);
    e1_rgb_control(e1_dev.periph, e1_dev.addr, 0, 0, 0);
    update_display();
    debug_printf(EVAL_COM0, "Power OFF\r\n");
}

static void handle_manual_speed_change(int8_t delta)
{
    int16_t new_speed = (int16_t)fan_speed + delta;
    if(new_speed < 0)   new_speed = 0;
    if(new_speed > 100) new_speed = 100;
    fan_speed = (uint8_t)new_speed;
    set_fan_and_log(fan_speed);
}

static void update_display(void)
{
    if(!e1_dev.flag) return;

    if(!device_on) {
        e1_digital_display(e1_dev.periph, e1_dev.addr, NODIS, NODIS, NODIS, NODIS);
        return;
    }

    if(control_mode == CTRL_MODE_TEMP && temp_phase == TEMP_PHASE_INIT) {
        e1_digital_display(e1_dev.periph, e1_dev.addr,
                           0, 0, target_temp / 10, target_temp % 10);
        return;
    }

    if(display_mode == DISPLAY_MODE_CONTINUOUS || display_timer > 0) {
        e1_digital_display(e1_dev.periph, e1_dev.addr,
                           NODIS, NODIS, fan_speed / 10, fan_speed % 10);
        return;
    }

    if(control_mode == CTRL_MODE_TEMP && temp_phase == TEMP_PHASE_CONTROL) {
        uint8_t t = (uint8_t)(current_temp + 0.5f);
        e1_digital_display(e1_dev.periph, e1_dev.addr,
                           0x0C, NODIS, t / 10, t % 10);
        return;
    }

    e1_digital_display(e1_dev.periph, e1_dev.addr, NODIS, NODIS, NODIS, NODIS);
}

static void task_short_press(uint8_t key)
{
    switch(key) {

    case KEY_POWER_TOGGLE:
        if(device_on) power_off();
        else power_on();
        break;

    case KEY_SPEED_UP:
        if(!device_on) break;
        if(control_mode == CTRL_MODE_TEMP && temp_phase == TEMP_PHASE_INIT) {
            if(target_temp < 50) target_temp++;
            update_display();
            break;
        }
        if(control_mode != CTRL_MODE_MANUAL) break;
        handle_manual_speed_change(10);
        update_display();
        break;

    case KEY_SPEED_DOWN:
        if(!device_on) break;
        if(control_mode == CTRL_MODE_TEMP && temp_phase == TEMP_PHASE_INIT) {
            if(target_temp > 0) target_temp--;
            update_display();
            break;
        }
        if(control_mode != CTRL_MODE_MANUAL) break;
        handle_manual_speed_change(-10);
        update_display();
        break;

    case KEY_DISPLAY:
        if(!device_on) break;
        display_timer = 3;
        update_display();
        debug_printf(EVAL_COM0, "Show speed: 3s\r\n");
        break;

    case KEY_DISTANCE_MODE:
        if(!device_on) break;
        if(control_mode == CTRL_MODE_DISTANCE) {
            control_mode = CTRL_MODE_MANUAL;
            debug_printf(EVAL_COM0, "Exit distance mode\r\n");
        } else {
            control_mode = CTRL_MODE_DISTANCE;
            temp_phase = TEMP_PHASE_INIT;
            debug_printf(EVAL_COM0, "Enter distance mode\r\n");
        }
        update_display();
        break;

    case KEY_TEMP_MODE:
        if(!device_on) break;
        if(control_mode == CTRL_MODE_TEMP) {
            control_mode = CTRL_MODE_MANUAL;
            temp_phase = TEMP_PHASE_INIT;
            debug_printf(EVAL_COM0, "Exit temp control mode\r\n");
        } else {
            control_mode = CTRL_MODE_TEMP;
            temp_phase = TEMP_PHASE_INIT;
            target_temp = 25;
            debug_printf(EVAL_COM0, "Enter temp control mode\r\n");
        }
        update_display();
        break;

    case KEY_TEMP_CONFIRM:
        if(!device_on) break;
        if(control_mode == CTRL_MODE_TEMP) {
            if(temp_phase == TEMP_PHASE_INIT) {
                temp_phase = TEMP_PHASE_CONTROL;
                debug_printf(EVAL_COM0, "Temp target set to %d, enter control\r\n", target_temp);
            } else {
                temp_phase = TEMP_PHASE_INIT;
                e2_speed_control(e2_dev.periph, e2_dev.addr, 0);
                debug_printf(EVAL_COM0, "Exit temp control, back to input\r\n");
            }
            update_display();
        }
        break;

    default:
        break;
    }
}

static void task_long_press(uint8_t key)
{
    if(key == KEY_DISPLAY) {
        if(!device_on) return;
        if(display_mode == DISPLAY_MODE_CONTINUOUS) {
            display_mode = DISPLAY_MODE_NORMAL;
            debug_printf(EVAL_COM0, "Continuous display OFF\r\n");
        } else {
            display_mode = DISPLAY_MODE_CONTINUOUS;
            debug_printf(EVAL_COM0, "Continuous display ON\r\n");
        }
        display_timer = 0;
        update_display();
    }
}

void task_key_scan_100ms(void)
{
    static uint8_t last_key = KEY_NONE;
    static uint8_t press_count = 0;
    static uint8_t key_handled = 0;

    uint8_t key = s1_key_scan(s1_dev.periph, s1_dev.addr);

    if(key != KEY_NONE && key == last_key) {
        if(!key_handled && ++press_count >= LONG_PRESS_SCAN_COUNT) {
            if(key == KEY_DISPLAY) {
                task_long_press(key);
                key_handled = 1;
            }
            press_count = LONG_PRESS_SCAN_COUNT;
        }
    } else if(key != KEY_NONE && key != last_key) {
        press_count = 0;
        key_handled = 0;
        last_key = key;
    } else if(key == KEY_NONE && last_key != KEY_NONE) {
        if(!key_handled) {
            task_short_press(last_key);
        }
        last_key = KEY_NONE;
        key_handled = 0;
        press_count = 0;
    }
}

void task_1s_update(void)
{
    if(!device_on) return;

    if(display_timer > 0) {
        display_timer--;
        if(display_timer == 0) {
            update_display();
        }
    }

    // 距离自适应模式
    if(control_mode == CTRL_MODE_DISTANCE && s6_dev.flag) {
        uint8_t dist_buf[2];
        if(s6_read_distance(s6_dev.periph, s6_dev.addr, dist_buf)) {
            uint16_t distance = ((uint16_t)dist_buf[0] << 8) | dist_buf[1];
            uint8_t new_speed;
            if(distance <= 40)          new_speed = 10;
            else if(distance <= 80)     new_speed = 30;
            else                        new_speed = 50;

            if(new_speed != fan_speed) {
                fan_speed = new_speed;
                e2_speed_control(e2_dev.periph, e2_dev.addr, fan_speed);
            }
        }
    }

    // 温控模式 - 控制阶段
    if(control_mode == CTRL_MODE_TEMP && temp_phase == TEMP_PHASE_CONTROL && s8_dev.flag) {
        s8_para data = s8_read_sht3x(s8_dev.periph, S8_SHT30_ADDR);
        current_temp = data.temperature;

        uint8_t human_present = 0;
        if(s7_dev.flag) {
            human_present = s7_get_status(s7_dev.periph, s7_dev.addr);
        }

        if(current_temp < (float)target_temp) {
            e2_speed_control(e2_dev.periph, e2_dev.addr, 0);
            debug_printf(EVAL_COM0, "Temp below target, fan off\r\n");
        } else if(human_present) {
            uint8_t diff = (uint8_t)(current_temp - target_temp);
            uint8_t new_speed;
            if(diff <= 2)       new_speed = 10;
            else if(diff <= 4)  new_speed = 30;
            else                new_speed = 50;

            fan_speed = new_speed;
            e2_speed_control(e2_dev.periph, e2_dev.addr, fan_speed);
        } else {
            e2_speed_control(e2_dev.periph, e2_dev.addr, 0);
        }

        update_display();
    }
}
