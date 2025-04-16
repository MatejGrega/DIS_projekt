#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>
#include <stdbool.h>

void set_sound_level(uint16_t sound_level);

uint16_t get_sound_level(void);

void set_volume(uint8_t volume);

uint8_t get_volume(void);

void set_playing(bool playing);

bool get_playing(void);

void set_lcd_update_flag(void);

bool get_lcd_update_flag(void);



#endif