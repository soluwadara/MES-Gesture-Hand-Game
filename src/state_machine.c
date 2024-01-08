/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include "state_machine.h"
#include "led.h"
#include "button.h"
#include "imu.h"
#include "buzzer.h"
#include "utils.h"

#define STATE_MACHINE_STACK     1024 
static struct k_work select_level_work;
static struct k_timer select_level_timer;
enum state currentState = START;

K_SEM_DEFINE(state_machine_sem, 0,1); // wait until buzzer is initiailzed 

int state_machine_init()
{
    /* work job for selecting level */
    k_work_init(&select_level_work,select_level_workq);
    /* Initialize timer for selecting a level */
    k_timer_init(&select_level_timer,select_level_workq_submit, NULL);
    k_sem_give(&state_machine_sem);
    return 0;
}

void state_machine_func(void *d0,void *d1,void *d2)
{
    /* Block until state_machine is initialized */
    k_sem_take(&state_machine_sem,K_FOREVER); 
    while (1)
    {
        switch (currentState)
        {
            case START:
                led_start();
                k_msleep(1000);
                led_blinky();
           /*     play_startup_song();
                next_state(ePICKLEVEL);
                break;
            case ePICKLEVEL:
                printk("SM: pick level \r\n");
                led_blinky();
                k_timer_start(&select_level_timer, K_MSEC(8000), K_MSEC(0)); // 10s to pick a level
                break;
           /* case eLEVEL1:  
                printk("SM:Level 1!\r\n");
                play_level_one(); // this is part of the state machine thread because it's called from the SM. 
                // it dosen't matter what file it is called from it matters where it is called from 
                printk("Return from playing LEVEL 1!\r\n");
                if (user_won)
                {
                    play_congrats_song();
                    user_won = 0;
                    next_state(eLEVEL2);
                }
                else 
                {
                    next_state(END);
                }
                break;
            case eLEVEL2:
                printk("SM:Level 2!\r\n");
                play_level_two();
                printk("Return from playing LEVEL 2!\r\n");
                if (user_won)
                {
                    play_congrats_song();
                    user_won = 0;
                    next_state(eLEVEL3);
                }
                else 
                {
                    next_state(END);
                }
                break;
            case eLEVEL3:
                printk("SM:Level 3!\r\n");
                play_level_three();
                printk("Return from playing LEVEL 3!\r\n");
                if (user_won)
                {
                    play_congrats_song();
                    user_won = 0;
                    next_state(END);
                }
                else 
                {
                    next_state(END);
                }
            case END:
                play_goodbye_song();
                break; */
        } 
    }
}

K_THREAD_DEFINE(state_machine_thd,STATE_MACHINE_STACK,state_machine_func,NULL,NULL,NULL,THREAD0_PRIORITY ,0,5000);

void next_state(enum state nextState)
{
    switch(nextState)
    {
        case START: 
            break;
        case ePICKLEVEL:
            printk("Next state:PICK_LEVEL!\r\n");
            currentState = ePICKLEVEL;
            k_wakeup(state_machine_thd); // to keep it from sleeping when it returns from this function and breaks.
            break;
        case eLEVEL1:
            printk("Next state:LEVEL 1!\r\n");
            currentState = eLEVEL1;
            k_wakeup(state_machine_thd);
            break;
        case eLEVEL2:
            currentState = eLEVEL2;
            k_wakeup(state_machine_thd);
            break;
        case eLEVEL3:
            currentState = eLEVEL3;
            k_wakeup(state_machine_thd);
            break;
        case END:
            printk("Next state:END!\r\n");
            currentState = END;
           // k_wakeup(state_machine_thd);
            break;
    }
}

void select_level_workq_submit() // when timer expires submit for work to be done
{
    if (k_timer_remaining_get(&select_level_timer) == 0){
        k_work_submit(&select_level_work);
    }
}

void select_level_workq()
{
     if (num_presses == 1){  
        //printk("Selected level 1!\r\n"); 
        next_state(eLEVEL1);
    }
    if (num_presses == 2){
        //printk("Selected level 2!\r\n"); 
        next_state(eLEVEL1);
    }
    if (num_presses == 3){
        // printk("Selected level 3!\r\n"); 
        next_state(eLEVEL3);
    }
}

void wake_state_machine_thd()
{
    k_wakeup(state_machine_thd);
}

SYS_INIT(state_machine_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
