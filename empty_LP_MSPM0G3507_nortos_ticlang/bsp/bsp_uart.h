#ifndef BSP_UART_H
#define BSP_UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** @file bsp_uart.h @brief 非阻塞调试 UART 边界接口。 */

/** @brief 初始化 UART 和底层 TX/RX 缓冲。 */
void BSP_UART_Init(void);

/**
 * @brief 尝试把一段数据放入发送队列，不等待物理发送完成。
 * @return true=全部接收；false=队列未配置或空间不足。
 */
bool BSP_UART_TryWrite(const uint8_t *data, size_t length);

/** @brief 尝试从 RX 队列取一个字节；无数据时立即返回 false。 */
bool BSP_UART_TryReadByte(uint8_t *value);

#endif /* BSP_UART_H */
