/*
    DIS project
*/

//system files
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

//custom files
#include "hw_init.h"
#include "lvgl_graphics.h"

#define LVGL_TASK_MAX_DELAY_MS 500
#define LVGL_TASK_STACK_SIZE   (4 * 1024)
#define LVGL_TASK_PRIORITY     2

static const char *TAG = "DIS_projekt";

// LCD object for LVGL library
lv_display_t *display;
// LVGL library is not thread-safe, so use a mutex to protect calling LVGL APIs from different tasks
static _lock_t lvgl_api_lock;

static led_strip_handle_t led_strip;

//LVGL task
static void lvgl_port_task(void *arg){
    ESP_LOGI(TAG, "Starting LVGL task");
    uint32_t time_till_next_ms = 0;
    uint32_t time_threshold_ms = 1000 / CONFIG_FREERTOS_HZ;
    while (1) {
        _lock_acquire(&lvgl_api_lock);
        time_till_next_ms = lv_timer_handler();
        _lock_release(&lvgl_api_lock);
        // in case of triggering a task watch dog time out
        time_till_next_ms = MAX(time_till_next_ms, time_threshold_ms);
        usleep(1000 * time_till_next_ms);
    }
}

void app_main(void)
{
    hw_init_lcd(display);
    hw_init_LED_RGB(&led_strip);
    hw_init_buttons();

    ESP_LOGI(TAG, "Create LVGL task");
    xTaskCreate(lvgl_port_task, "LVGL", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL);


    led_strip_set_pixel(led_strip, 0, 16, 1, 16);
    led_strip_refresh(led_strip);






    




    uint32_t i = 0;
    while(1){
        _lock_acquire(&lvgl_api_lock);
        display_graphics(display, i++);
        _lock_release(&lvgl_api_lock);

        sleep(1);
    }
}