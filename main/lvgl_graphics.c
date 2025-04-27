/*
    Graphics of application
*/

#include "lvgl_graphics.h"

#define UI_TASK_STACK_SIZE 1024
#define UI_TASK_PRIORITY 0

bool blinking_cursor_visible = true;

// return 10^exponent
static uint16_t __pwr_10(uint8_t exponent){
    uint16_t result = 1;
    for( ; exponent > 0; exponent--){
        result *= 10;
    }
    return result;
}

static void __user_interface_task(void *args){
    button_adc_t btn_state_last = BUTTON_NONE;
    button_adc_t btn_state_actual = BUTTON_NONE;
    int16_t frequency = AUDIO_FREQUENCY_MIN;
    uint8_t setting_digit_index = 0;    //0 represents LSB
    bool actualize_lcd_blinking_cursor_flag = false;

    while(1){
        btn_state_actual = hw_get_buttons();
        if(btn_state_actual != btn_state_last){
            set_lcd_update_flag();
            if(btn_state_last == BUTTON_NONE){
                switch(btn_state_actual){
                    case BUTTON_K1:
                    setting_digit_index = get_audio_freq_digit();
                    set_audio_freq_digit(++setting_digit_index);
                        if(get_audio_freq_digit() > 3){
                            set_audio_freq_digit(0);
                        }
                        break;
                    case BUTTON_K2:
                        frequency += __pwr_10(get_audio_freq_digit());
                        if(frequency > AUDIO_FREQUENCY_MAX){
                            frequency = AUDIO_FREQUENCY_MAX;
                        }
                        set_audio_frequency(frequency);
                        break;
                    case BUTTON_K3:
                        frequency -= __pwr_10(get_audio_freq_digit());
                        if(frequency < AUDIO_FREQUENCY_MIN){
                            frequency = AUDIO_FREQUENCY_MIN;
                        }
                        set_audio_frequency(frequency);
                        break;
                    case BUTTON_K4:
                        set_playing(!get_playing());
                        break;
                    case BUTTON_K5:
                        if(get_volume() < VOLUME_MAX){
                            set_volume(get_volume() + 10);
                        }
                        break;
                    case BUTTON_K6:
                        if(get_volume() > 0){
                            set_volume(get_volume() - 10);
                        }
                        break;
                    default:
                        break;
                }
            }
        }
        btn_state_last = btn_state_actual;

        // blinking cursor on LCD
        if((((esp_timer_get_time() / 100000) % 10) >= 5) && !actualize_lcd_blinking_cursor_flag){
            actualize_lcd_blinking_cursor_flag = true;
            blinking_cursor_visible = true;
            set_lcd_update_flag();
        }
        else if((((esp_timer_get_time() / 100000) % 10) < 5) && actualize_lcd_blinking_cursor_flag){
            actualize_lcd_blinking_cursor_flag = false;
            blinking_cursor_visible = false;
            set_lcd_update_flag();
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void ui_init(void){
    xTaskCreate(__user_interface_task, "buttons", UI_TASK_STACK_SIZE, NULL, UI_TASK_PRIORITY, NULL);
}

// swaping red and blue in LCD buffer - LCD is displaying red instead of blue and vice versa
static void swap_color_bytes(void){
    uint8_t *canvas_buffer = (uint8_t*)hw_init_get_canvas_buffer();
    for (int i = 0; i < LCD_H_RES * LCD_V_RES; i++) {
        uint8_t r = canvas_buffer[i * 3];
        uint8_t g = canvas_buffer[i * 3 + 1];
        uint8_t b = canvas_buffer[i * 3 + 2];

        // swap R <-> B
        canvas_buffer[i * 3]     = b;
        canvas_buffer[i * 3 + 1] = g;
        canvas_buffer[i * 3 + 2] = r;
    }
}


//definitions related to graphics
#define COLOR_WHITE                     255,255,255
#define COLOR_BACKGROUND                10, 0, 10
#define COLOR_MENU_INACTIVE             0,0,10
#define COLOR_MENU_ACTIVE               255,180,0
#define COLOR_MENU_LINES                255, 255, 0
#define COLOR_MENU_ARROWS               COLOR_WHITE
#define COLOR_FREQ_CURSOR               COLOR_WHITE
#define COLOR_DASHED_LINE               0, 255, 0
#define COLOR_SOUND_LEVEL_BAR           0, 0, 255
#define COLOR_SOUND_LEVEL_BAR_BG        0, 0, 0
#define COLOR_SOUND_LEVEL_BAR_BORDER    COLOR_WHITE

#define MENU_LEFT_LINE_H_PX 260

#define MENU_1ST_LINE_V_PX (LCD_V_RES / 6 * 1)
#define MENU_2ND_LINE_V_PX (LCD_V_RES / 6 * 2)
#define MENU_3RD_LINE_V_PX (LCD_V_RES / 6 * 3)
#define MENU_4TH_LINE_V_PX (LCD_V_RES / 6 * 4)
#define MENU_5TH_LINE_V_PX (LCD_V_RES / 6 * 5)

#define MENU_1ST_TEXT_V_PX (0                  + LCD_V_RES / 12 - 6)
#define MENU_2ND_TEXT_V_PX (MENU_1ST_LINE_V_PX + LCD_V_RES / 12 - 6)
#define MENU_3RD_TEXT_V_PX (MENU_2ND_LINE_V_PX + LCD_V_RES / 12 - 6)
#define MENU_4TH_TEXT_V_PX (MENU_3RD_LINE_V_PX + LCD_V_RES / 12 - 6)
#define MENU_5TH_TEXT_V_PX (MENU_4TH_LINE_V_PX + LCD_V_RES / 12 - 6)
#define MENU_6TH_TEXT_V_PX (MENU_5TH_LINE_V_PX + LCD_V_RES / 12 - 6)

#define ARROW_HEIGHT_PX         25
#define ARROW_WIDTH_PX          7
#define ARROW_LINE_WIDTH_PX     2
#define ARROW_SIDES_WIDTH_PX    35
#define ARROW_SIDES_HEAD_LENGTH 10
#define ARROW_SIDES_HEAD_WIDTH  7

#define TONE_GEN_SET_VAL_LEFT_PX 120

#define TEXT_FREQ_FREQ_TOP_PX   40
#define TEXT_FREQ_FREQ_LEFT_PX  10
#define TEXT_FREQ_HEIGHT_PX     35
#define TEXT_FREQ_TOP_PX        (TEXT_FREQ_FREQ_TOP_PX - 10)
#define TEXT_FREQ_BOTTOM_PX     (TEXT_FREQ_TOP_PX + TEXT_FREQ_HEIGHT_PX)
#define TEXT_FREQ_LEFT_PX       TONE_GEN_SET_VAL_LEFT_PX
#define TEXT_FREQ_DIGIT_WIDTH   20

#define TEXT_TONE_GEN_TOP_PX   5
#define TEXT_TONE_GEN_LEFT_PX   0

#define TEXT_VOL_TOP_PX         82
#define TEXT_VOL_LEFT_PX        10
#define TEXT_VOL_VAL_TOP_PX     (TEXT_VOL_TOP_PX - 10)
#define TEXT_VOL_VAL_LEFT_PX    TONE_GEN_SET_VAL_LEFT_PX

#define LINE_DASHED_V_PX        120
#define DASHED_LINE_DASH_LENGTH_PX 20
#define DASHED_LINE_WIDTH       3

#define TEXT_MIC_TOP_PX         130
#define TEXT_MIC_LEFT_PX        0

#define SOUND_LEVEL_BAR_X_PX    10
#define SOUND_LEVEL_BAR_Y_PX    180
#define SOUND_LEVEL_BAR_WIDTH   (MENU_LEFT_LINE_H_PX - 2 * SOUND_LEVEL_BAR_X_PX)
#define SOUND_LEVEL_BAR_HEIGHT  25
#define SOUND_LEVEL_BAR_BORDER_WIDTH    2
#define TEXT_SOUND_BAR_LEFT_PX  SOUND_LEVEL_BAR_X_PX
#define TEXT_SOUND_BAR_TOP_PX   160


// graphics
void display_graphics(lv_display_t *disp)
{
    lv_obj_t *canvas = hw_init_get_canvas();
    lv_canvas_fill_bg(canvas, lv_color_make(COLOR_BACKGROUND), LV_OPA_100);

    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);

    // - - - - - - - - - - - Tone generation label

    lv_draw_label_dsc_t txt_tone_gen;
    lv_draw_label_dsc_init(&txt_tone_gen);
    txt_tone_gen.color = lv_color_make(COLOR_WHITE);
    txt_tone_gen.text = "Tone generator settings:";
    txt_tone_gen.align = LV_ALIGN_TOP_MID;
    lv_area_t coords_txt_tone_gen;
    coords_txt_tone_gen.x1 = TEXT_TONE_GEN_LEFT_PX;
    coords_txt_tone_gen.y1 = TEXT_TONE_GEN_TOP_PX;
    coords_txt_tone_gen.x2 = MENU_LEFT_LINE_H_PX;
    coords_txt_tone_gen.y2 = LCD_V_RES;
    lv_draw_label(&layer, &txt_tone_gen, &coords_txt_tone_gen);

    // - - - - - - - - - - - setting frequency -> digits

    //prepare frequency digits
    uint8_t frequency_digits[] = {0, 0, 0, 0};
    uint16_t tmp_freq = get_audio_frequency();
    for(int8_t i = 0; i < 4; i++){
        frequency_digits[i] = tmp_freq % 10;
        tmp_freq /= 10;
    }

    lv_draw_label_dsc_t txt_freq_dig3_dsc;
    lv_draw_label_dsc_init(&txt_freq_dig3_dsc);
    txt_freq_dig3_dsc.color = lv_color_make(COLOR_WHITE);
    txt_freq_dig3_dsc.font = &lv_font_montserrat_30;
    txt_freq_dig3_dsc.align = LV_TEXT_ALIGN_CENTER;
    char chbuff_dig3[2];
    chbuff_dig3[0] = '0' + frequency_digits[3];
    chbuff_dig3[1] = 0;
    txt_freq_dig3_dsc.text = chbuff_dig3;
    lv_area_t coords_txt_freq_dig3;
    coords_txt_freq_dig3.x1 = TEXT_FREQ_LEFT_PX;
    coords_txt_freq_dig3.y1 = TEXT_FREQ_TOP_PX;
    coords_txt_freq_dig3.x2 = coords_txt_freq_dig3.x1 + TEXT_FREQ_DIGIT_WIDTH;
    coords_txt_freq_dig3.y2 = TEXT_FREQ_BOTTOM_PX;
    lv_draw_label(&layer, &txt_freq_dig3_dsc, &coords_txt_freq_dig3);

    lv_draw_label_dsc_t txt_freq_dig2_dsc;
    lv_draw_label_dsc_init(&txt_freq_dig2_dsc);
    txt_freq_dig2_dsc.color = lv_color_make(COLOR_WHITE);
    txt_freq_dig2_dsc.font = &lv_font_montserrat_30;
    txt_freq_dig2_dsc.align = LV_TEXT_ALIGN_CENTER;
    char chbuff_dig2[2];
    chbuff_dig2[0] = '0' + frequency_digits[2];
    chbuff_dig2[1] = 0;
    txt_freq_dig2_dsc.text = chbuff_dig2;
    lv_area_t coords_txt_freq_dig2;
    coords_txt_freq_dig2.x1 = coords_txt_freq_dig3.x2;
    coords_txt_freq_dig2.y1 = TEXT_FREQ_TOP_PX;
    coords_txt_freq_dig2.x2 = coords_txt_freq_dig2.x1 + TEXT_FREQ_DIGIT_WIDTH;
    coords_txt_freq_dig2.y2 = TEXT_FREQ_BOTTOM_PX;
    lv_draw_label(&layer, &txt_freq_dig2_dsc, &coords_txt_freq_dig2);

    lv_draw_label_dsc_t txt_freq_dig1_dsc;
    lv_draw_label_dsc_init(&txt_freq_dig1_dsc);
    txt_freq_dig1_dsc.color = lv_color_make(COLOR_WHITE);
    txt_freq_dig1_dsc.font = &lv_font_montserrat_30;
    txt_freq_dig1_dsc.align = LV_TEXT_ALIGN_CENTER;
    char chbuff_dig1[2];
    chbuff_dig1[0] = '0' + frequency_digits[1];
    chbuff_dig1[1] = 0;
    txt_freq_dig1_dsc.text = chbuff_dig1;
    lv_area_t coords_txt_freq_dig1;
    coords_txt_freq_dig1.x1 = coords_txt_freq_dig2.x2;
    coords_txt_freq_dig1.y1 = TEXT_FREQ_TOP_PX;
    coords_txt_freq_dig1.x2 = coords_txt_freq_dig1.x1 + TEXT_FREQ_DIGIT_WIDTH;
    coords_txt_freq_dig1.y2 = TEXT_FREQ_BOTTOM_PX;
    lv_draw_label(&layer, &txt_freq_dig1_dsc, &coords_txt_freq_dig1);

    lv_draw_label_dsc_t txt_freq_dig0_dsc;
    lv_draw_label_dsc_init(&txt_freq_dig0_dsc);
    txt_freq_dig0_dsc.color = lv_color_make(COLOR_WHITE);
    txt_freq_dig0_dsc.font = &lv_font_montserrat_30;
    txt_freq_dig0_dsc.align = LV_TEXT_ALIGN_CENTER;
    char chbuff_dig0[2];
    chbuff_dig0[0] = '0' + frequency_digits[0];
    chbuff_dig0[1] = 0;
    txt_freq_dig0_dsc.text = chbuff_dig0;
    lv_area_t coords_txt_freq_dig0;
    coords_txt_freq_dig0.x1 = coords_txt_freq_dig1.x2;
    coords_txt_freq_dig0.y1 = TEXT_FREQ_TOP_PX;
    coords_txt_freq_dig0.x2 = coords_txt_freq_dig0.x1 + TEXT_FREQ_DIGIT_WIDTH;
    coords_txt_freq_dig0.y2 = TEXT_FREQ_BOTTOM_PX;
    lv_draw_label(&layer, &txt_freq_dig0_dsc, &coords_txt_freq_dig0);

    // - - - - - - - - - - - setting frequency -> blinking cursor

    if(blinking_cursor_visible){
        lv_draw_line_dsc_t line_freq_cursor_dsc;
        lv_draw_line_dsc_init(&line_freq_cursor_dsc);
        line_freq_cursor_dsc.color = lv_color_make(COLOR_MENU_LINES);
        line_freq_cursor_dsc.width = 2;
        line_freq_cursor_dsc.p1.x = TEXT_FREQ_LEFT_PX + (3 - get_audio_freq_digit()) * TEXT_FREQ_DIGIT_WIDTH;
        line_freq_cursor_dsc.p1.y = TEXT_FREQ_BOTTOM_PX;
        line_freq_cursor_dsc.p2.x = line_freq_cursor_dsc.p1.x + TEXT_FREQ_DIGIT_WIDTH;
        line_freq_cursor_dsc.p2.y = TEXT_FREQ_BOTTOM_PX;
        lv_draw_line(&layer, &line_freq_cursor_dsc);
    }

    // - - - - - - - - - - - setting frequency -> texts

    lv_draw_label_dsc_t txt_freq_freq;
    lv_draw_label_dsc_init(&txt_freq_freq);
    txt_freq_freq.color = lv_color_make(COLOR_WHITE);
    txt_freq_freq.text = "Frequency:";
    txt_freq_freq.align = LV_ALIGN_LEFT_MID;
    lv_area_t coords_txt_freq_freq;
    coords_txt_freq_freq.x1 = TEXT_FREQ_FREQ_LEFT_PX;
    coords_txt_freq_freq.y1 = TEXT_FREQ_FREQ_TOP_PX;
    coords_txt_freq_freq.x2 = LCD_H_RES;
    coords_txt_freq_freq.y2 = LCD_V_RES;
    lv_draw_label(&layer, &txt_freq_freq, &coords_txt_freq_freq);

    lv_draw_label_dsc_t txt_freq_hz;
    lv_draw_label_dsc_init(&txt_freq_hz);
    txt_freq_hz.color = lv_color_make(COLOR_WHITE);
    txt_freq_hz.font = &lv_font_montserrat_30;
    txt_freq_hz.text = "Hz";
    txt_freq_hz.align = LV_ALIGN_LEFT_MID;
    lv_area_t coords_txt_freq_hz;
    coords_txt_freq_hz.x1 = coords_txt_freq_dig0.x2 + 7;
    coords_txt_freq_hz.y1 = TEXT_FREQ_TOP_PX;
    coords_txt_freq_hz.x2 = LCD_H_RES;
    coords_txt_freq_hz.y2 = LCD_V_RES;
    lv_draw_label(&layer, &txt_freq_hz, &coords_txt_freq_hz);

    // - - - - - - - - - - - audio output volume

    lv_draw_label_dsc_t txt_volume_dsc;
    lv_draw_label_dsc_init(&txt_volume_dsc);
    txt_volume_dsc.color = lv_color_make(COLOR_WHITE);
    txt_volume_dsc.text = "Audio volume:";
    lv_area_t coords_volume;
    coords_volume.x1 = TEXT_VOL_LEFT_PX;
    coords_volume.y1 = TEXT_VOL_TOP_PX;
    coords_volume.x2 = LCD_H_RES;
    coords_volume.y2 = LCD_V_RES;
    lv_draw_label(&layer, &txt_volume_dsc, &coords_volume);

    lv_draw_label_dsc_t txt_volume_val_dsc;
    lv_draw_label_dsc_init(&txt_volume_val_dsc);
    txt_volume_val_dsc.color = lv_color_make(COLOR_WHITE);
    txt_volume_val_dsc.font = &lv_font_montserrat_30;
    char vol_buff[10] = {0};
    itoa((int)get_volume(), vol_buff, 10);
    txt_volume_val_dsc.text = vol_buff;
    lv_area_t coords_volume_val;
    coords_volume_val.x1 = TEXT_VOL_VAL_LEFT_PX;
    coords_volume_val.y1 = TEXT_VOL_VAL_TOP_PX;
    coords_volume_val.x2 = LCD_H_RES;
    coords_volume_val.y2 = LCD_V_RES;
    lv_draw_label(&layer, &txt_volume_val_dsc, &coords_volume_val);

    // - - - - - - - - - - - MENU -> background and active button

    //menu background
    lv_draw_rect_dsc_t bg_menu_dsc;
    lv_draw_rect_dsc_init(&bg_menu_dsc);
    bg_menu_dsc.bg_color = lv_color_make(COLOR_MENU_INACTIVE);
    lv_area_t bg_menu_coords = {MENU_LEFT_LINE_H_PX, 0, LCD_H_RES, LCD_V_RES};
    lv_draw_rect(&layer, &bg_menu_dsc, &bg_menu_coords);

    //active button background
    lv_draw_rect_dsc_t bg_menu_active_dsc;
    lv_draw_rect_dsc_init(&bg_menu_active_dsc);
    bg_menu_active_dsc.bg_color = lv_color_make(COLOR_MENU_ACTIVE);
    int8_t btn_curr_state = (int8_t)hw_get_buttons();
    if(btn_curr_state == BUTTON_NONE){
        btn_curr_state = -1;            //move index of pressed button away from valid numbers, so rectangle of active button is outside the canvas
    }
    lv_area_t bg_menu_active_coords = {MENU_LEFT_LINE_H_PX, ((btn_curr_state - BUTTON_K1) * MENU_1ST_LINE_V_PX), LCD_H_RES, ((btn_curr_state - BUTTON_K1 + 1) * MENU_1ST_LINE_V_PX)};
    lv_draw_rect(&layer, &bg_menu_active_dsc, &bg_menu_active_coords);

    // - - - - - - - - - - - Audio analysis -> dashed line and label

    lv_draw_line_dsc_t line_audio_analysis_dashed_0_dsc;
    lv_draw_line_dsc_init(&line_audio_analysis_dashed_0_dsc);
    line_audio_analysis_dashed_0_dsc.color = lv_color_make(COLOR_DASHED_LINE);
    line_audio_analysis_dashed_0_dsc.width = DASHED_LINE_WIDTH;
    line_audio_analysis_dashed_0_dsc.p1.x = 0 * 2 * DASHED_LINE_DASH_LENGTH_PX;
    line_audio_analysis_dashed_0_dsc.p1.y = LINE_DASHED_V_PX;
    line_audio_analysis_dashed_0_dsc.p2.x = line_audio_analysis_dashed_0_dsc.p1.x + DASHED_LINE_DASH_LENGTH_PX;
    line_audio_analysis_dashed_0_dsc.p2.y = LINE_DASHED_V_PX;
    lv_draw_line(&layer, &line_audio_analysis_dashed_0_dsc);

    lv_draw_line_dsc_t line_audio_analysis_dashed_1_dsc;
    lv_draw_line_dsc_init(&line_audio_analysis_dashed_1_dsc);
    line_audio_analysis_dashed_1_dsc.color = lv_color_make(COLOR_DASHED_LINE);
    line_audio_analysis_dashed_1_dsc.width = DASHED_LINE_WIDTH;
    line_audio_analysis_dashed_1_dsc.p1.x = 1 * 2 * DASHED_LINE_DASH_LENGTH_PX;
    line_audio_analysis_dashed_1_dsc.p1.y = LINE_DASHED_V_PX;
    line_audio_analysis_dashed_1_dsc.p2.x = line_audio_analysis_dashed_1_dsc.p1.x + DASHED_LINE_DASH_LENGTH_PX;
    line_audio_analysis_dashed_1_dsc.p2.y = LINE_DASHED_V_PX;
    lv_draw_line(&layer, &line_audio_analysis_dashed_1_dsc);

    lv_draw_line_dsc_t line_audio_analysis_dashed_2_dsc;
    lv_draw_line_dsc_init(&line_audio_analysis_dashed_2_dsc);
    line_audio_analysis_dashed_2_dsc.color = lv_color_make(COLOR_DASHED_LINE);
    line_audio_analysis_dashed_2_dsc.width = DASHED_LINE_WIDTH;
    line_audio_analysis_dashed_2_dsc.p1.x = 2 * 2 * DASHED_LINE_DASH_LENGTH_PX;
    line_audio_analysis_dashed_2_dsc.p1.y = LINE_DASHED_V_PX;
    line_audio_analysis_dashed_2_dsc.p2.x = line_audio_analysis_dashed_2_dsc.p1.x + DASHED_LINE_DASH_LENGTH_PX;
    line_audio_analysis_dashed_2_dsc.p2.y = LINE_DASHED_V_PX;
    lv_draw_line(&layer, &line_audio_analysis_dashed_2_dsc);

    lv_draw_line_dsc_t line_audio_analysis_dashed_3_dsc;
    lv_draw_line_dsc_init(&line_audio_analysis_dashed_3_dsc);
    line_audio_analysis_dashed_3_dsc.color = lv_color_make(COLOR_DASHED_LINE);
    line_audio_analysis_dashed_3_dsc.width = DASHED_LINE_WIDTH;
    line_audio_analysis_dashed_3_dsc.p1.x = 3 * 2 * DASHED_LINE_DASH_LENGTH_PX;
    line_audio_analysis_dashed_3_dsc.p1.y = LINE_DASHED_V_PX;
    line_audio_analysis_dashed_3_dsc.p2.x = line_audio_analysis_dashed_3_dsc.p1.x + DASHED_LINE_DASH_LENGTH_PX;
    line_audio_analysis_dashed_3_dsc.p2.y = LINE_DASHED_V_PX;
    lv_draw_line(&layer, &line_audio_analysis_dashed_3_dsc);

    lv_draw_line_dsc_t line_audio_analysis_dashed_4_dsc;
    lv_draw_line_dsc_init(&line_audio_analysis_dashed_4_dsc);
    line_audio_analysis_dashed_4_dsc.color = lv_color_make(COLOR_DASHED_LINE);
    line_audio_analysis_dashed_4_dsc.width = DASHED_LINE_WIDTH;
    line_audio_analysis_dashed_4_dsc.p1.x = 4 * 2 * DASHED_LINE_DASH_LENGTH_PX;
    line_audio_analysis_dashed_4_dsc.p1.y = LINE_DASHED_V_PX;
    line_audio_analysis_dashed_4_dsc.p2.x = line_audio_analysis_dashed_4_dsc.p1.x + DASHED_LINE_DASH_LENGTH_PX;
    line_audio_analysis_dashed_4_dsc.p2.y = LINE_DASHED_V_PX;
    lv_draw_line(&layer, &line_audio_analysis_dashed_4_dsc);

    lv_draw_line_dsc_t line_audio_analysis_dashed_5_dsc;
    lv_draw_line_dsc_init(&line_audio_analysis_dashed_5_dsc);
    line_audio_analysis_dashed_5_dsc.color = lv_color_make(COLOR_DASHED_LINE);
    line_audio_analysis_dashed_5_dsc.width = DASHED_LINE_WIDTH;
    line_audio_analysis_dashed_5_dsc.p1.x = 5 * 2 * DASHED_LINE_DASH_LENGTH_PX;
    line_audio_analysis_dashed_5_dsc.p1.y = LINE_DASHED_V_PX;
    line_audio_analysis_dashed_5_dsc.p2.x = line_audio_analysis_dashed_5_dsc.p1.x + DASHED_LINE_DASH_LENGTH_PX;
    line_audio_analysis_dashed_5_dsc.p2.y = LINE_DASHED_V_PX;
    lv_draw_line(&layer, &line_audio_analysis_dashed_5_dsc);

    lv_draw_line_dsc_t line_audio_analysis_dashed_6_dsc;
    lv_draw_line_dsc_init(&line_audio_analysis_dashed_6_dsc);
    line_audio_analysis_dashed_6_dsc.color = lv_color_make(COLOR_DASHED_LINE);
    line_audio_analysis_dashed_6_dsc.width = DASHED_LINE_WIDTH;
    line_audio_analysis_dashed_6_dsc.p1.x = 6 * 2 * DASHED_LINE_DASH_LENGTH_PX;
    line_audio_analysis_dashed_6_dsc.p1.y = LINE_DASHED_V_PX;
    line_audio_analysis_dashed_6_dsc.p2.x = line_audio_analysis_dashed_6_dsc.p1.x + DASHED_LINE_DASH_LENGTH_PX;
    line_audio_analysis_dashed_6_dsc.p2.y = LINE_DASHED_V_PX;
    lv_draw_line(&layer, &line_audio_analysis_dashed_6_dsc);

    lv_draw_label_dsc_t txt_audio_analysis_dsc;
    lv_draw_label_dsc_init(&txt_audio_analysis_dsc);
    txt_audio_analysis_dsc.color = lv_color_make(COLOR_WHITE);
    txt_audio_analysis_dsc.text = "Audio analysis:";
    txt_audio_analysis_dsc.align = LV_ALIGN_TOP_MID;
    lv_area_t coords_txt_audio_analysis_dsc;
    coords_txt_audio_analysis_dsc.x1 = TEXT_MIC_LEFT_PX;
    coords_txt_audio_analysis_dsc.y1 = TEXT_MIC_TOP_PX;
    coords_txt_audio_analysis_dsc.x2 = MENU_LEFT_LINE_H_PX;
    coords_txt_audio_analysis_dsc.y2 = LCD_V_RES;
    lv_draw_label(&layer, &txt_audio_analysis_dsc, &coords_txt_audio_analysis_dsc);

    // sound level bar label
    lv_draw_label_dsc_t txt_sound_bar_freq_dsc;
    lv_draw_label_dsc_init(&txt_sound_bar_freq_dsc);
    txt_sound_bar_freq_dsc.color = lv_color_make(COLOR_WHITE);
    txt_sound_bar_freq_dsc.text = "Sound level:";
    lv_area_t coords_txt_sound_bar_freq;
    coords_txt_sound_bar_freq.x1 = TEXT_SOUND_BAR_LEFT_PX;
    coords_txt_sound_bar_freq.y1 = TEXT_SOUND_BAR_TOP_PX;
    coords_txt_sound_bar_freq.x2 = LCD_H_RES;
    coords_txt_sound_bar_freq.y2 = LCD_V_RES;
    lv_draw_label(&layer, &txt_sound_bar_freq_dsc, &coords_txt_sound_bar_freq);

    // - - - - - - - - - - - Audio analysis -> sound level bar

    // sound level bar background
    lv_draw_rect_dsc_t sound_level_bar_bg_dsc;
    lv_draw_rect_dsc_init(&sound_level_bar_bg_dsc);
    sound_level_bar_bg_dsc.bg_color = lv_color_make(COLOR_SOUND_LEVEL_BAR_BG);
    lv_area_t sound_level_bar_bg_coords;
    sound_level_bar_bg_coords.x1 = SOUND_LEVEL_BAR_X_PX;
    sound_level_bar_bg_coords.y1 = SOUND_LEVEL_BAR_Y_PX;
    sound_level_bar_bg_coords.x2 = sound_level_bar_bg_coords.x1 + SOUND_LEVEL_BAR_WIDTH;
    sound_level_bar_bg_coords.y2 = sound_level_bar_bg_coords.y1 + SOUND_LEVEL_BAR_HEIGHT;
    lv_draw_rect(&layer, &sound_level_bar_bg_dsc, &sound_level_bar_bg_coords);
    
    // sound level bar fill
    uint32_t sound_level_bar_value_px = (uint32_t)get_sound_level() * SOUND_LEVEL_BAR_WIDTH / SOUND_LEVEL_BAR_MAX_VAL;
    if(sound_level_bar_value_px > SOUND_LEVEL_BAR_WIDTH){
        sound_level_bar_value_px = SOUND_LEVEL_BAR_WIDTH;
    }
    lv_draw_rect_dsc_t sound_level_bar_rct_dsc;
    lv_draw_rect_dsc_init(&sound_level_bar_rct_dsc);
    sound_level_bar_rct_dsc.bg_color = lv_color_make(COLOR_SOUND_LEVEL_BAR);
    lv_area_t sound_level_bar_rct_coords;
    sound_level_bar_rct_coords.x1 = SOUND_LEVEL_BAR_X_PX;
    sound_level_bar_rct_coords.y1 = SOUND_LEVEL_BAR_Y_PX;
    sound_level_bar_rct_coords.x2 = sound_level_bar_rct_coords.x1 + sound_level_bar_value_px;
    sound_level_bar_rct_coords.y2 = sound_level_bar_rct_coords.y1 + SOUND_LEVEL_BAR_HEIGHT;
    lv_draw_rect(&layer, &sound_level_bar_rct_dsc, &sound_level_bar_rct_coords);

    // Sound level bar border
    lv_draw_line_dsc_t line_sound_level_bar_left_dsc;
    lv_draw_line_dsc_init(&line_sound_level_bar_left_dsc);
    line_sound_level_bar_left_dsc.color = lv_color_make(COLOR_SOUND_LEVEL_BAR_BORDER);
    line_sound_level_bar_left_dsc.width = SOUND_LEVEL_BAR_BORDER_WIDTH;
    line_sound_level_bar_left_dsc.p1.x = SOUND_LEVEL_BAR_X_PX;
    line_sound_level_bar_left_dsc.p1.y = SOUND_LEVEL_BAR_Y_PX;
    line_sound_level_bar_left_dsc.p2.x = SOUND_LEVEL_BAR_X_PX;
    line_sound_level_bar_left_dsc.p2.y = line_sound_level_bar_left_dsc.p1.y + SOUND_LEVEL_BAR_HEIGHT;
    lv_draw_line(&layer, &line_sound_level_bar_left_dsc);

    lv_draw_line_dsc_t line_sound_level_bar_right_dsc;
    lv_draw_line_dsc_init(&line_sound_level_bar_right_dsc);
    line_sound_level_bar_right_dsc.color = lv_color_make(COLOR_SOUND_LEVEL_BAR_BORDER);
    line_sound_level_bar_right_dsc.width = SOUND_LEVEL_BAR_BORDER_WIDTH;
    line_sound_level_bar_right_dsc.p1.x = SOUND_LEVEL_BAR_X_PX + SOUND_LEVEL_BAR_WIDTH;
    line_sound_level_bar_right_dsc.p1.y = SOUND_LEVEL_BAR_Y_PX;
    line_sound_level_bar_right_dsc.p2.x = SOUND_LEVEL_BAR_X_PX + SOUND_LEVEL_BAR_WIDTH;
    line_sound_level_bar_right_dsc.p2.y = line_sound_level_bar_right_dsc.p1.y + SOUND_LEVEL_BAR_HEIGHT;
    lv_draw_line(&layer, &line_sound_level_bar_right_dsc);

    lv_draw_line_dsc_t line_sound_level_bar_top_dsc;
    lv_draw_line_dsc_init(&line_sound_level_bar_top_dsc);
    line_sound_level_bar_top_dsc.color = lv_color_make(COLOR_SOUND_LEVEL_BAR_BORDER);
    line_sound_level_bar_top_dsc.width = SOUND_LEVEL_BAR_BORDER_WIDTH;
    line_sound_level_bar_top_dsc.p1.x = SOUND_LEVEL_BAR_X_PX;
    line_sound_level_bar_top_dsc.p1.y = SOUND_LEVEL_BAR_Y_PX;
    line_sound_level_bar_top_dsc.p2.x = SOUND_LEVEL_BAR_X_PX + SOUND_LEVEL_BAR_WIDTH;
    line_sound_level_bar_top_dsc.p2.y = SOUND_LEVEL_BAR_Y_PX;
    lv_draw_line(&layer, &line_sound_level_bar_top_dsc);

    lv_draw_line_dsc_t line_sound_level_bar_bottom_dsc;
    lv_draw_line_dsc_init(&line_sound_level_bar_bottom_dsc);
    line_sound_level_bar_bottom_dsc.color = lv_color_make(COLOR_SOUND_LEVEL_BAR_BORDER);
    line_sound_level_bar_bottom_dsc.width = SOUND_LEVEL_BAR_BORDER_WIDTH;
    line_sound_level_bar_bottom_dsc.p1.x = SOUND_LEVEL_BAR_X_PX;
    line_sound_level_bar_bottom_dsc.p1.y = SOUND_LEVEL_BAR_Y_PX + SOUND_LEVEL_BAR_HEIGHT;
    line_sound_level_bar_bottom_dsc.p2.x = SOUND_LEVEL_BAR_X_PX + SOUND_LEVEL_BAR_WIDTH;
    line_sound_level_bar_bottom_dsc.p2.y = SOUND_LEVEL_BAR_Y_PX + SOUND_LEVEL_BAR_HEIGHT;
    lv_draw_line(&layer, &line_sound_level_bar_bottom_dsc);

    // - - - - - - - - - - - MENU -> lines (border of buttons)

    lv_draw_line_dsc_t line_menu_left_dsc;
    lv_draw_line_dsc_init(&line_menu_left_dsc);
    line_menu_left_dsc.color = lv_color_make(COLOR_MENU_LINES);
    line_menu_left_dsc.width = 2;
    line_menu_left_dsc.p1.x = MENU_LEFT_LINE_H_PX;
    line_menu_left_dsc.p1.y = 0;
    line_menu_left_dsc.p2.x = MENU_LEFT_LINE_H_PX;
    line_menu_left_dsc.p2.y = LCD_V_RES;
    lv_draw_line(&layer, &line_menu_left_dsc);

    lv_draw_line_dsc_t line_menu_1st_dsc;
    lv_draw_line_dsc_init(&line_menu_1st_dsc);
    line_menu_1st_dsc.color = lv_color_make(COLOR_MENU_LINES);
    line_menu_1st_dsc.width = 2;
    line_menu_1st_dsc.p1.x = MENU_LEFT_LINE_H_PX;
    line_menu_1st_dsc.p1.y = MENU_1ST_LINE_V_PX;
    line_menu_1st_dsc.p2.x = LCD_H_RES;
    line_menu_1st_dsc.p2.y = MENU_1ST_LINE_V_PX;
    lv_draw_line(&layer, &line_menu_1st_dsc);

    lv_draw_line_dsc_t line_menu_2nd_dsc;
    lv_draw_line_dsc_init(&line_menu_2nd_dsc);
    line_menu_2nd_dsc.color = lv_color_make(COLOR_MENU_LINES);
    line_menu_2nd_dsc.width = 2;
    line_menu_2nd_dsc.p1.x = MENU_LEFT_LINE_H_PX;
    line_menu_2nd_dsc.p1.y = MENU_2ND_LINE_V_PX;
    line_menu_2nd_dsc.p2.x = LCD_H_RES;
    line_menu_2nd_dsc.p2.y = MENU_2ND_LINE_V_PX;
    lv_draw_line(&layer, &line_menu_2nd_dsc);

    lv_draw_line_dsc_t line_menu_3rd_dsc;
    lv_draw_line_dsc_init(&line_menu_3rd_dsc);
    line_menu_3rd_dsc.color = lv_color_make(COLOR_MENU_LINES);
    line_menu_3rd_dsc.width = 2;
    line_menu_3rd_dsc.p1.x = MENU_LEFT_LINE_H_PX;
    line_menu_3rd_dsc.p1.y = MENU_3RD_LINE_V_PX;
    line_menu_3rd_dsc.p2.x = LCD_H_RES;
    line_menu_3rd_dsc.p2.y = MENU_3RD_LINE_V_PX;
    lv_draw_line(&layer, &line_menu_3rd_dsc);

    lv_draw_line_dsc_t line_menu_4th_dsc;
    lv_draw_line_dsc_init(&line_menu_4th_dsc);
    line_menu_4th_dsc.color = lv_color_make(COLOR_MENU_LINES);
    line_menu_4th_dsc.width = 2;
    line_menu_4th_dsc.p1.x = MENU_LEFT_LINE_H_PX;
    line_menu_4th_dsc.p1.y = MENU_4TH_LINE_V_PX;
    line_menu_4th_dsc.p2.x = LCD_H_RES;
    line_menu_4th_dsc.p2.y = MENU_4TH_LINE_V_PX;
    lv_draw_line(&layer, &line_menu_4th_dsc);

    lv_draw_line_dsc_t line_menu_5th_dsc;
    lv_draw_line_dsc_init(&line_menu_5th_dsc);
    line_menu_5th_dsc.color = lv_color_make(COLOR_MENU_LINES);
    line_menu_5th_dsc.width = 2;
    line_menu_5th_dsc.p1.x = MENU_LEFT_LINE_H_PX;
    line_menu_5th_dsc.p1.y = MENU_5TH_LINE_V_PX;
    line_menu_5th_dsc.p2.x = LCD_H_RES;
    line_menu_5th_dsc.p2.y = MENU_5TH_LINE_V_PX;
    lv_draw_line(&layer, &line_menu_5th_dsc);

    // - - - - - - - - - - - MENU -> arrows (lines)

    //arrow right-left
    lv_draw_line_dsc_t line_menu_arrw_sides_1_dsc;
    lv_draw_line_dsc_init(&line_menu_arrw_sides_1_dsc);
    line_menu_arrw_sides_1_dsc.color = lv_color_make(COLOR_MENU_ARROWS);
    line_menu_arrw_sides_1_dsc.width = ARROW_LINE_WIDTH_PX;
    line_menu_arrw_sides_1_dsc.p1.x = MENU_LEFT_LINE_H_PX + (LCD_H_RES - MENU_LEFT_LINE_H_PX - ARROW_SIDES_WIDTH_PX) / 2;
    line_menu_arrw_sides_1_dsc.p1.y = MENU_1ST_LINE_V_PX / 2;
    line_menu_arrw_sides_1_dsc.p2.x = line_menu_arrw_sides_1_dsc.p1.x + ARROW_SIDES_WIDTH_PX;
    line_menu_arrw_sides_1_dsc.p2.y = line_menu_arrw_sides_1_dsc.p1.y;
    lv_draw_line(&layer, &line_menu_arrw_sides_1_dsc);

    lv_draw_line_dsc_t line_menu_arrw_sides_2_dsc;
    lv_draw_line_dsc_init(&line_menu_arrw_sides_2_dsc);
    line_menu_arrw_sides_2_dsc.color = lv_color_make(COLOR_MENU_ARROWS);
    line_menu_arrw_sides_2_dsc.width = ARROW_LINE_WIDTH_PX;
    line_menu_arrw_sides_2_dsc.p1.x = line_menu_arrw_sides_1_dsc.p1.x;
    line_menu_arrw_sides_2_dsc.p1.y = line_menu_arrw_sides_1_dsc.p1.y;
    line_menu_arrw_sides_2_dsc.p2.x = line_menu_arrw_sides_2_dsc.p1.x + ARROW_SIDES_HEAD_LENGTH;
    line_menu_arrw_sides_2_dsc.p2.y = line_menu_arrw_sides_2_dsc.p1.y - ARROW_SIDES_HEAD_WIDTH;
    lv_draw_line(&layer, &line_menu_arrw_sides_2_dsc);

    lv_draw_line_dsc_t line_menu_arrw_sides_3_dsc;
    lv_draw_line_dsc_init(&line_menu_arrw_sides_3_dsc);
    line_menu_arrw_sides_3_dsc.color = lv_color_make(COLOR_MENU_ARROWS);
    line_menu_arrw_sides_3_dsc.width = ARROW_LINE_WIDTH_PX;
    line_menu_arrw_sides_3_dsc.p1.x = line_menu_arrw_sides_1_dsc.p1.x;
    line_menu_arrw_sides_3_dsc.p1.y = line_menu_arrw_sides_1_dsc.p1.y;
    line_menu_arrw_sides_3_dsc.p2.x = line_menu_arrw_sides_3_dsc.p1.x + ARROW_SIDES_HEAD_LENGTH;
    line_menu_arrw_sides_3_dsc.p2.y = line_menu_arrw_sides_3_dsc.p1.y + ARROW_SIDES_HEAD_WIDTH;
    lv_draw_line(&layer, &line_menu_arrw_sides_3_dsc);

    lv_draw_line_dsc_t line_menu_arrw_sides_4_dsc;
    lv_draw_line_dsc_init(&line_menu_arrw_sides_4_dsc);
    line_menu_arrw_sides_4_dsc.color = lv_color_make(COLOR_MENU_ARROWS);
    line_menu_arrw_sides_4_dsc.width = ARROW_LINE_WIDTH_PX;
    line_menu_arrw_sides_4_dsc.p1.x = line_menu_arrw_sides_1_dsc.p2.x;
    line_menu_arrw_sides_4_dsc.p1.y = line_menu_arrw_sides_1_dsc.p1.y;
    line_menu_arrw_sides_4_dsc.p2.x = line_menu_arrw_sides_4_dsc.p1.x - ARROW_SIDES_HEAD_LENGTH;
    line_menu_arrw_sides_4_dsc.p2.y = line_menu_arrw_sides_4_dsc.p1.y + ARROW_SIDES_HEAD_WIDTH;
    lv_draw_line(&layer, &line_menu_arrw_sides_4_dsc);

    lv_draw_line_dsc_t line_menu_arrw_sides_5_dsc;
    lv_draw_line_dsc_init(&line_menu_arrw_sides_5_dsc);
    line_menu_arrw_sides_5_dsc.color = lv_color_make(COLOR_MENU_ARROWS);
    line_menu_arrw_sides_5_dsc.width = ARROW_LINE_WIDTH_PX;
    line_menu_arrw_sides_5_dsc.p1.x = line_menu_arrw_sides_1_dsc.p2.x;
    line_menu_arrw_sides_5_dsc.p1.y = line_menu_arrw_sides_1_dsc.p1.y;
    line_menu_arrw_sides_5_dsc.p2.x = line_menu_arrw_sides_5_dsc.p1.x - ARROW_SIDES_HEAD_LENGTH;
    line_menu_arrw_sides_5_dsc.p2.y = line_menu_arrw_sides_5_dsc.p1.y - ARROW_SIDES_HEAD_WIDTH;
    lv_draw_line(&layer, &line_menu_arrw_sides_5_dsc);

    //arrow up
    lv_draw_line_dsc_t line_menu_arrw_up_1_dsc;
    lv_draw_line_dsc_init(&line_menu_arrw_up_1_dsc);
    line_menu_arrw_up_1_dsc.color = lv_color_make(COLOR_MENU_ARROWS);
    line_menu_arrw_up_1_dsc.width = ARROW_LINE_WIDTH_PX;
    line_menu_arrw_up_1_dsc.p1.x = MENU_LEFT_LINE_H_PX + (LCD_H_RES - MENU_LEFT_LINE_H_PX) / 2;
    line_menu_arrw_up_1_dsc.p1.y = MENU_1ST_LINE_V_PX + (MENU_2ND_LINE_V_PX - MENU_1ST_LINE_V_PX - ARROW_HEIGHT_PX) / 2;
    line_menu_arrw_up_1_dsc.p2.x = line_menu_arrw_up_1_dsc.p1.x;
    line_menu_arrw_up_1_dsc.p2.y = line_menu_arrw_up_1_dsc.p1.y + ARROW_HEIGHT_PX;
    lv_draw_line(&layer, &line_menu_arrw_up_1_dsc);

    lv_draw_line_dsc_t line_menu_arrw_up_2_dsc;
    lv_draw_line_dsc_init(&line_menu_arrw_up_2_dsc);
    line_menu_arrw_up_2_dsc.color = lv_color_make(COLOR_MENU_ARROWS);
    line_menu_arrw_up_2_dsc.width = ARROW_LINE_WIDTH_PX;
    line_menu_arrw_up_2_dsc.p1.x = MENU_LEFT_LINE_H_PX + (LCD_H_RES - MENU_LEFT_LINE_H_PX) / 2;
    line_menu_arrw_up_2_dsc.p1.y = MENU_1ST_LINE_V_PX + (MENU_2ND_LINE_V_PX - MENU_1ST_LINE_V_PX - ARROW_HEIGHT_PX) / 2;
    line_menu_arrw_up_2_dsc.p2.x = line_menu_arrw_up_2_dsc.p1.x + ARROW_WIDTH_PX;
    line_menu_arrw_up_2_dsc.p2.y = line_menu_arrw_up_2_dsc.p1.y + ARROW_HEIGHT_PX / 2;
    lv_draw_line(&layer, &line_menu_arrw_up_2_dsc);

    lv_draw_line_dsc_t line_menu_arrw_up_3_dsc;
    lv_draw_line_dsc_init(&line_menu_arrw_up_3_dsc);
    line_menu_arrw_up_3_dsc.color = lv_color_make(COLOR_MENU_ARROWS);
    line_menu_arrw_up_3_dsc.width = ARROW_LINE_WIDTH_PX;
    line_menu_arrw_up_3_dsc.p1.x = MENU_LEFT_LINE_H_PX + (LCD_H_RES - MENU_LEFT_LINE_H_PX) / 2;
    line_menu_arrw_up_3_dsc.p1.y = MENU_1ST_LINE_V_PX + (MENU_2ND_LINE_V_PX - MENU_1ST_LINE_V_PX - ARROW_HEIGHT_PX) / 2;
    line_menu_arrw_up_3_dsc.p2.x = line_menu_arrw_up_3_dsc.p1.x - ARROW_WIDTH_PX;
    line_menu_arrw_up_3_dsc.p2.y = line_menu_arrw_up_3_dsc.p1.y + ARROW_HEIGHT_PX / 2;
    lv_draw_line(&layer, &line_menu_arrw_up_3_dsc);

    //arrow down
    lv_draw_line_dsc_t line_menu_arrw_down_1_dsc;
    lv_draw_line_dsc_init(&line_menu_arrw_down_1_dsc);
    line_menu_arrw_down_1_dsc.color = lv_color_make(COLOR_MENU_ARROWS);
    line_menu_arrw_down_1_dsc.width = ARROW_LINE_WIDTH_PX;
    line_menu_arrw_down_1_dsc.p1.x = MENU_LEFT_LINE_H_PX + (LCD_H_RES - MENU_LEFT_LINE_H_PX) / 2;
    line_menu_arrw_down_1_dsc.p1.y = MENU_2ND_LINE_V_PX + (MENU_3RD_LINE_V_PX - MENU_2ND_LINE_V_PX - ARROW_HEIGHT_PX) / 2;
    line_menu_arrw_down_1_dsc.p2.x = line_menu_arrw_down_1_dsc.p1.x;
    line_menu_arrw_down_1_dsc.p2.y = line_menu_arrw_down_1_dsc.p1.y + ARROW_HEIGHT_PX;
    lv_draw_line(&layer, &line_menu_arrw_down_1_dsc);

    lv_draw_line_dsc_t line_menu_arrw_down_2_dsc;
    lv_draw_line_dsc_init(&line_menu_arrw_down_2_dsc);
    line_menu_arrw_down_2_dsc.color = lv_color_make(COLOR_MENU_ARROWS);
    line_menu_arrw_down_2_dsc.width = ARROW_LINE_WIDTH_PX;
    line_menu_arrw_down_2_dsc.p1.x = MENU_LEFT_LINE_H_PX + (LCD_H_RES - MENU_LEFT_LINE_H_PX) / 2;
    line_menu_arrw_down_2_dsc.p1.y = MENU_3RD_LINE_V_PX - (MENU_3RD_LINE_V_PX - MENU_2ND_LINE_V_PX - ARROW_HEIGHT_PX) / 2;
    line_menu_arrw_down_2_dsc.p2.x = line_menu_arrw_down_2_dsc.p1.x + ARROW_WIDTH_PX;
    line_menu_arrw_down_2_dsc.p2.y = line_menu_arrw_down_2_dsc.p1.y - ARROW_HEIGHT_PX / 2;
    lv_draw_line(&layer, &line_menu_arrw_down_2_dsc);

    lv_draw_line_dsc_t line_menu_arrw_down_3_dsc;
    lv_draw_line_dsc_init(&line_menu_arrw_down_3_dsc);
    line_menu_arrw_down_3_dsc.color = lv_color_make(COLOR_MENU_ARROWS);
    line_menu_arrw_down_3_dsc.width = ARROW_LINE_WIDTH_PX;
    line_menu_arrw_down_3_dsc.p1.x = MENU_LEFT_LINE_H_PX + (LCD_H_RES - MENU_LEFT_LINE_H_PX) / 2;
    line_menu_arrw_down_3_dsc.p1.y = MENU_3RD_LINE_V_PX - (MENU_3RD_LINE_V_PX - MENU_2ND_LINE_V_PX - ARROW_HEIGHT_PX) / 2;
    line_menu_arrw_down_3_dsc.p2.x = line_menu_arrw_down_3_dsc.p1.x - ARROW_WIDTH_PX;
    line_menu_arrw_down_3_dsc.p2.y = line_menu_arrw_down_2_dsc.p1.y - ARROW_HEIGHT_PX / 2;
    lv_draw_line(&layer, &line_menu_arrw_down_3_dsc);

    // - - - - - - - - - - - MENU -> labels (texts)

    // 4th button label
    lv_draw_label_dsc_t text_start_stop_dsc;
    lv_draw_label_dsc_init(&text_start_stop_dsc);
    text_start_stop_dsc.color = lv_color_make(COLOR_WHITE);
    text_start_stop_dsc.text = get_playing() ? "STOP" : "START";
    text_start_stop_dsc.align = LV_TEXT_ALIGN_CENTER;
    lv_area_t coords_text_start_stop = {MENU_LEFT_LINE_H_PX, MENU_4TH_TEXT_V_PX, LCD_H_RES, MENU_5TH_LINE_V_PX};
    lv_draw_label(&layer, &text_start_stop_dsc, &coords_text_start_stop);
    
    // 5th button label
    lv_draw_label_dsc_t text_vol_up_dsc;
    lv_draw_label_dsc_init(&text_vol_up_dsc);
    text_vol_up_dsc.color = lv_color_make(COLOR_WHITE);
    text_vol_up_dsc.text = "VOL+";
    text_vol_up_dsc.align = LV_TEXT_ALIGN_CENTER;
    lv_area_t coords_text_vol_up = {MENU_LEFT_LINE_H_PX, MENU_5TH_TEXT_V_PX, LCD_H_RES, MENU_5TH_LINE_V_PX};
    lv_draw_label(&layer, &text_vol_up_dsc, &coords_text_vol_up);

    // 6th button label
    lv_draw_label_dsc_t text_vol_down_dsc;
    lv_draw_label_dsc_init(&text_vol_down_dsc);
    text_vol_down_dsc.color = lv_color_make(COLOR_WHITE);
    text_vol_down_dsc.text = "VOL-";
    text_vol_down_dsc.align = LV_TEXT_ALIGN_CENTER;
    lv_area_t coords_text_vol_down = {MENU_LEFT_LINE_H_PX, MENU_6TH_TEXT_V_PX, LCD_H_RES, LCD_V_RES};
    lv_draw_label(&layer, &text_vol_down_dsc, &coords_text_vol_down);


    lv_canvas_finish_layer(canvas, &layer);

    swap_color_bytes();
}
