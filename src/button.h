/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <zephyr/kernel.h>

void button_debounce_workq_submit();
void button_workq();

extern uint8_t num_presses;


#endif
