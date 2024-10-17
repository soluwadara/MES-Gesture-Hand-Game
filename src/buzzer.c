/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/sensor.h>
#include <stdlib.h>
#include <time.h>
#include "buzzer.h"
#include "imu.h"
#include "utils.h"
#include "led.h"
#include "state_machine.h"

#define BUZZER_STACK                1024
#define STARTUP_NOTES               15
#define GOODBYE_NOTES               15
#define CONGRATES_NOTES             13
#define SEVEN_NOTES                 7
#define SIX_NOTES                   6
#define LOWER_BOUND                 0
#define UPPER_BOUND                 5
#define LEVEL_ONE_NOTES             10
#define LEVEL_TWO_NOTES             16
#define LEVEL_THREE_NOTES           22
#define HIGH_SCORE_LEVEL_ONE        5
#define HIGH_SCORE_LEVEL_TWO        8  
#define HIGH_SCORE_LEVEL_THREE      11

static const struct pwm_dt_spec sBuzzer = PWM_DT_SPEC_GET(DT_ALIAS(pwm_buzzer));
const char *ORIENTATION_STRING[SEVEN_NOTES] = {
    "UP", "DOWN", "RIGHT", "LEFT","RIGHTSIDE_UP", "UPSIDE_DOWN", "INVALID",
};

enum song_choice
{
    silence,
    startup_sound,
    congrats_sound,
    goodbye_sound,
    test
};
struct notes_highscore
{
    int notes_per_level;
    int highest_score;
};

struct note_duration
{   
    int note; // hz
    int duration; //
};

struct note_side
{   
    int note;
    int side;
    int duration; 
};

struct notes_highscore level1_scoreboard = {
    .notes_per_level= LEVEL_ONE_NOTES, .highest_score = HIGH_SCORE_LEVEL_ONE
};
struct notes_highscore level2_scoreboard = {
    .notes_per_level= LEVEL_TWO_NOTES,.highest_score = HIGH_SCORE_LEVEL_TWO
};
struct notes_highscore level3_scoreboard = {
    .notes_per_level= LEVEL_THREE_NOTES,.highest_score = HIGH_SCORE_LEVEL_THREE
};

struct note_side user_action[SIX_NOTES] = {
    {.note = A4, .side = up, .duration = whole },
    {.note = B4, .side = down, .duration = whole},
    {.note = C5, .side = right, .duration = whole},
    {.note = D5, .side = left, .duration = whole},
    {.note = E5, .side = rightside_up, .duration = whole},
    {.note = F5, .side = upside_down, .duration = whole}
};

struct note_duration startup_song[STARTUP_NOTES] = {
    {.note= F4, .duration = quarter},
    {.note= F4, .duration = quarter},
    {.note= G4, .duration = quarter},
    {.note= A4, .duration = quarter},
    {.note= Bb4, .duration = quarter},
    {.note= G4, .duration = quarter},
    {.note= F4, .duration = quarter},
    {.note= Bb4, .duration = quarter},
    {.note= Bb4, .duration = quarter},
    {.note= C5, .duration = quarter},
    {.note= D5, .duration = quarter},
    {.note= Bb4, .duration = quarter},
    {.note= C5, .duration = quarter},
    {.note= A4, .duration = quarter},
    {.note= Bb4, .duration = half_note}
};

struct note_duration goodbye_song[GOODBYE_NOTES] = {
    {.note= F4, .duration = eigth},
    {.note= F4, .duration = eigth},
    {.note= G4, .duration = eigth},
    {.note= A4, .duration = eigth},
    {.note= Bb4, .duration = eigth},
    {.note= G4, .duration = eigth},
    {.note= F4, .duration = eigth},
    {.note= Bb4, .duration = eigth},
    {.note= Bb4, .duration = eigth},
    {.note= C5, .duration = eigth},
    {.note= D5, .duration = eigth},
    {.note= Bb4, .duration = eigth},
    {.note= C5, .duration = eigth},
    {.note= A4, .duration = eigth},
    {.note= Bb4, .duration = quarter}
};

