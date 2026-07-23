#ifndef BSP_LINE_ADC_H
#define BSP_LINE_ADC_H

#include <stdbool.h>
#include <stdint.h>

#include "config/car_config.h"

/** @file bsp_line_adc.h @brief 一帧多路循迹 ADC 结果的硬件读取接口。 */

/** @brief 启动 ADC 序列/触发/DMA；不能在此加入阻塞延时。 */
void BSP_LineADC_Init(void);

/**
 * @brief 尝试复制一帧时间一致的循迹 ADC 数据。
 * @param values 输出数组，必须按物理最左到最右排列。
 * @return true=复制了完整的新帧；false=当前没有新帧。
 */
bool BSP_LineADC_Read(uint16_t values[CAR_LINE_SENSOR_COUNT]);

#endif /* BSP_LINE_ADC_H */
