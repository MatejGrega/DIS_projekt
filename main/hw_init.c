/*
    hardware initialization
*/

#include "hw_init.h"

static const char *TAG = "hw_init";

//====================================================================================================
//  LCD
//====================================================================================================

// Using SPI2 for the communication with LCD
#define LCD_HOST  SPI2_HOST

// LCD configuration
#define LCD_PIXEL_CLOCK_HZ     (20 * 1000 * 1000)
#define LCD_BK_LIGHT_ON_LEVEL  0
#define PIN_NUM_LCD_SCLK       15
#define PIN_NUM_LCD_MOSI       9
#define PIN_NUM_LCD_MISO       8
#define PIN_NUM_LCD_DC         13
#define PIN_NUM_LCD_RST        16
#define PIN_NUM_LCD_CS         11
#define PIN_NUM_LCD_BK_LIGHT   6

// Bit number used to represent command and parameter
#define LCD_CMD_BITS           8
#define LCD_PARAM_BITS         8

#define LVGL_DRAW_BUF_LINES    20 // number of display lines in each draw buffer
#define LVGL_TICK_PERIOD_MS    2

lv_color_t *canvas_buffer = NULL;
lv_obj_t *canvas = NULL;

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx){
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

/* Rotate display and touch, when rotated screen in LVGL. Called when driver parameters are updated. */
static void lvgl_port_update_callback(lv_display_t *disp){
    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);
    esp_lcd_panel_swap_xy(panel_handle, false);
    esp_lcd_panel_mirror(panel_handle, false, true);
}

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map){
    lvgl_port_update_callback(disp);
    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // because SPI LCD is big-endian, we need to swap the RGB bytes order
    lv_draw_sw_rgb565_swap(px_map, (offsetx2 + 1 - offsetx1) * (offsety2 + 1 - offsety1));
    // copy a buffer's content to a specific area of the display
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
}

static void increase_lvgl_tick(void *arg){
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

void hw_init_lcd(lv_display_t *display){
    ESP_LOGI(TAG, "Initialize SPI bus");
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_LCD_SCLK,
        .mosi_io_num = PIN_NUM_LCD_MOSI,
        .miso_io_num = PIN_NUM_LCD_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_LCD_DC,
        .cs_gpio_num = PIN_NUM_LCD_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_LOGI(TAG, "Install ILI9341 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));

    // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();

    // create a lvgl display
    display = lv_display_create(LCD_H_RES, LCD_V_RES);

    // alloc draw buffers used by LVGL
    // it's recommended to choose the size of the draw buffer(s) to be at least 1/10 screen sized
    size_t draw_buffer_sz = LCD_H_RES * LVGL_DRAW_BUF_LINES * sizeof(lv_color16_t);

    void *buf1 = spi_bus_dma_memory_alloc(LCD_HOST, draw_buffer_sz, 0);
    assert(buf1);
    void *buf2 = spi_bus_dma_memory_alloc(LCD_HOST, draw_buffer_sz, 0);
    assert(buf2);
    // initialize LVGL draw buffers
    lv_display_set_buffers(display, buf1, buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
    // associate the mipi panel handle to the display
    lv_display_set_user_data(display, panel_handle);
    // set color depth
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    // set the callback which can copy the rendered image to an area of the display
    lv_display_set_flush_cb(display, lvgl_flush_cb);

    ESP_LOGI(TAG, "Install LVGL tick timer");
    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    ESP_LOGI(TAG, "Register io panel event callback for LVGL flush ready notification");
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = notify_lvgl_flush_ready,
    };
    /* Register done callback */
    ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, display));

    if(canvas_buffer == NULL){
        canvas_buffer = malloc(LCD_H_RES * LCD_V_RES * sizeof(lv_color_t));   //alocate static buffer just once
    }
    if(canvas == NULL){
        lv_obj_t *scr = lv_display_get_screen_active(display);
        canvas = lv_canvas_create(scr);
        lv_canvas_set_buffer(canvas, canvas_buffer, LCD_H_RES, LCD_V_RES, LV_COLOR_FORMAT_RGB888);
        lv_obj_center(canvas);
    }
}

