#ifndef _IMU_H_
#define _IMU_H_


void imu_thread_func(void *d0, void *d1, void *d2);
extern int imu_data_processing();

#endif
