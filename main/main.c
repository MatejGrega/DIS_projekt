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
//#include "led_strip.h"

//custom files
#include "hw_init.h"
#include "lvgl_graphics.h"


// buffer size for recording and playing audio
#define REC_PLAY_I2S_BUFF_SIZE   (2400)

// LCD related definitions
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


static const char err_reason[][30] = {"input param is invalid",
                                      "operation timeout"
                                     };
static i2s_chan_handle_t tx_handle = NULL;
static i2s_chan_handle_t rx_handle = NULL;


static void i2s_echo(void *args)
{
    int16_t *mic_data = malloc(2*REC_PLAY_I2S_BUFF_SIZE);
    int16_t *spkr_data = malloc(2*REC_PLAY_I2S_BUFF_SIZE);
    bool PCM_printed = false;
    if (!mic_data) {
        ESP_LOGE(TAG, "[echo] No memory for read data buffer");
        abort();
    }
    if (!spkr_data) {
        ESP_LOGE(TAG, "[echo] No memory for write data buffer");
        abort();
    }
    esp_err_t ret = ESP_OK;
    size_t bytes_read = 0;
    size_t bytes_write = 0;
    ESP_LOGI(TAG, "[echo] Echo start");

    for(uint16_t i = 0; i < REC_PLAY_I2S_BUFF_SIZE; i++){
        if((i % 20) < 10){
            spkr_data[i] = 5000;
        }
        else{
            spkr_data[i] = -5000;
        }
    }

    while (1) {
        memset(mic_data, 0, REC_PLAY_I2S_BUFF_SIZE);
        /* Read sample data from mic */
        ret = i2s_channel_read(rx_handle, mic_data, REC_PLAY_I2S_BUFF_SIZE, &bytes_read, 1000);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[echo] i2s read failed, %s", err_reason[ret == ESP_ERR_TIMEOUT]);
            abort();
        }
        /* Write sample data to earphone */
        ret = i2s_channel_write(tx_handle, spkr_data, REC_PLAY_I2S_BUFF_SIZE, &bytes_write, 1000);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[echo] i2s write failed, %s", err_reason[ret == ESP_ERR_TIMEOUT]);
            abort();
        }
        if (bytes_read != bytes_write) {
            ESP_LOGW(TAG, "[echo] %d bytes read but only %d bytes are written", bytes_read, bytes_write);
        }

        if((esp_timer_get_time() > (2 * 1000 * 1000)) && !PCM_printed){
            PCM_printed = true;
            for(uint32_t i = 0; i < REC_PLAY_I2S_BUFF_SIZE; i++){
                ESP_LOGI(TAG, "%d", mic_data[i]);
            }
        }
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    hw_init_audio(&tx_handle, &rx_handle);
    hw_init_lcd(display);
    hw_init_LED_RGB(&led_strip);
    hw_init_buttons();

    ESP_LOGI(TAG, "Create LVGL task");
    xTaskCreate(lvgl_port_task, "LVGL", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL);

    xTaskCreate(i2s_echo, "i2s_echo", 8192, NULL, 5, NULL);


    led_strip_set_pixel(led_strip, 0, 16, 1, 16);
    led_strip_refresh(led_strip);

    uint32_t i = 0;
    while(1){
        _lock_acquire(&lvgl_api_lock);
        display_graphics(display, i++);
        _lock_release(&lvgl_api_lock);

        usleep(250 * 1000);
    }
}