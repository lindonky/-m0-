/*
 * IMU 型号占位实现。
 * 选定 MPU6050/ICM42688/BMI088 等器件后，应在此实现 WHO_AM_I、量程、零偏
 * 标定、滤波和超时；在此之前 valid 永远为 false，应用层不得使用其控制车辆。
 */
#include "drivers/imu.h"

static IMU_Data g_data;

bool IMU_Init(void)
{
    g_data = (IMU_Data) {0};
    /* TODO：选定型号后实现寄存器初始化和器件身份检查。 */
    return false;
}

bool IMU_Update(float dtSeconds)
{
    (void) dtSeconds;
    /* TODO：读取 Z 轴、去零偏、滤波，然后用 dt 积分短时间偏航角。 */
    g_data.valid = false;
    return false;
}

void IMU_StartGyroCalibration(void)
{
    /* TODO：用分时状态机收集静止样本，不能阻塞调度器。 */
}

void IMU_ResetYaw(void) { g_data.yawDegrees = 0.0f; }
const IMU_Data *IMU_GetData(void) { return &g_data; }
