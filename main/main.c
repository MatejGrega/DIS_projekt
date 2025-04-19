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
#include <stdlib.h>
#include <string.h>
#include "esp_system.h"
#include "soc/gpio_struct.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"
#include <math.h>

#include "esp_dsp.h"
//#include "led_strip.h"

//custom files
#include "common.h"
#include "hw_init.h"
#include "lvgl_graphics.h"

// buffer size for recording and playing audio
#define MIC_I2S_BUFF_SIZE   (2400)
#define SPKR_I2S_BUFF_SIZE  (32000)

// LCD related definitions
#define LVGL_TASK_MAX_DELAY_MS  500
#define LVGL_TASK_STACK_SIZE    (4 * 1024)
#define LVGL_TASK_PRIORITY      2

#define RGB_LED_RED_MAX_INTENSITY   80
#define RGB_LED_GREEN_MAX_INTENSITY   35

static const char *TAG = "DIS_projekt";

// LCD object for LVGL library
lv_display_t *display;
// LVGL library is not thread-safe, so use a mutex to protect calling LVGL APIs from different tasks
static _lock_t lvgl_api_lock;

static led_strip_handle_t led_strip;

static i2s_chan_handle_t tx_handle = NULL;
static i2s_chan_handle_t rx_handle = NULL;

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

// task for sending data to speaker
static void spkr_task(void *args)
{
    // allocate RAM for buffers holding samples of output signal
    int16_t *spkr_data = malloc(SPKR_I2S_BUFF_SIZE * sizeof(int16_t));
    float *sig_data = malloc(SPKR_I2S_BUFF_SIZE * sizeof(float));

    // values from last cycle: volume value, state of playing (on/off) and set output frequency
    uint8_t volume_last = 0;
    bool playing_last = !get_playing();
    uint16_t audio_freq_last = 0;

    // test memory allocation
    if (!spkr_data) {
        ESP_LOGE(TAG, "Memory for speaker data allocation error.");
        abort();
    }
    esp_err_t err_ret = ESP_OK;
    size_t bytes_write = 0;     //number of bytes send to codec

    while (1) {
        uint16_t audio_freq = get_audio_frequency();

        //compute new samples of output signal if set frequency was changed
        if(audio_freq != audio_freq_last){
            audio_freq_last = audio_freq;
            //generate sine wave
            dsps_tone_gen_f32(sig_data, SPKR_I2S_BUFF_SIZE, 1000, (float)audio_freq/((float)AUDIO_SAMPLE_RATE * 2.0), 0);
            if(get_playing()){
                //copy signal samples from intermediate buffer to speaker buffer
                for(uint16_t i = 0; i < SPKR_I2S_BUFF_SIZE; i++){
                    spkr_data[i] = (int16_t)sig_data[i];
                }
            }
        }
        
        //mute or unmute speaker if needed
        uint8_t playing_new = get_playing();
        bool actualize_volume = false;
        if(playing_last != playing_new){
            playing_last = playing_new;
            if(playing_new == false){
                //fill output buffer with zeros
                memset(spkr_data, 0, SPKR_I2S_BUFF_SIZE * sizeof(int16_t));
            }
            else{
                actualize_volume = true;    //flag that cause refilling output buffer with signal samples
            }
        }

        // recompute samples for speaker if volume or playing state has changed
        uint8_t volume_new = get_volume();
        if(((volume_last != volume_new) || actualize_volume) && playing_new){
            volume_last = volume_new;
            for(uint16_t i = 0; i < SPKR_I2S_BUFF_SIZE; i++){
                spkr_data[i] = (int16_t)(sig_data[i] * volume_new / VOLUME_MAX);    //adjust aplitude
            }
        }

        // send data to speaker
        err_ret = i2s_channel_write(tx_handle, spkr_data, SPKR_I2S_BUFF_SIZE * sizeof(int16_t), &bytes_write, 10000);
        if (err_ret != ESP_OK) {
            ESP_LOGE(TAG, "Sending data to speaker error.");
            abort();
        }
    }
    vTaskDelete(NULL);
}

// task for reading data from microphone and computing amplitude
static void mic_task(void *args){
    // allocate RAM for buffer holding samples of input signal
    int16_t *mic_data = malloc(MIC_I2S_BUFF_SIZE * sizeof(int16_t));
    esp_err_t err_ret = ESP_OK;
    size_t bytes_read = 0;

    // test memory allocation
    if (!mic_data) {
        ESP_LOGE(TAG, "Memory for microphone data allocation error.");
        abort();
    }

    while(1){
        // Read data from microphone
        err_ret = i2s_channel_read(rx_handle, mic_data, MIC_I2S_BUFF_SIZE * sizeof(int16_t), &bytes_read, 1000);
        if (err_ret != ESP_OK) {
            ESP_LOGE(TAG, "Reading data from microphone error.");
            abort();
        }

        //find minimum and maximum of input signal
        int16_t max_mic_val = -32000;
        int16_t min_mic_val = 32000;
        for(uint16_t i = 0; i < MIC_I2S_BUFF_SIZE; i++){
            if(mic_data[i] > max_mic_val){
                max_mic_val = mic_data[i];
            }
            if(mic_data[i] < min_mic_val){
                min_mic_val = mic_data[i];
            }
        }
        set_sound_level(max_mic_val - min_mic_val); //compute amplitude
    }
    vTaskDelete(NULL);
}

// RGB LED is signaling sound level - full green = silence, full red = high sound level
void RGB_LED_task(void *args){
    while(1){
        uint32_t red_val;
        uint32_t green_val;

        red_val = get_sound_level();
        red_val = red_val > SOUND_LEVEL_BAR_MAX_VAL ? SOUND_LEVEL_BAR_MAX_VAL : red_val;
        green_val = SOUND_LEVEL_BAR_MAX_VAL - red_val;
        red_val = red_val * RGB_LED_RED_MAX_INTENSITY / SOUND_LEVEL_BAR_MAX_VAL;
        green_val = green_val * RGB_LED_GREEN_MAX_INTENSITY / SOUND_LEVEL_BAR_MAX_VAL;
        led_strip_set_pixel(led_strip, 0, red_val, green_val, 0);
        led_strip_refresh(led_strip);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    //initialize hardware and submodules
    hw_init_audio(&tx_handle, &rx_handle);
    hw_init_lcd(display);
    hw_init_LED_RGB(&led_strip);
    hw_init_buttons();

    xTaskCreate(lvgl_port_task, "LVGL", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL);
    ui_init();
    xTaskCreate(spkr_task, "spkr_task", 8192, NULL, 5, NULL);
    xTaskCreate(mic_task, "mic_task", 4096, NULL, 4, NULL);
    xTaskCreate(RGB_LED_task, "RGB_LED_task", 2048, NULL, 0, NULL);


    while(1){
        if(get_lcd_update_flag()){
            _lock_acquire(&lvgl_api_lock);
            display_graphics(display);
            _lock_release(&lvgl_api_lock);
            reset_lcd_update_flag();
        }
        usleep(10 * 1000);
    }
}