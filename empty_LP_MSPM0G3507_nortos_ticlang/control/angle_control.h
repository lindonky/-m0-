#ifndef ANGLE_CONTROL_H
#define ANGLE_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

#include "control/pid.h"

/**
 * @file angle_control.h
 * @brief IMU 相对偏航角闭环，专用于当前原地定角调试模式。
 *
 * 角度定义统一为：进入/重新启动时当前车头为 0 deg，向右为正、向左为负；手机
 * 下发范围为 -180~+180 deg。控制器输出不是 PWM，而是原地旋转时的单轮目标速度
 * mm/s，后续仍由左右轮速度 PID 和电机层完成闭环、斜坡与限幅。
 */

typedef struct {
    float targetDegrees;
    float measuredDegrees;
    float errorDegrees;
    float gyroZDps;
    float steeringMmS;
    bool imuValid;
    bool settled;
    uint16_t settledCycles;
} AngleControl_Status;

/** @brief 初始化角度 PID，默认目标为 0 deg。 */
void AngleControl_Init(void);

/** @brief 清除 PID 历史并把目标重新设为 0 deg。 */
void AngleControl_Reset(void);

/**
 * @brief 修改相对目标角度并清除旧积分/微分状态。
 * @return true=目标位于 -180~+180 deg；false=拒绝越界值且保持原目标。
 */
bool AngleControl_SetTargetDegrees(float targetDegrees);

/**
 * @brief 执行一次角度闭环。
 * @return 原地旋转的有符号单轮目标速度；正值表示右转。
 */
float AngleControl_Update(float yawDegrees, float gyroZDps, bool imuValid,
                          float dtSeconds);

/** @return 当前角度 PID，供 HC-05 在线修改 Kp/Ki/Kd。 */
PID_Controller *AngleControl_GetPID(void);

/** @return 最近一次角度控制状态。 */
const AngleControl_Status *AngleControl_GetStatus(void);

#endif /* ANGLE_CONTROL_H */
