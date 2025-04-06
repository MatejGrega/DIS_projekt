/*
    Graphics of application
*/

#include "lvgl.h"

#define CANVAS_HEIGHT 200
#define CANVAS_WIDTH 300

lv_color_t *canvas_buffer = NULL;
lv_obj_t *canvas = NULL;

void display_graphics(lv_display_t *disp, uint32_t number)
{
    if(canvas_buffer == NULL){
        canvas_buffer = malloc(CANVAS_WIDTH * CANVAS_HEIGHT * sizeof(lv_color_t));   //alocate static buffer just once
    }
    if(canvas == NULL){
        lv_obj_t *scr = lv_display_get_screen_active(disp);
        canvas = lv_canvas_create(scr);
        lv_canvas_set_buffer(canvas, canvas_buffer, CANVAS_WIDTH, CANVAS_HEIGHT, LV_COLOR_FORMAT_RGB888);
        lv_obj_center(canvas);
    }
    lv_canvas_fill_bg(canvas, lv_color_make(number%5*63,0,0), LV_OPA_100);

    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);

    lv_draw_label_dsc_t text_dsc;
    lv_draw_label_dsc_init(&text_dsc);
    text_dsc.color = lv_color_make(0,0,0);
    text_dsc.text = "Hello";
    lv_area_t coords = {10, 10, 130, 60};
    lv_draw_label(&layer, &text_dsc, &coords);
    lv_canvas_finish_layer(canvas, &layer);

    //swap color bytes to match displayed colors
    uint8_t *canvas_buffer_uint8_t = (uint8_t*)canvas_buffer;
    for (int i = 0; i < CANVAS_WIDTH * CANVAS_HEIGHT; i++) {
        uint8_t r = canvas_buffer_uint8_t[i * 3];
        uint8_t g = canvas_buffer_uint8_t[i * 3 + 1];
        uint8_t b = canvas_buffer_uint8_t[i * 3 + 2];

        // swap R <-> B
        canvas_buffer_uint8_t[i * 3]     = b;
        canvas_buffer_uint8_t[i * 3 + 1] = g;
        canvas_buffer_uint8_t[i * 3 + 2] = r;
    }
}
