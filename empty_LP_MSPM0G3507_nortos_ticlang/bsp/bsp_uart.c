/*
 * 调试 UART 非阻塞适配层。
 *
 * 上层会在主循环高频轮询 RX、每 20 ms 尝试输出一行 CSV。本层不能等待 FIFO
 * 清空，推荐用环形缓冲和 TX/RX 中断，后期也可替换为 DMA。
 */
#include "bsp/bsp_uart.h"

void BSP_UART_Init(void)
{
    /* TODO：启动 empty.syscfg 中的 UART，并初始化软件缓冲区。 */
}

bool BSP_UART_TryWrite(const uint8_t *data, size_t length)
{
    (void) data;
    (void) length;
    /* TODO：非阻塞入队；队列空间不足时返回 false，不允许忙等。 */
    return false;
}

bool BSP_UART_TryReadByte(uint8_t *value)
{
    (void) value;
    /* TODO：从 RX 环形缓冲取一个字节；空队列立即返回 false。 */
    return false;
}
