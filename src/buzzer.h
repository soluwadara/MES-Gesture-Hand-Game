#ifndef _BUZZER_H_
#define _BUZZER_H_

#include "utils.h"

extern void play_startup_song();
extern void play_congrats_song();
extern void play_goodbye_song();
extern void play_goodbye_song();
extern void play_test();
extern void play_silence();
extern void play_level_one();
extern void play_level_two();
extern void play_level_three();
extern void wake_buzzer_thd();
extern uint8_t tone_done;
extern volatile uint8_t user_won;

enum orientation 
{
    up,
    down,
    right,
    left,
    rightside_up,
    upside_down, 
    invalid_side,
    invalid_gyr
};
extern enum orientation eSide;

#define whole       600 //whole note
#define eigth       75 //eigth note
#define quarter     150
#define half_note   300

#define C4  262
#define F4  349
#define G4  392
#define A4  440
#define Bb4 466
#define B4  494
#define C5  523
#define D5  587  
#define E5  659
#define F5  698
#define G5  784
#define A5  880

#define REST 1

#endif