struct note_duration congrats_song[CONGRATES_NOTES] = {
    {.note= G4, .duration = quarter},
    {.note= G4, .duration = quarter},
    {.note= A4, .duration = eigth},
    {.note= Bb4, .duration = half_note},
    {.note= F4, .duration = quarter},
    {.note= F4, .duration = quarter},
    {.note= F4, .duration = quarter},
    {.note= C5, .duration = quarter},
    {.note= C5, .duration = quarter},
    {.note= C5, .duration = quarter},
    {.note= Bb4, .duration = eigth},
    {.note= A4, .duration = quarter},
    {.note= G4, .duration = whole}
};

enum song_choice eSong = silence; 
uint8_t tone_done = 0; 
volatile uint8_t user_won =0;

K_SEM_DEFINE(buzzer_initialize_sem, 0,1); // wait until buzzer is initiailzed 
K_MUTEX_DEFINE(tone_done_mutex); // wait until buzzer is initiailzed 
K_MUTEX_DEFINE(play_game_mutex); 

void device_side_translation()
{
    printk("rotated to side:%s \r\n", ORIENTATION_STRING[device_side]);
}

void buzzer_thread_func(void *d0,void *d1,void *d2)
{
    /* Block until buzzer is available */
    k_sem_take(&buzzer_initialize_sem,K_FOREVER); 
    while(1) 
    {
        switch (eSong)
        {
            case silence: 
                pwm_set_pulse_dt(&sBuzzer, 0);
                k_sleep(K_FOREVER);
                break;
            case startup_sound:
                printk("starting sound\r\n");
                k_mutex_lock(&tone_done_mutex, K_FOREVER);
                for (int i = 0; i < STARTUP_NOTES;i++)
                {
                    pwm_set_dt(&sBuzzer,PWM_HZ(startup_song[i].note),PWM_HZ(startup_song[i].note)/2U); 
                    k_msleep(startup_song[i].duration); // not sleeping the processor it's sleeping the thread. it's started the PWM. 
                }
                pwm_set_pulse_dt(&sBuzzer, 0);
                printk("in buzzer.done with start up tone\r\n");
                k_mutex_unlock(&tone_done_mutex);
                wake_state_machine_thd();
                k_sleep(K_FOREVER); // sleep the buzzer thread
                break;
            case goodbye_sound: 
                k_mutex_lock(&tone_done_mutex, K_FOREVER);
                for (int i = 0; i < GOODBYE_NOTES;i++)
                {
                    pwm_set_dt(&sBuzzer,PWM_HZ(goodbye_song[i].note),PWM_HZ(goodbye_song[i].note)/2U); 
                    k_msleep(goodbye_song[i].duration);
                }
                pwm_set_pulse_dt(&sBuzzer, 0);
                printk("in buzzer.done with goodbye tone\r\n");
                k_mutex_unlock(&tone_done_mutex);
                wake_state_machine_thd();
                k_sleep(K_FOREVER);
                break;
            case test: 
                k_mutex_lock(&tone_done_mutex, K_FOREVER);
                printk("side: %s\n", ORIENTATION_STRING[device_side]);
                for (int i = 0; i < SIX_NOTES;i++)
                {
                    if (device_side == user_action[i].side)
                    {
                        pwm_set_dt(&sBuzzer,PWM_HZ(user_action[i].note),PWM_HZ(user_action[i].note)/2U); 
                        k_msleep(user_action[i].duration);
                    }
                    else pwm_set_pulse_dt(&sBuzzer, 0);
                }
                pwm_set_pulse_dt(&sBuzzer, 0);
                k_mutex_unlock(&tone_done_mutex);
                break;
            case congrats_sound:
                k_mutex_lock(&tone_done_mutex, K_FOREVER);
                for (int i = 0; i < CONGRATES_NOTES;i++)
                {
                    pwm_set_dt(&sBuzzer,PWM_HZ(congrats_song[i].note),PWM_HZ(congrats_song[i].note)/2U); 
                    k_msleep(congrats_song[i].duration); // not sleeping the processor it's sleeping the thread. it's started the PWM. 
                }
                pwm_set_pulse_dt(&sBuzzer, 0);
                printk("in buzzer.done with congrats_sound\r\n");
                k_mutex_unlock(&tone_done_mutex);
                wake_state_machine_thd();
                k_sleep(K_FOREVER);
                break;
        }
        
    }
}
void play_level(struct notes_highscore sNotes_Higherscore)
{   
    uint8_t user_score = 0;
    int i = 0;
    srand(time(NULL)); // initialization of random num generator 
    int rand_num;
    k_mutex_lock(&play_game_mutex, K_FOREVER);
    while(i < sNotes_Higherscore.notes_per_level)
    {
        rand_num = (rand() % (UPPER_BOUND - LOWER_BOUND + 1)) + LOWER_BOUND;
        pwm_set_dt(&sBuzzer,PWM_HZ(user_action[rand_num].note),PWM_HZ(user_action[rand_num].note)/2U);
        k_msleep(user_action[rand_num].duration);
        printk("PLAYED TONE. rotate to side:%s \r\n", ORIENTATION_STRING[user_action[rand_num].side]);
        pwm_set_pulse_dt(&sBuzzer, 0);
        k_msleep(2500);
        imu_data_processing();
        k_msleep(500);
        printk("rotated to side:%s \r\n", ORIENTATION_STRING[device_side]);
        if (device_side == user_action[rand_num].side)
        {
            printk("Matched!\r\n");
            user_score++;
        }
        else {
            printk("Did not match!\r\n");
            led_error();
          //  pwm_set_dt(&sBuzzer,PWM_HZ(C4),PWM_HZ(C4)/2U);
           // k_msleep(eigth);
            //pwm_set_pulse_dt(&sBuzzer, 0);
            //pwm_set_dt(&sBuzzer,PWM_HZ(C4),PWM_HZ(C4)/2U);
           // k_msleep(eigth);
           // pwm_set_pulse_dt(&sBuzzer, 0);
            led_off();
        }
        if (user_score >= sNotes_Higherscore.highest_score)
        {
            printk("User won!\r\n");
            user_won = 1;
        }
        else 
        {
            printk("still playing!\r\n");
            i++;
        }
    }
    i = 0;
    user_score = 0;
    printk("played the game..waking up state machine\r\n");
    k_mutex_lock(&play_game_mutex, K_FOREVER);
    //wake_state_machine_thd(); // call the thread before going to sleep 
   // k_sleep(K_FOREVER); // frees up whatever thread is ready to run
}

