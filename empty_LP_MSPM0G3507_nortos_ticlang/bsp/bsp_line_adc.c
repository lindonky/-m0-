/*
 * 循迹 ADC 帧适配层。
 *
 * 上层要求一次获得同一采样批次的全部通道，避免车辆运动时逐通道旧/新数据混杂。
 * 使用 DMA 时建议采用双缓冲或“完成序号”，只有完整新帧才返回 true。
 */
#include "bsp/bsp_line_adc.h"

void BSP_LineADC_Init(void)
{
    /*
     * TODO：在 empty.syscfg 配置定时器触发的 ADC 序列。通道顺序应按物理左到右；
     * 若 SysConfig 顺序不同，在 Read() 复制时重排。首版可不用 DMA。
     */
}

bool BSP_LineADC_Read(uint16_t values[CAR_LINE_SENSOR_COUNT])
{
    uint32_t index;

    for (index = 0U; index < CAR_LINE_SENSOR_COUNT; ++index) {
        values[index] = 0U;
    }

    /* TODO：复制一帧一致数据并返回 true；没有新帧必须返回 false。 */
    return false;
}
