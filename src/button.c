/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include "button.h"
#include "state_machine.h"

#define QUICK_PRESS_DEBOUNCE_TIME_MS   10

uint8_t num_presses = 0;
volatile uint8_t accept_button_presses = 0;

static const struct gpio_dt_spec sUser_btn = GPIO_DT_SPEC_GET(DT_ALIAS(sw0),gpios);
static struct gpio_callback button_cb_data;
static struct k_work timer_work;
static struct k_timer debounce_timer;

void button_debounce_workq_submit() // when timer expires submit for work to be done
{
    if (k_timer_remaining_get(&debounce_timer) == 0){
        k_work_submit(&timer_work);
    }
}
void button_workq()
{
    if (num_presses < 3){
        num_presses++;
    }
    else num_presses = 0;
}
static void button_press_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    k_timer_start(&debounce_timer, K_MSEC(QUICK_PRESS_DEBOUNCE_TIME_MS), K_MSEC(0));
}

static void configure_button_interrupt()
{
    gpio_init_callback(&button_cb_data,button_press_isr,BIT(sUser_btn.pin));
	gpio_add_callback(sUser_btn.port, &button_cb_data);
}

static void configure_button_timer()
{
     /* Initialize button debounce timer work queue */
    k_work_init(&timer_work,button_workq);

    /* Initialize button debounce timer */
    k_timer_init(&debounce_timer,button_debounce_workq_submit, NULL);
}

static void button_gpio_init()
{
    int err; 
    if (!device_is_ready(sUser_btn.port)){
        printk("Error %d: failed to configure %s pin %d\n",
        err, sUser_btn.port->name,sUser_btn.pin);
    }

    err = gpio_pin_configure_dt(&sUser_btn,GPIO_INPUT);
	if (err != 0){
		printk("Error %d: failed to configure %s pin %d\n",
		err, sUser_btn.port->name, sUser_btn.pin);
	}

    err = gpio_pin_interrupt_configure_dt(&sUser_btn, GPIO_INT_EDGE_TO_ACTIVE);
	if (err !=0){
		printk("Error %d: failed to configure interrupt %s pin %d\n",
		err, sUser_btn.port->name, sUser_btn.pin);
	}
}

static int button_init()
{
    button_gpio_init();
    configure_button_timer();
    configure_button_interrupt();
    return 0;
}

SYS_INIT(button_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
