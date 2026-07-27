#ifndef LINE_CONTROL_H
#define LINE_CONTROL_H

#include "control/pid.h"
#include "drivers/line_sensor.h"

/**
 * @file line_control.h
 * @brief 把循迹位置误差转换为前进速度和转向速度差。
 */

/** 方向环输出，两个字段单位都是 mm/s。 */
typedef struct {
    float forwardMmS;
    float steeringMmS;
} LineControl_Output;

/**
 * 灰度方向环与 IMU 角速度内环的最近一次运行状态。
 * 这些量只用于 OLED、HC-05 和在线调参；实际电机命令仍只使用
 * LineControl_Output，避免诊断代码进入实时控制数据通路。
 */
typedef struct {
    /** 灰度位置环直接给出的基础转向速度差。 */
    float lineSteeringMmS;
    /** 由基础转向换算出的目标偏航角速度。 */
    float targetYawRateDps;
    /** IMU 最近测得的 Z 轴偏航角速度。 */
    float measuredYawRateDps;
    /** 角速度 PID 生成并经过渐入系数处理的附加转向量。 */
    float yawRateCorrectionMmS;
    /** 送往 VehicleMixer 的最终转向量。 */
    float finalSteeringMmS;
    /** 0~1，表示 IMU 修正当前已渐入的比例。 */
    float imuEngageRatio;
    /** true=本周期 IMU 有效、未标定，角速度闭环参与了控制。 */
    bool yawRateControlActive;
} LineControl_Status;

/** @brief 使用 car_config.h 默认参数初始化方向 PID。 */
void LineControl_Init(void);

/** @brief 清除方向 PID 和最后有效循迹方向。 */
void LineControl_Reset(void);

/**
 * @brief 根据最新循迹结果和 IMU 角速度计算本周期速度和转向。
 * @param measuredYawRateDps IMU Z 轴角速度，约定车辆向右转为正。
 * @param yawRateValid true=IMU 数据未超时且当前不在零偏标定。
 *
 * IMU 不可用时自动复位角速度 PID，并无扰回退到原灰度方向环输出。
 */
LineControl_Output LineControl_Update(const LineSensor_Data *line,
                                      float measuredYawRateDps,
                                      bool yawRateValid,
                                      float dtSeconds);

/** @return 方向 PID 指针，供在线调参使用；不要在 ISR 中修改。 */
PID_Controller *LineControl_GetPID(void);

/** @return IMU 角速度内环 PID，供在线调参；不要在 ISR 中修改。 */
PID_Controller *LineControl_GetYawRatePID(void);

/** @return 最近一次方向控制状态；只读，不要修改返回内容。 */
const LineControl_Status *LineControl_GetStatus(void);

#endif
