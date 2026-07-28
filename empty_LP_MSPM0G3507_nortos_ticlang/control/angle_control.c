/*
 * IMU 相对角度闭环。
 *
 * 这里只做“角度误差 -> 原地旋转轮速”的外层控制，不直接写 TB6612。左右轮编码器
 * 速度 PID 仍然负责把 +steering/-steering 变为 PWM，因此角度环不会绕过已有的
 * 电机斜坡、限幅、极性和急停保护。
 */
#include "control/angle_control.h"

#include <stdint.h>

#include "config/car_config.h"

static PID_Controller g_anglePid;
static AngleControl_Status g_status;

static float absolute(float value)
{
    return (value < 0.0f) ? -value : value;
}

/** 把任意角度误差折算到 [-180, +180]，保证始终选择较短旋转方向。 */
static float wrap_error_180(float errorDegrees)
{
    while (errorDegrees > 180.0f) errorDegrees -= 360.0f;
    while (errorDegrees < -180.0f) errorDegrees += 360.0f;
    return errorDegrees;
}

void AngleControl_Init(void)
{
    PID_Init(&g_anglePid, CAR_ANGLE_KP, CAR_ANGLE_KI, CAR_ANGLE_KD,
             -CAR_ANGLE_STEERING_LIMIT_MM_S,
             CAR_ANGLE_STEERING_LIMIT_MM_S);
    /* 角度由 0.1 deg 分辨率的 500 Hz 数据积分而来，略加强微分低通以抑制跳动。 */
    PID_SetDerivativeFilter(&g_anglePid, 0.85f);
    AngleControl_Reset();
}

void AngleControl_Reset(void)
{
    PID_Reset(&g_anglePid);
    g_status = (AngleControl_Status) {0};
}

bool AngleControl_SetTargetDegrees(float targetDegrees)
{
    if ((targetDegrees < CAR_ANGLE_TARGET_MIN_DEG) ||
        (targetDegrees > CAR_ANGLE_TARGET_MAX_DEG)) {
        return false;
    }

    g_status.targetDegrees = targetDegrees;
    g_status.settled = false;
    g_status.settledCycles = 0U;
    PID_Reset(&g_anglePid);
    return true;
}

float AngleControl_Update(float yawDegrees, float gyroZDps, bool imuValid,
                          float dtSeconds)
{
    uint16_t requiredCycles = (uint16_t)
        ((CAR_ANGLE_SETTLE_TIME_MS + CAR_CONTROL_PERIOD_MS - 1U) /
         CAR_CONTROL_PERIOD_MS);

    g_status.measuredDegrees = yawDegrees;
    g_status.gyroZDps = gyroZDps;
    g_status.imuValid = imuValid;
    g_status.errorDegrees = wrap_error_180(g_status.targetDegrees - yawDegrees);

    if (!imuValid || (dtSeconds <= 0.0f)) {
        PID_Reset(&g_anglePid);
        g_status.steeringMmS = 0.0f;
        g_status.settled = false;
        g_status.settledCycles = 0U;
        return 0.0f;
    }

    if ((absolute(g_status.errorDegrees) <= CAR_ANGLE_TOLERANCE_DEG) &&
        (absolute(gyroZDps) <= CAR_ANGLE_RATE_TOLERANCE_DPS)) {
        if (g_status.settledCycles < requiredCycles) g_status.settledCycles++;
    } else {
        g_status.settledCycles = 0U;
        g_status.settled = false;
    }

    if ((requiredCycles == 0U) || (g_status.settledCycles >= requiredCycles)) {
        g_status.settled = true;
        g_status.steeringMmS = 0.0f;
        PID_Reset(&g_anglePid);
        return 0.0f;
    }

    /*
     * 通用 PID 的内部误差是 setpoint-measurement。这里已先完成 ±180 deg 最短方向
     * 折算，因此以 error 为 setpoint、0 为 measurement，避免 0/360 附近跳变。
     */
    g_status.steeringMmS = PID_Update(&g_anglePid, g_status.errorDegrees,
                                      0.0f, dtSeconds);
    return g_status.steeringMmS;
}

PID_Controller *AngleControl_GetPID(void)
{
    return &g_anglePid;
}

const AngleControl_Status *AngleControl_GetStatus(void)
{
    return &g_status;
}
