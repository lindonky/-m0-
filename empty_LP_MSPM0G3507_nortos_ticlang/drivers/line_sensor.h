#ifndef LINE_SENSOR_H
#define LINE_SENSOR_H

#include <stdbool.h>
#include <stdint.h>
#include "config/car_config.h"

/**
 * @file line_sensor.h
 * @brief 模拟循迹阵列的标定、归一化、位置解算和基础元素标志。
 */

/** 一帧循迹结果。position 左负右正，confidence 范围约 0~1。 */
typedef struct {
    uint16_t raw[CAR_LINE_SENSOR_COUNT];
    uint16_t normalized[CAR_LINE_SENSOR_COUNT];
    float position;
    float confidence;
    bool fresh;
    bool lineDetected;
    bool allBlack;
    bool allWhite;
    bool junction;
} LineSensor_Data;

/** @brief 初始化 ADC 边界和默认 0~4095 标定范围。 */
void LineSensor_Init(void);

/** @brief 尝试处理一帧新 ADC 数据；没有新帧时返回 false。 */
bool LineSensor_Sample(void);

/** @brief 清空极值并进入连续标定状态。 */
void LineSensor_StartCalibration(void);

/** @brief 用当前 raw 帧更新每个通道的最小/最大值。 */
void LineSensor_UpdateCalibration(void);

/** @brief 结束标定，保留 RAM 中的极值。 */
void LineSensor_FinishCalibration(void);

/** @brief 查询是否正在标定。 */
bool LineSensor_IsCalibrating(void);

/** @return 最近一次处理结果的只读指针。 */
const LineSensor_Data *LineSensor_GetData(void);

#endif
