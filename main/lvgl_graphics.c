/*
    Graphics of application
*/

#include "lvgl_graphics.h"

#define UI_TASK_STACK_SIZE 1024
#define UI_TASK_PRIORITY 0
#define COLOR_WHITE 255,255,255

static void __user_interface_task(void *args){
    button_adc_t btn_state_last = BUTTON_NONE;
    button_adc_t btn_state_actual = BUTTON_NONE;

    while(1){
        btn_state_actual = hw_get_buttons();
        if(btn_state_actual != btn_state_last){
            set_lcd_update_flag();
            if(btn_state_last == BUTTON_NONE){
                switch(btn_state_actual){
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
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void ui_init(void){
    xTaskCreate(__user_interface_task, "buttons", UI_TASK_STACK_SIZE, NULL, UI_TASK_PRIORITY, NULL);
}

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


#define COLOR_MENU_INACTIVE 0,0,10
#define COLOR_MENU_ACTIVE 255,180,0
#define COLOR_MENU_LINES 255, 255, 0
#define COLOR_MENU_ARROWS COLOR_WHITE

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

#define ARROW_HEIGHT_PX 25
#define ARROW_WIDTH_PX 7
#define ARROW_LINE_WIDTH_PX 2
#define ARROW_SIDES_WIDTH_PX 35
#define ARROW_SIDES_HEAD_LENGTH 10
#define ARROW_SIDES_HEAD_WIDTH 7

void display_graphics(lv_display_t *disp)
{
    lv_obj_t *canvas = hw_init_get_canvas();
    lv_canvas_fill_bg(canvas, lv_color_make(0, 0, 0), LV_OPA_100);

    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);

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



    lv_draw_label_dsc_t cislo_dsc;
    lv_draw_label_dsc_init(&cislo_dsc);
    cislo_dsc.color = lv_color_make(COLOR_WHITE);
    char chbuff[100] = {0};
    itoa((int)get_volume(), chbuff, 10);
    cislo_dsc.text = chbuff;
    lv_area_t coords2 = {0, 0, LCD_H_RES, LCD_V_RES};
    lv_draw_label(&layer, &cislo_dsc, &coords2);

    lv_canvas_finish_layer(canvas, &layer);

    swap_color_bytes();
}
