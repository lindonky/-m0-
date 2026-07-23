#ifndef BSP_LINE_ADC_H
#define BSP_LINE_ADC_H

#include <stdbool.h>
#include <stdint.h>

#include "config/car_config.h"

/**
 * @file bsp_line_adc.h
 * @brief 八路地址复用数字循迹模块的非阻塞采样边界。
 *
 * 文件名保留 ADC 是为了不破坏现有上层接口；实际模块只输出数字 0/1。BSP 将其
 * 映射为 0/4095，使现有归一化和加权质心算法可以继续使用。
 */

/**
 * @brief 清空扫描状态并选择 CH0。
 * @note 必须在 SYSCFG_DL_init() 之后调用；函数不包含任何阻塞延时。
 */
void BSP_LineADC_Init(void);

/**
 * @brief 读取当前通道、切换到下一通道，并在 CH0~CH7 全部完成后复制一帧。
 * @param values 输出数组，必须按物理最左到最右排列。
 * @return true=本次形成并复制了完整新帧；false=仍在扫描或硬件尚未配置。
 *
 * 当前应用每 1 ms 调用一次，因此每个地址有约 1 ms 建立时间，完整帧率为 125 Hz。
 * 不要在无时间间隔的紧循环中反复调用本函数。
 */
bool BSP_LineADC_Read(uint16_t values[CAR_LINE_SENSOR_COUNT]);

/** @brief 查询真实 GPIO 分支是否已通过 CAR_LINE_MUX_READY 启用。 */
bool BSP_LineADC_IsReady(void);

/** @brief 返回自初始化以来完成的八通道整帧数量，便于调试器观察。 */
uint32_t BSP_LineADC_GetFrameCount(void);

#endif /* BSP_LINE_ADC_H */
