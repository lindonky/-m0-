/*
 * TB6612 的 MSPM0 GPIO/PWM 适配层。
 *
 * 上层 TB6612 驱动已经实现方向真值表，本文件只负责把布尔电平和千分比占空比
 * 写入 SysConfig 选择的外设。当前为空实现，未配置硬件时不会误操作寄存器。
 */
#include "bsp/bsp_tb6612.h"

void BSP_TB6612_Init(void)
{
    /*
     * TODO：在 empty.syscfg 创建两个 20 kHz 边沿对齐 PWM 和五个 GPIO 输出
     *（AIN1/AIN2/BIN1/BIN2/STBY），上电 PWM=0、STBY=0，然后使用生成符号。
     */
}

void BSP_TB6612_SetStandby(bool standbyReleased)
{
    (void) standbyReleased;
    /* TODO：使用生成的 STBY 端口/引脚宏执行 set/clear。 */
}

void BSP_TB6612_SetInputs(BSP_TB6612_Channel channel, bool input1, bool input2)
{
    (void) channel;
    (void) input1;
    (void) input2;
    /* TODO：根据 channel 写 AIN1/AIN2 或 BIN1/BIN2。 */
}

void BSP_TB6612_SetPwmPermille(BSP_TB6612_Channel channel, uint16_t dutyPermille)
{
    (void) channel;
    (void) dutyPermille;
    /*
     * TODO：把 0~1000 换算为 LOAD/CC 数值，再写 PWMA 或 PWMB 对应 CC。
     * 写入前应再次限制 dutyPermille <= 1000，并确认 PWM 极性是否需要反向比较值。
     */
}
