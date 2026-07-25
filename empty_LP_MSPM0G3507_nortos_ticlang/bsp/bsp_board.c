/*
 * 整板补充初始化。
 *
 * 引脚复用、时钟和工作模式由 SysConfig 负责；各 BSP 自己负责对应外设的安全
 * 启动顺序，因此这里不再重复启动 PWM、QEI、ADC、UART 或 1 ms 时基。
 */
#include "bsp/bsp_board.h"

void BSP_Board_Init(void)
{
    /*
     * 保留这个入口用于未来真正跨外设的板级联动。当前所有外设初始化均已有明确
     * 归属：BSP_Time、BSP_TB6612、BSP_Encoder、BSP_LineADC 和 UART BSP。
     * 空函数是有意设计，不代表漏配；禁止在此加入低功耗或阻塞延时。
     */
}
