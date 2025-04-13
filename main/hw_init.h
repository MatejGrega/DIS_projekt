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

#include "driver/adc.h"
#include "esp_adc/adc_oneshot.h"

#include <string.h>
#include "sdkconfig.h"
#include "driver/i2s_std.h"
#include "esp_system.h"
#include "esp_check.h"
#include "es8311.h"

// The pixel number in horizontal and vertical
#define LCD_H_RES              320
#define LCD_V_RES              240

//button states on dev kit
typedef enum{
    BUTTON_NONE,
    BUTTON_K1,
    BUTTON_K2,
    BUTTON_K3,
    BUTTON_K4,
    BUTTON_K5,
    BUTTON_K6,
} button_adc_t;

void hw_init_lcd(lv_display_t *display);
lv_obj_t* hw_init_get_canvas(void);
lv_color_t* hw_init_get_canvas_buffer(void);

void hw_init_LED_RGB(led_strip_handle_t *led_strip_p);

void hw_init_buttons(void);
button_adc_t hw_get_buttons(void);

esp_err_t es8311_codec_init(void);
esp_err_t i2s_driver_init(i2s_chan_handle_t *tx_handle, i2s_chan_handle_t *rx_handle);


#endif