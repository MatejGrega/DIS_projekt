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
#include "common.h"
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






#include <stdlib.h>
#include <string.h>
#include "esp_system.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"
#include <math.h>

#include "esp_dsp.h"

const float low_pass_coefs[] = {
    -0.001375616062081043,
    -0.001681932845199558,
    -0.002001418224072743,
    -0.002327833476324061,
    -0.002654043677172017,
    -0.002972086781993778,
    -0.003273264009542430,
    -0.003548250747225786,
    -0.003787226733313199,
    -0.003980023814881915,
    -0.004116289145086632,
    -0.004185661279072421,
    -0.004177956264250299,
    -0.004083360506746956,
    -0.003892626939769326,
    -0.003597270828446691,
    -0.003189761425214370,
    -0.002663705644347283,
    -0.002014019956653185,
    -0.001237086816769248,
    -0.000330892125450211,
    0.000704859504543284,
    0.001868654571781152,
    0.003157108086201147,
    0.004564930568294723,
    0.006084926153689202,
    0.007708022751750492,
    0.009423334618575361,
    0.011218257097468909,
    0.013078592665903825,
    0.014988706819547204,
    0.016931711733782703,
    0.018889675083595028,
    0.020843850885573320,
    0.022774928762230223,
    0.024663297628921062,
    0.026489319476253662,
    0.028233608673440165,
    0.029877312056391291,
    0.031402384992582341,
    0.032791858635076124,
    0.034030093690914809,
    0.035103016232811389,
    0.035998331374202143,
    0.036705711000949492,
    0.037216952201264218,
    0.037526103550118435,
    0.037629556975503305,
    0.037526103550118435,
    0.037216952201264218,
    0.036705711000949492,
    0.035998331374202143,
    0.035103016232811389,
    0.034030093690914809,
    0.032791858635076124,
    0.031402384992582341,
    0.029877312056391291,
    0.028233608673440165,
    0.026489319476253662,
    0.024663297628921062,
    0.022774928762230223,
    0.020843850885573320,
    0.018889675083595032,
    0.016931711733782703,
    0.014988706819547204,
    0.013078592665903825,
    0.011218257097468909,
    0.009423334618575361,
    0.007708022751750492,
    0.006084926153689202,
    0.004564930568294723,
    0.003157108086201147,
    0.001868654571781152,
    0.000704859504543284,
    -0.000330892125450211,
    -0.001237086816769248,
    -0.002014019956653185,
    -0.002663705644347283,
    -0.003189761425214370,
    -0.003597270828446690,
    -0.003892626939769326,
    -0.004083360506746956,
    -0.004177956264250299,
    -0.004185661279072421,
    -0.004116289145086634,
    -0.003980023814881915,
    -0.003787226733313199,
    -0.003548250747225788,
    -0.003273264009542430,
    -0.002972086781993778,
    -0.002654043677172016,
    -0.002327833476324061,
    -0.002001418224072743,
    -0.001681932845199558,
    -0.001375616062081043
};