lv_obj_t* hw_init_get_canvas(void){
    return canvas;
}

lv_color_t* hw_init_get_canvas_buffer(void){
    return canvas_buffer;
}


//====================================================================================================
//  RGB LED
//====================================================================================================

#define PIN_LED_RGB 45

void hw_init_LED_RGB(led_strip_handle_t *led_strip_p){
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    // LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = PIN_LED_RGB,
        .max_leds = 1,
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, led_strip_p));

    /* Set all LED off to clear all pixels */
    led_strip_clear(*led_strip_p);
}


//====================================================================================================
//  ADC for buttons
//====================================================================================================

#define ADC_CHANNEL             ADC_CHANNEL_5   // GPIO6 → ADC1_CHANNEL_5
#define ADC_UNIT                ADC_UNIT_1

#define ADC_TASK_STACK_SIZE     1024
#define ADC_TASK_PRIORITY       0
#define ADC_MEASURING_PERIOD_MS 5
#define STABLE_MEAS_THRESHOLD   4   //number of stable subsequent measurements to consider valid button press

adc_oneshot_unit_handle_t adc_handle;
button_adc_t buttons_state = BUTTON_NONE;

static void adc_buttons_task(void *args){
    button_adc_t button_states_FIFO[STABLE_MEAS_THRESHOLD];

    for(uint8_t i = 0; i < STABLE_MEAS_THRESHOLD; i++){     //initialization of FIFO buffer
        button_states_FIFO[i] = BUTTON_NONE;
    }

    while(1){
        int adc_raw;

        //shift buffer with previou states
        for(uint8_t i = STABLE_MEAS_THRESHOLD - 1; i > 0; i--){
            button_states_FIFO[i] = button_states_FIFO[i - 1];
        }
        
        adc_oneshot_read(adc_handle, ADC_CHANNEL, &adc_raw);

        if(adc_raw > 7771){
            button_states_FIFO[0] = BUTTON_NONE;
        }
        else if(adc_raw > 6675){
            button_states_FIFO[0] = BUTTON_K1;
        }
        else if(adc_raw > 5440){
            button_states_FIFO[0] = BUTTON_K2;
        }
        else if(adc_raw > 4115){
            button_states_FIFO[0] = BUTTON_K3;
        }
        else if(adc_raw > 2900){
            button_states_FIFO[0] = BUTTON_K4;
        }
        else if(adc_raw > 1790){
            button_states_FIFO[0] = BUTTON_K5;
        }
        else if(adc_raw > 0){
            button_states_FIFO[0] = BUTTON_K6;
        }
        else{
            button_states_FIFO[0] = BUTTON_NONE;
        }

        //check whether all values are the same
        bool valid_state = true;
        for(uint8_t i = 1; i < STABLE_MEAS_THRESHOLD; i++){
            if(button_states_FIFO[0] != button_states_FIFO[i]){
                valid_state = false;
                break;
            }
        }
        if(valid_state == true){
            buttons_state = button_states_FIFO[0];  //actualize button states
        }

        vTaskDelay(ADC_MEASURING_PERIOD_MS / portTICK_PERIOD_MS);
    }
}

void hw_init_buttons(void){
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT,
    };
    adc_oneshot_new_unit(&init_cfg, &adc_handle);

    // 2. Konfigurácia kanála
    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11  // max rozsah do ~3.3V
    };
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_cfg);

    xTaskCreate(adc_buttons_task, "buttons", ADC_TASK_STACK_SIZE, NULL, ADC_TASK_PRIORITY, NULL);
}

button_adc_t hw_get_buttons(void){
    return buttons_state;
}


//====================================================================================================
//  Audio codec
//====================================================================================================

