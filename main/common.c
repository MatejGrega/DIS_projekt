/*
    Definitions and variables used to comunicate between files.
*/

#include "common.h"

uint16_t __sound_level = 0;
uint8_t __volume = 50;   //0 - 100
bool __playing = false;
bool __update_lcd = true;
uint16_t __audio_freq = AUDIO_FREQUENCY_MIN;
uint8_t __audio_freq_digit = 0;

inline void set_sound_level(uint16_t sound_level){
    __sound_level = sound_level;
}

inline uint16_t get_sound_level(void){
    return __sound_level;
}

inline void set_volume(uint8_t volume){
    __volume = volume;
}

inline uint8_t get_volume(void){
    return __volume;
}

inline void set_playing(bool playing){
    __playing = playing;
}

inline bool get_playing(void){
    return __playing;
}

inline void set_lcd_update_flag(void){
    __update_lcd = true;
}

inline bool get_lcd_update_flag(void){
    return __update_lcd;
}

inline void set_audio_frequency(uint16_t freq){
    __audio_freq = freq;
}

inline uint16_t get_audio_frequency(void){
    return __audio_freq;
}

inline void set_audio_freq_digit(uint8_t digit){
    __audio_freq_digit = digit;
}

inline uint8_t get_audio_freq_digit(void){
    return __audio_freq_digit;
}