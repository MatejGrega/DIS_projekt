/*
    Header file - graphics to be print on LCD
*/

#ifndef _LVGL_GRAPHICS_H_
#define _LVGL_GRAPHICS_H_

#include "lvgl.h"
#include "hw_init.h"
#include "common.h"

#define VOLUME_MAX 100

void ui_init(void);

void display_graphics(lv_display_t *disp);



#endif