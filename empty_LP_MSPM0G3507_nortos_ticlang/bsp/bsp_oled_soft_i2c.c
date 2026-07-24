/*
 * MSPM0G3507 软件 I2C GPIO 实现。
 *
 * 为避免误把 I2C 高电平配置成推挽输出，本实现用方向控制模拟开漏：
 * - 逻辑 0：输出锁存器写 0，并允许输出；
 * - 逻辑 1：禁止输出，由外部上拉电阻把总线拉高。
 *
 * OLED 刷新是阻塞式软件 I2C，只能放在主循环低优先级任务中，不能在 ISR 或
 * 5 ms 电机控制任务中调用。
 */
#include "bsp/bsp_oled_soft_i2c.h"

#include "config/board_config.h"
#include "ti_msp_dl_config.h"

#if CAR_OLED_SOFT_I2C_READY
static void release_or_pull_low(GPIO_Regs *port, uint32_t pin, bool high)
{
    if (high) {
        DL_GPIO_disableOutput(port, pin);
    } else {
        DL_GPIO_clearPins(port, pin);
        DL_GPIO_enableOutput(port, pin);
    }

    /* 仅用于形成软件 I2C 位宽；忙等期间 CPU 不进入休眠。 */
    DL_Common_delayCycles(CAR_OLED_SOFT_I2C_DELAY_CYCLES);
}
#endif

void BSP_OLED_SoftI2C_Init(void)
{
#if CAR_OLED_SOFT_I2C_READY
    /*
     * SysConfig 初始为 SET+Hi-Z，即两根总线均处于释放状态。这里必须先关闭
     * 输出，再把锁存器预置为低；若顺序相反，会在初始化瞬间产生一次低脉冲。
     * 后续逻辑 0 只需开启输出，逻辑 1 只需关闭输出。
     */
    DL_GPIO_disableOutput(CAR_OLED_SCL_PORT, CAR_OLED_SCL_PIN);
    DL_GPIO_disableOutput(CAR_OLED_SDA_PORT, CAR_OLED_SDA_PIN);
    DL_GPIO_clearPins(CAR_OLED_SCL_PORT, CAR_OLED_SCL_PIN);
    DL_GPIO_clearPins(CAR_OLED_SDA_PORT, CAR_OLED_SDA_PIN);
    DL_Common_delayCycles(CAR_OLED_SOFT_I2C_DELAY_CYCLES);
#endif
}

void BSP_OLED_SoftI2C_WriteSCL(bool high)
{
#if CAR_OLED_SOFT_I2C_READY
    release_or_pull_low(CAR_OLED_SCL_PORT, CAR_OLED_SCL_PIN, high);
#else
    (void) high;
#endif
}

void BSP_OLED_SoftI2C_WriteSDA(bool high)
{
#if CAR_OLED_SOFT_I2C_READY
    release_or_pull_low(CAR_OLED_SDA_PORT, CAR_OLED_SDA_PIN, high);
#else
    (void) high;
#endif
}

bool BSP_OLED_SoftI2C_ReadSDA(void)
{
#if CAR_OLED_SOFT_I2C_READY
    return ((DL_GPIO_readPins(CAR_OLED_SDA_PORT, CAR_OLED_SDA_PIN) &
             CAR_OLED_SDA_PIN) != 0U);
#else
    /* 未配置时模拟总线高电平，表示没有从机拉低 ACK。 */
    return true;
#endif
}
