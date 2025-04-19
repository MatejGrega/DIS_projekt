#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>
#include <stdbool.h>

#define AUDIO_FREQUENCY_MIN 50
#define AUDIO_FREQUENCY_MAX 8000

void set_sound_level(uint16_t sound_level);

uint16_t get_sound_level(void);

void set_volume(uint8_t volume);

uint8_t get_volume(void);

void set_playing(bool playing);

bool get_playing(void);

void set_lcd_update_flag(void);

void reset_lcd_update_flag(void);

bool get_lcd_update_flag(void);

void set_audio_frequency(uint16_t freq);

uint16_t get_audio_frequency(void);

void set_audio_freq_digit(uint8_t digit);

uint8_t get_audio_freq_digit(void);



#endif