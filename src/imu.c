/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include "imu.h"
#include "buzzer.h"
#include "state_machine.h"

#define IMU_STACK    1024
#define GRAVITY         9
#define NEG_GRAVITY    -9

static const struct device *sIMU_device = DEVICE_DT_GET_ONE(bosch_bmi270);
static struct sensor_value acc[3], gyr[3];
static struct sensor_value full_scale,sampling_freq,oversampling;
int device_side;

K_SEM_DEFINE(imu_initialize_sem,0,1); // wait until buzzer is initiailzed 
K_MUTEX_DEFINE(imu_processing_mutex); // wait until buzzer is initiailzed 

static int imu_init()
{
    int err;
    if (!device_is_ready(sIMU_device))
    {
		printk("Error %d: failed to configure %s\n", err, sIMU_device->name);
	}

    full_scale.val1 = 2;            /* G maximum it can read. it's range/scale */ 
    full_scale.val2 = 0;
    sampling_freq.val1 = 15;       /* Hz. Performance mode. Faster = more sampling/note. Don't want to move fast */
    sampling_freq.val2 = 0;
    oversampling.val1 = 1;          /* Normal mode. oversample and filter out the extra data */
    oversampling.val2 = 0;

    /* Setting scale in G, due to loss of precision if the SI unit m/s^2
        * is used
        */
    sensor_attr_set(sIMU_device,SENSOR_CHAN_ACCEL_XYZ,SENSOR_ATTR_FULL_SCALE,&full_scale);
    sensor_attr_set(sIMU_device,SENSOR_CHAN_ACCEL_XYZ,SENSOR_ATTR_OVERSAMPLING,&oversampling);
    sensor_attr_set(sIMU_device,SENSOR_CHAN_ACCEL_XYZ,SENSOR_ATTR_SAMPLING_FREQUENCY,&sampling_freq);

    full_scale.val1 = 300;          /* dps. probably too fast */
    full_scale.val2 = 0;
    sampling_freq.val1 = 50;       /* Hz. Performance mode */
    sampling_freq.val2 = 0;
    oversampling.val1 = 1;          /* Normal mode */
    oversampling.val2 = 0;

    sensor_attr_set(sIMU_device,SENSOR_CHAN_GYRO_XYZ,SENSOR_ATTR_FULL_SCALE,&full_scale);
    sensor_attr_set(sIMU_device,SENSOR_CHAN_GYRO_XYZ,SENSOR_ATTR_OVERSAMPLING,&oversampling);
    sensor_attr_set(sIMU_device,SENSOR_CHAN_GYRO_XYZ,SENSOR_ATTR_SAMPLING_FREQUENCY,&sampling_freq);

    k_sem_give(&imu_initialize_sem);
    k_sleep(K_FOREVER); // frees up whatever thread is ready to run
    return 0;
}

void approx_sensor_position(struct sensor_value *acc_pos, struct sensor_value *gyr_pos)
{
   /* Use gyroscope to make sure it's not moving. You can get a false reading if the 
    user shakes the device)*/
    if ((gyr_pos[0].val1 < 1) & (gyr_pos[1].val1< 1) & (gyr_pos[2].val1< 1))
    {
        if ((acc_pos[2].val1 <= NEG_GRAVITY) & (acc_pos[1].val1 < GRAVITY) & (acc_pos[0].val1 < GRAVITY))
        {
            //printk("-z value! up! \r\n"); // -z = up +z = down 
            device_side = up;
        }
        else if ((acc_pos[2].val1 >= GRAVITY) & (acc_pos[1].val1 < GRAVITY)  & (acc_pos[0].val1 < GRAVITY))
        {
            //printk("+z value! down!\r\n"); // -z = up +z = down 
            device_side = down;
        }
        else if ((acc_pos[1].val1 <= NEG_GRAVITY) & (acc_pos[2].val1 < GRAVITY)  & (acc_pos[0].val1 < GRAVITY))
        {
           //printk("-y value! right! \r\n");  // yvalue
            device_side = right;
        }
        else if ((acc_pos[1].val1 >= GRAVITY) & (acc_pos[2].val1 < GRAVITY)  &  (acc_pos[0].val1 < GRAVITY))
        {
           // printk("+y value! left!\r\n"); 
            device_side = left;
        } 
        else if ((acc_pos[0].val1 <= NEG_GRAVITY) & (acc_pos[2].val1 < GRAVITY)  & (acc_pos[1].val1 < GRAVITY))
        {
            //printk("-x value! upside_down! \r\n"); // x value
            device_side = upside_down;
        }
        else if ((acc_pos[0].val1 >= GRAVITY) & (acc_pos[2].val1 < GRAVITY)  & (acc_pos[1].val1 < GRAVITY))
        {
            //printk("+x value! rightside_up!\r\n"); 
            device_side = rightside_up;
        }
        else
        {
            device_side = invalid_side; // on an invalid side
        }
    }
    else
    {
        device_side = invalid_gyr; // move the device too fast
    }
}

void imu_thread_func(void *d0, void *d1, void *d2)
{
    /* Block until imu is available */
    k_sem_take(&imu_initialize_sem,K_FOREVER); 

    while(1)
    {
        /*protect critical section of code without interrupts*/
        k_mutex_lock(&imu_processing_mutex, K_FOREVER);
        while (device_side != invalid_gyr)
        { // keep getting data
            sensor_sample_fetch(sIMU_device);
            sensor_channel_get(sIMU_device, SENSOR_CHAN_GYRO_XYZ, gyr);
            sensor_channel_get(sIMU_device, SENSOR_CHAN_ACCEL_XYZ, acc);

          /*  printk("AX: %d.%06d, AY: %d.%06d, AZ: %d.%06d, \n",
                        //"GX: %d.%06d, GY: %d.%06d, GZ: %d.%06d,\n",
                        acc[0].val1, acc[0].val2,
                        acc[1].val1, acc[1].val2,
                        acc[2].val1, acc[2].val2);
                        //gyr[0].val1, gyr[0].val2,
                        //gyr[1].val1, gyr[1].val2,
                       // gyr[2].val1, gyr[2].val2);
            */
            
            approx_sensor_position(acc,gyr);
            //device_side_translation();
            k_sleep(K_MSEC(1000));
       }
        k_mutex_unlock(&imu_processing_mutex);
        wake_state_machine_thd(); // call the thread before going to sleep 
        k_sleep(K_FOREVER); // frees up whatever thread is ready to run
    }
}

K_THREAD_DEFINE(imu_thd,IMU_STACK,imu_thread_func,NULL,NULL,NULL,THREAD2_PRIORITY,0,5000);

void imu_data_processing()
{
    k_wakeup(imu_thd); // after i run the imu thread device side is ready to be read
    k_sleep(K_FOREVER); // SM to sleep. I can't go back to SM until the device side is valid.
}

SYS_INIT(imu_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
