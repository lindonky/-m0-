/*
 * TB6612 的 MSPM0 GPIO/PWM 适配层。
 *
 * 上层 TB6612 驱动已经实现方向真值表，本文件只负责把布尔电平和千分比占空比
 * 写入 SysConfig 选择的外设。初始化顺序以“任何中间状态都不能驱动电机”为准。
 */
#include "bsp/bsp_tb6612.h"

#include "config/board_config.h"
#include "ti_msp_dl_config.h"

#if CAR_TB6612_READY
static uint32_t g_pwmPeriodCounts = CAR_TB6612_PWM_PERIOD_COUNTS;

/** @brief 用同一个小函数统一 GPIO 高低电平写法，避免方向逻辑散落。 */
static void write_pin(GPIO_Regs *port, uint32_t pin, bool high)
{
    if (high) {
        DL_GPIO_setPins(port, pin);
    } else {
        DL_GPIO_clearPins(port, pin);
    }
}
#endif

void BSP_TB6612_Init(void)
{
#if CAR_TB6612_READY
    /*
     * SysConfig 已把五根 GPIO 的初始锁存值设为低，但这里再次显式写入，保证即使
     * 以后有人改了 SysConfig 初值，电机层仍从安全状态开始。
     */
    DL_TimerG_stopCounter(CAR_TB6612_PWM_INST);
    BSP_TB6612_SetStandby(false);
    BSP_TB6612_SetInputs(BSP_TB6612_CHANNEL_A, false, false);
    BSP_TB6612_SetInputs(BSP_TB6612_CHANNEL_B, false, false);

    /* 运行时读取实际 LOAD，避免将来只在 SysConfig 改周期后换算仍使用旧值。 */
    g_pwmPeriodCounts = DL_TimerG_getLoadValue(CAR_TB6612_PWM_INST);
    if (g_pwmPeriodCounts == 0U) {
        g_pwmPeriodCounts = CAR_TB6612_PWM_PERIOD_COUNTS;
    }

    BSP_TB6612_SetPwmPermille(BSP_TB6612_CHANNEL_A, 0U);
    BSP_TB6612_SetPwmPermille(BSP_TB6612_CHANNEL_B, 0U);

    /* 只启动 20 kHz PWM 计数器；STBY 仍为低，车辆不会自动运行。 */
    DL_TimerG_startCounter(CAR_TB6612_PWM_INST);
#endif
}

void BSP_TB6612_SetStandby(bool standbyReleased)
{
#if CAR_TB6612_READY
    write_pin(CAR_TB6612_STBY_GPIO_PORT, CAR_TB6612_STBY_GPIO_PIN,
              standbyReleased);
#else
    (void) standbyReleased;
#endif
}

void BSP_TB6612_SetInputs(BSP_TB6612_Channel channel, bool input1, bool input2)
{
#if CAR_TB6612_READY
    if (channel == BSP_TB6612_CHANNEL_A) {
        write_pin(CAR_TB6612_AIN1_GPIO_PORT, CAR_TB6612_AIN1_GPIO_PIN, input1);
        write_pin(CAR_TB6612_AIN2_GPIO_PORT, CAR_TB6612_AIN2_GPIO_PIN, input2);
    } else if (channel == BSP_TB6612_CHANNEL_B) {
        write_pin(CAR_TB6612_BIN1_GPIO_PORT, CAR_TB6612_BIN1_GPIO_PIN, input1);
        write_pin(CAR_TB6612_BIN2_GPIO_PORT, CAR_TB6612_BIN2_GPIO_PIN, input2);
    } else {
        /* 非法枚举值不操作任何引脚。 */
    }
#else
    (void) channel;
    (void) input1;
    (void) input2;
#endif
}

void BSP_TB6612_SetPwmPermille(BSP_TB6612_Channel channel, uint16_t dutyPermille)
{
#if CAR_TB6612_READY
    uint32_t activeCounts;
    uint32_t compareValue;

    if (dutyPermille > 1000U) dutyPermille = 1000U;

    /*
     * 当前为边沿对齐向下计数 PWM：CC=LOAD 是 0%，CC=0 是 100%。加 500 后
     * 再除以 1000，相当于四舍五入，减小千分比换算的系统性偏差。
     */
    activeCounts = ((g_pwmPeriodCounts * (uint32_t) dutyPermille) + 500U) /
                   1000U;
    compareValue = g_pwmPeriodCounts - activeCounts;

    if (channel == BSP_TB6612_CHANNEL_A) {
        DL_TimerG_setCaptureCompareValue(CAR_TB6612_PWM_INST, compareValue,
                                          CAR_TB6612_PWM_A_CC_INDEX);
    } else if (channel == BSP_TB6612_CHANNEL_B) {
        DL_TimerG_setCaptureCompareValue(CAR_TB6612_PWM_INST, compareValue,
                                          CAR_TB6612_PWM_B_CC_INDEX);
    } else {
        /* 非法枚举值不写比较寄存器。 */
    }
#else
    (void) channel;
    (void) dutyPermille;
#endif
}
