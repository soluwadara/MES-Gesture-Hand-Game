/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include "state_machine.h"
#include "led.h"
#include "utils.h"

#define LED_ON                  1
#define LED_OFF                 0
#define ALIVE_LED_PERIOD        500
#define LED_STACK               500 

static const struct gpio_dt_spec sLED_Grn = GPIO_DT_SPEC_GET(DT_ALIAS(led1),gpios);
static const struct gpio_dt_spec sLED_Red = GPIO_DT_SPEC_GET(DT_ALIAS(led2),gpios);
enum eLED_state LED_state;

K_SEM_DEFINE(led_initialize_sem,0,1); // wait until LED is initiailzed 

int led_init()
{
    int err;
    if (!device_is_ready(sLED_Grn.port)){
		printk("Error %d: failed to configure %s pin %d\n",
		err, sLED_Grn.port->name,sLED_Grn.pin);
	}
	err = gpio_pin_configure_dt(&sLED_Grn, GPIO_OUTPUT_INACTIVE);
	if (err != 0){
		printk("Error %d: failed to configure %s pin %d\n",
		err, sLED_Grn.port->name, sLED_Grn.pin);
	}
    if (!device_is_ready(sLED_Red.port)){
		printk("Error %d: failed to configure %s pin %d\n",
		err, sLED_Red.port->name,sLED_Red.pin);
	}
	err = gpio_pin_configure_dt(&sLED_Red, GPIO_OUTPUT_INACTIVE);
	if (err != 0){
		printk("Error %d: failed to configure %s pin %d\n",
		err, sLED_Red.port->name, sLED_Red.pin);
	}
    k_sem_give(&led_initialize_sem);
    return err;
}

void led_thread_func(void *d0,void *d1,void *d2){
 
    k_sem_take(&led_initialize_sem,K_FOREVER); 
    printk("Semaphore taken in LED \r\n");
    uint32_t cnt = 0;
    while (1)
    {
        switch (LED_state)
        {
            case eLED_off:
                gpio_pin_set_dt(&sLED_Grn,LED_OFF);
                gpio_pin_set_dt(&sLED_Red,LED_OFF);
                wake_state_machine_thd();
                k_sleep(K_FOREVER); // the LED thread will go to sleep
                break;
            case eLED_on:
                gpio_pin_set_dt(&sLED_Grn,LED_ON);
                wake_state_machine_thd();
                k_sleep(K_FOREVER); // the LED thread will go to sleep
                break;
            case eLED_blinky: 
                gpio_pin_set_dt(&sLED_Grn,cnt%2);
                k_msleep(ALIVE_LED_PERIOD/2);
                cnt++; 
                break; 
            case eLED_error:
                gpio_pin_set_dt(&sLED_Red,LED_ON);
                wake_state_machine_thd();
                k_sleep(K_FOREVER); // the LED thread will go to sleep
                break;
        }
    }
}

K_THREAD_DEFINE(g_alive_thd,LED_STACK,led_thread_func,NULL,NULL,NULL,THREAD3_PRIORITY,0,5000);

void led_off()
{
    LED_state = eLED_off;
    k_wakeup(g_alive_thd);
    k_sleep(K_FOREVER); // turn of SM thread
    // turn off the current thread so that the LED thread can run or make it go to sleep for some time
}
void led_start()
{   
    LED_state = eLED_on;
    k_wakeup(g_alive_thd);
    k_sleep(K_FOREVER);
    // turn off the current thread so that the LED thread can run or make it go to sleep for some time
}
void led_blinky()
{
    LED_state = eLED_blinky;
    k_wakeup(g_alive_thd); // not sleep SM because want to blink while waiting for user input
    k_sleep(K_FOREVER); // turn of SM thread
}
void led_error()
{
    LED_state = eLED_error;
    k_wakeup(g_alive_thd); 
    k_sleep(K_FOREVER); // turn off the current thread 
}
SYS_INIT(led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
