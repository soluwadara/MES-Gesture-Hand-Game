/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _LED_H_
#define _LED_H_

enum eLED_state
{
    eLED_off,
    eLED_on,
    eLED_blinky,
    eLED_error
};
extern enum eLED_state LED_state;
extern void led_off();
extern void led_start();
extern void led_blinky();
extern void led_error();

#endif
