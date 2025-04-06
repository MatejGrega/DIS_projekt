/*
    Header file - hardware initialization
*/
#ifndef _HW_INIT_H_
#define _HW_INIT_H_

#include <stdio.h>
#include <unistd.h>
#include <sys/lock.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_lcd_ili9341.h"

#include "led_strip.h"

// The pixel number in horizontal and vertical
#define LCD_H_RES              320
#define LCD_V_RES              240

void hw_init_lcd(lv_display_t *display);
lv_obj_t* hw_init_get_canvas(void);
lv_color_t* hw_init_get_canvas_buffer(void);

void hw_init_LED_RGB(led_strip_handle_t *led_strip_p);

#endif