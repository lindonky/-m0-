/*
 * 1 ms 软件时基。
 *
 * Cortex-M0+ 对对齐的 32 位读写是原子的，因此主循环读取该计数不需要临界区。
 * uint32_t 回绕由调度器中的有符号差值比较自然处理。
 */
#include "bsp/bsp_time.h"

static volatile uint32_t g_timeMs;

void BSP_Time_Init(void)
{
    g_timeMs = 0U;
}

uint32_t BSP_Time_GetMs(void)
{
    return g_timeMs;
}

void BSP_Time_Tick1msFromISR(void)
{
    /* ISR 中只做常数时间自增，保持中断延迟可预测。 */
    ++g_timeMs;
}
