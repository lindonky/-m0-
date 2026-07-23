/*
 * 整板补充初始化。
 *
 * 该文件故意不猜测任何外设实例名称。完成 empty.syscfg 后，只在这里补充“启动”
 * 动作；引脚复用、时钟、工作模式等静态配置仍由 SysConfig 负责。
 */
#include "bsp/bsp_board.h"

void BSP_Board_Init(void)
{
    /*
     * TODO（完成 empty.syscfg 后）：
     * 1. 保持 TB6612 STBY 低，两个 PWM 比较值为 0；
     * 2. 启动 1 ms 周期定时器并允许中断；
     * 3. 启动左右 QEI；
     * 4. 若使用定时器触发 ADC，在此启动触发链；
     * 5. 根据实现启动 UART/I2C 中断或 DMA。
     *
     * 禁止在此加入 WFI、Sleep、Standby 或其他 MCU 低功耗入口。
     */
}
