#ifndef IMU_H
#define IMU_H

#include <stdbool.h>

/**
 * @file imu.h
 * @brief 尚未绑定具体型号的 IMU 公共接口。
 * @warning 当前实现只返回无效数据，不能用于控制。
 */

/** 循迹车首版主要使用 Z 轴角速度和短时间积分偏航角。 */
typedef struct {
    float gyroZDps;
    float yawDegrees;
    float temperatureC;
    bool valid;
} IMU_Data;

bool IMU_Init(void);
bool IMU_Update(float dtSeconds);
void IMU_StartGyroCalibration(void);
void IMU_ResetYaw(void);
const IMU_Data *IMU_GetData(void);

#endif
