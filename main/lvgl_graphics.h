/*
    Header file - graphics to be print on LCD
*/

#ifndef _LVGL_GRAPHICS_H_
#define _LVGL_GRAPHICS_H_

#include "lvgl.h"
#include "hw_init.h"
#include "common.h"

#define VOLUME_MAX 100

// initialize user interface
void ui_init(void);

// print actual graphics on LCD
void display_graphics(lv_display_t *disp);



#endif