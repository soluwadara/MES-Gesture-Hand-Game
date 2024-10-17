/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include "buzzer.h"
#include "state_machine.h"
#include "imu.h"
#include "led.h"


int main(void)
{
	return 0;
}

// led is up: -9 And led is down: +9 looking for 9 -- gravity 9 m/s^2
// when there is no movement G should be close to 0. when there is movement and accel is settling 
// G should not be 0. The sum of squares of A is consistent if i'm not moving. This is the magnatitude
//