static void i2s_echo(void *args)
{
    int16_t *mic_data = malloc(REC_PLAY_I2S_BUFF_SIZE * sizeof(int16_t));
    int16_t *spkr_data = malloc(REC_PLAY_I2S_BUFF_SIZE * sizeof(int16_t));
    float *sig_data = malloc(REC_PLAY_I2S_BUFF_SIZE * sizeof(float));
    bool PCM_printed = false;
    uint8_t volume_last = 0;
    bool playing_last = !get_playing();

    if (!mic_data) {
        ESP_LOGE(TAG, "[echo] No memory for read data buffer");
        abort();
    }
    if (!spkr_data) {
        ESP_LOGE(TAG, "[echo] No memory for write data buffer");
        abort();
    }
    esp_err_t err_ret = ESP_OK;
    size_t bytes_read = 0;
    size_t bytes_write = 0;
    ESP_LOGI(TAG, "[echo] Echo start");

    dsps_tone_gen_f32(sig_data, REC_PLAY_I2S_BUFF_SIZE, 1000, (float)400/((float)AUDIO_SAMPLE_RATE * 2), 0);
    for(uint16_t i = 0; i < REC_PLAY_I2S_BUFF_SIZE; i++){
       spkr_data[i] = 0;//(int16_t)sig_data[i];
    }

    while (1) {
        memset(mic_data, 0, REC_PLAY_I2S_BUFF_SIZE);
        // Read sample data from mic
        err_ret = i2s_channel_read(rx_handle, mic_data, REC_PLAY_I2S_BUFF_SIZE, &bytes_read, 1000);
        if (err_ret != ESP_OK) {
            ESP_LOGE(TAG, "Reading data from microphone error: %s", err_reason[err_ret == ESP_ERR_TIMEOUT]);
            //abort();
        }


        int16_t mic_min = 32767;
        int16_t mic_max = -32768;
        for(uint32_t i = 0; i < 100; i++){
            if(mic_data[i] < mic_min){
                mic_min = mic_data[i];
            }
            if(mic_data[i] > mic_max){
                mic_max = mic_data[i];
            }
        }
        set_sound_level(mic_max - mic_min);


        /*if((esp_timer_get_time() > (4 * 1000 * 1000)) && !PCM_printed){
            PCM_printed = true;
            float mic_min = MAXFLOAT;
            float mic_max = -10e20;
            memset(sig_data, 0, REC_PLAY_I2S_BUFF_SIZE);
            for(uint32_t i = 0; i < 1000; i++){
                for(uint32_t j = 0; j < 95; j++){
                    uint32_t sig_index = i + j;
                    if(sig_index < 1000){
                        sig_data[sig_index] += (float)mic_data[i] * low_pass_coefs[j];
                    }
                    else{
                        j = 95;
                    }
                }
                //ESP_LOGI(TAG, "%d = %d", (int)i, mic_data[i]);
                if(sig_data[i] < mic_min){
                    mic_min = sig_data[i];
                }
                if(sig_data[i] > mic_max){
                    mic_max = sig_data[i];
                }
            }
            ESP_LOGI(TAG, "MIN: %f\nMAX: %f", mic_min, mic_max);
        }*/

        uint8_t playing_new = get_playing();
        bool actualize_volume = false;
        if(playing_last != playing_new){
            playing_last = playing_new;
            if(playing_new == false){
                memset(spkr_data, 0, REC_PLAY_I2S_BUFF_SIZE * sizeof(int16_t));
            }
            else{
                actualize_volume = true;
            }
        }
        // recompute samples for speaker if volume or playing state has changed
        uint8_t volume_new = get_volume();
        if(((volume_last != volume_new) || actualize_volume) && playing_new){
            volume_last = volume_new;
            for(uint16_t i = 0; i < REC_PLAY_I2S_BUFF_SIZE; i++){
                spkr_data[i] = (int16_t)(sig_data[i] * volume_new / VOLUME_MAX);
            }
        }

        // send data to speaker
        err_ret = i2s_channel_write(tx_handle, spkr_data, REC_PLAY_I2S_BUFF_SIZE, &bytes_write, 1000);
        if (err_ret != ESP_OK) {
            ESP_LOGE(TAG, "Sending data to speaker error: %s", err_reason[err_ret == ESP_ERR_TIMEOUT]);
            abort();
        }
        if (bytes_read != bytes_write) {
            ESP_LOGW(TAG, "Audio codec: Uneven number of bytes read and sent. %d bytes read, %d bytes are written", bytes_read, bytes_write);
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
    ui_init();

    xTaskCreate(i2s_echo, "i2s_echo", 8192, NULL, 5, NULL);


    led_strip_set_pixel(led_strip, 0, 16, 1, 16);
    led_strip_refresh(led_strip);

    while(1){
        if(get_lcd_update_flag()){
            _lock_acquire(&lvgl_api_lock);
            display_graphics(display);
            _lock_release(&lvgl_api_lock);
        }
        usleep(20 * 1000);
    }
}