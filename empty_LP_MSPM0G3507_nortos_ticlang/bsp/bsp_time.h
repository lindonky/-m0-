#ifndef BSP_TIME_H
#define BSP_TIME_H

#include <stdint.h>

/** @file bsp_time.h @brief 无休眠 NoRTOS 调度器使用的 1 ms 单调时基。 */

/** @brief 清零软件毫秒计数；不会自行配置或启动硬件定时器。 */
void BSP_Time_Init(void);

/** @return 自初始化以来经过的毫秒数；允许 uint32_t 自然回绕。 */
uint32_t BSP_Time_GetMs(void);

/**
 * @brief 由 1 ms 周期定时器 ISR 恰好调用一次。
 * @note 调用前应确认并清除对应中断标志，其他代码不应直接调用。
 */
void BSP_Time_Tick1msFromISR(void);

#endif /* BSP_TIME_H */