#define EXAMPLE_SAMPLE_RATE     (16000)
#define EXAMPLE_MCLK_MULTIPLE   (384) // If not using 24-bit data width, 256 should be enough
#define EXAMPLE_MCLK_FREQ_HZ    (EXAMPLE_SAMPLE_RATE * EXAMPLE_MCLK_MULTIPLE)
#define EXAMPLE_VOICE_VOLUME    70 // range: 0-100
#define EXAMPLE_MIC_GAIN        (ES8311_MIC_GAIN_36DB)

/* I2C port and GPIOs */
#define I2C_NUM         (0)
#define I2C_SCL_IO      (GPIO_NUM_7)
#define I2C_SDA_IO      (GPIO_NUM_8)

/* I2S port and GPIOs */
#define I2S_NUM         (0)
#define I2S_MCK_IO      (GPIO_NUM_35)
#define I2S_BCK_IO      (GPIO_NUM_18)
#define I2S_WS_IO       (GPIO_NUM_17)
#define I2S_DO_IO       (GPIO_NUM_12)
#define I2S_DI_IO       (GPIO_NUM_34)

esp_err_t es8311_codec_init(void){
    const i2c_config_t es_i2c_cfg = {
        .sda_io_num = I2C_SDA_IO,
        .scl_io_num = I2C_SCL_IO,
        .mode = I2C_MODE_MASTER,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    ESP_RETURN_ON_ERROR(i2c_param_config(I2C_NUM, &es_i2c_cfg), TAG, "config i2c failed");
    ESP_RETURN_ON_ERROR(i2c_driver_install(I2C_NUM, I2C_MODE_MASTER,  0, 0, 0), TAG, "install i2c driver failed");


    /* Initialize es8311 codec */
    es8311_handle_t es_handle = es8311_create(I2C_NUM, ES8311_ADDRRES_0);
    ESP_RETURN_ON_FALSE(es_handle, ESP_FAIL, TAG, "es8311 create failed");
    const es8311_clock_config_t es_clk = {
        .mclk_inverted = false,
        .sclk_inverted = false,
        .mclk_from_mclk_pin = true,
        .mclk_frequency = EXAMPLE_MCLK_FREQ_HZ,
        .sample_frequency = EXAMPLE_SAMPLE_RATE
    };

    ESP_ERROR_CHECK(es8311_init(es_handle, &es_clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16));
    ESP_RETURN_ON_ERROR(es8311_sample_frequency_config(es_handle, EXAMPLE_SAMPLE_RATE * EXAMPLE_MCLK_MULTIPLE, EXAMPLE_SAMPLE_RATE), TAG, "set es8311 sample frequency failed");
    ESP_RETURN_ON_ERROR(es8311_voice_volume_set(es_handle, EXAMPLE_VOICE_VOLUME, NULL), TAG, "set es8311 volume failed");
    ESP_RETURN_ON_ERROR(es8311_microphone_config(es_handle, false), TAG, "set es8311 microphone failed");
    ESP_RETURN_ON_ERROR(es8311_microphone_gain_set(es_handle, EXAMPLE_MIC_GAIN), TAG, "set es8311 microphone gain failed");

    return ESP_OK;
}

esp_err_t i2s_driver_init(i2s_chan_handle_t *tx_handle, i2s_chan_handle_t *rx_handle){
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true; // Auto clear the legacy data in the DMA buffer
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, tx_handle, rx_handle));
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(EXAMPLE_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_MCK_IO,
            .bclk = I2S_BCK_IO,
            .ws = I2S_WS_IO,
            .dout = I2S_DO_IO,
            .din = I2S_DI_IO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    std_cfg.clk_cfg.mclk_multiple = EXAMPLE_MCLK_MULTIPLE;

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(*tx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(*rx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(*tx_handle));
    ESP_ERROR_CHECK(i2s_channel_enable(*rx_handle));

    return ESP_OK;
}