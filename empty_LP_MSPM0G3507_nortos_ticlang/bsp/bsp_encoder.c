/*
 * 左右 QEI 硬件适配层。
 *
 * 本层只返回“当前累计计数”，方向极性、轮径和速度滤波由 drivers/encoder.c 处理。
 * 如果选用的定时器计数位宽较小，应在本层用溢出 ISR 扩展到 32 位。
 */
#include "bsp/bsp_encoder.h"

void BSP_Encoder_Init(void)
{
    /* TODO：启动 empty.syscfg 中配置的两个 QEI 定时器。 */
}

int32_t BSP_Encoder_GetLeftCount(void)
{
    /* TODO：返回左 QEI 的有符号/软件扩展累计计数。 */
    return 0;
}

int32_t BSP_Encoder_GetRightCount(void)
{
    /* TODO：返回右 QEI 的有符号/软件扩展累计计数。 */
    return 0;
}

void BSP_Encoder_ResetCounts(void)
{
    /* TODO：原子地清零两个硬件计数器及 ISR 维护的高位扩展。 */
}