K_THREAD_DEFINE(buzzer_thd,BUZZER_STACK,buzzer_thread_func,NULL,NULL,NULL,THREAD1_PRIORITY,0,5000);

void play_startup_song()
{
    eSong = startup_sound;
    k_wakeup(buzzer_thd);
    k_sleep(K_FOREVER); // sleep SM
}
void play_goodbye_song()
{
    eSong = goodbye_sound;
    k_wakeup(buzzer_thd);
    k_sleep(K_FOREVER);
}
void play_congrats_song()
{
    eSong = congrats_sound;
    k_wakeup(buzzer_thd);
    k_sleep(K_FOREVER); // sleep the SM
}
void play_test()
{
    eSong = test;
    k_wakeup(buzzer_thd);
    k_sleep(K_FOREVER);
}
void play_silence()
{
    eSong = silence;
    k_wakeup(buzzer_thd);
    k_sleep(K_FOREVER);
}

void wake_buzzer_thd()
{
    k_wakeup(buzzer_thd);
}

void play_level_one()
{
    play_level(level1_scoreboard); // SM is not asleep here
}
void play_level_two()
{
    play_level(level2_scoreboard); // SM is not asleep here
}
void play_level_three()
{
    play_level(level3_scoreboard); // SM is not asleep here
}
static int buzzer_init()
{
    int ret = 0;
    if (!device_is_ready(sBuzzer.dev))
	{                          
		printk("Device %s is not ready.\r\n",sBuzzer.dev->name);
        ret = 1;
	}

    k_sem_give(&buzzer_initialize_sem);
    k_sleep(K_FOREVER); // frees up whatever thread is ready to run
    return 0;
}

SYS_INIT(buzzer_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
