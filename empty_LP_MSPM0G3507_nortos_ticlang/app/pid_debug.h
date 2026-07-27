#ifndef PID_DEBUG_H
#define PID_DEBUG_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @file pid_debug.h
 * @brief 江协科技蓝牙小程序文本协议和 PID 在线调参。
 *
 * 协议沿用参考程序的方括号文本包：
 *
 * @code
 * [display,0,0,Hello World]
 * [plot,1.250,-0.375]
 * [key,1,up]
 * [slider,1,260.0]
 * @endcode
 *
 * 本模块不直接操作 UART 寄存器。RX 字节来自 BSP_UART 环形队列，TX 使用
 * BSP_UART_TryWrite() 一次性入队；队列空间不足时立即失败，不等待物理串口。
 */

typedef enum {
    PID_DEBUG_LOOP_LINE = 0,
    PID_DEBUG_LOOP_YAW_RATE,
    PID_DEBUG_LOOP_LEFT_SPEED,
    PID_DEBUG_LOOP_RIGHT_SPEED
} PIDDebug_Loop;

typedef struct {
    uint32_t receivedPackets;
    uint32_t keyPackets;
    uint32_t sliderPackets;
    uint32_t formatErrors;
    uint32_t overflowPackets;
    uint32_t timeoutPackets;
    uint32_t unknownPackets;
    uint32_t rejectedValues;
    uint32_t txRejected;
    uint32_t txTruncated;
    PIDDebug_Loop selectedLoop;
    bool plotEnabled;
} PIDDebug_Diagnostics;

/** @brief 复位文本包解析器，默认选择灰度方向 PID。 */
void PIDDebug_Init(void);

/**
 * @brief 向方括号协议解析器输入一个 HC-05 RX 字节。
 * @param nowMs 当前 1 ms 软件时基，用于不完整数据包超时保护。
 * @return true=字节属于 `[ ... ]` 文本包，调用者不得再把它解释为 R/S 等命令；
 *         false=当前不在文本包中，调用者可继续处理旧单字符命令。
 */
bool PIDDebug_PushRxByte(uint8_t byte, uint32_t nowMs);

/** @brief 检查长期未闭合的 `[` 数据包并在超时后退出接收状态。 */
void PIDDebug_CheckTimeout(uint32_t nowMs);

/**
 * @brief 非阻塞格式化并发送一段文本。
 *
 * 用法等价于参考程序的 printf，但函数名独立，避免全局重定向 libc printf：
 *
 * @code
 * PIDDebug_Printf("[display,0,0,Hello World]");
 * PIDDebug_Printf("[plot,%f,%f]", y1, y2);
 * @endcode
 *
 * @return true=完整格式化结果已进入 TX 队列；false=格式化失败、结果过长或队列满。
 * @note 允许 `%f`，但浮点格式化只能放在低频主循环调试任务，禁止在 ISR 和 5 ms
 *       控制任务中调用。
 */
bool PIDDebug_Printf(const char *format, ...);

/** @brief 发送 `[display,x,y,text]`；text 中不应包含 `]`。 */
bool PIDDebug_DisplayText(int16_t x, int16_t y, const char *text);

/** @brief 发送两条曲线的 `[plot,y1,y2]` 数据包。 */
bool PIDDebug_Plot2(float y1, float y2);

/**
 * @brief 低频发送所选 PID 参数和目标/实测曲线。
 * @note 由现有 CAR_DEBUG_PERIOD_MS 调度任务调用，不得放进 ISR。
 */
void PIDDebug_Task(void);

/** @return 文本协议、发送和当前选择状态的只读诊断信息。 */
const PIDDebug_Diagnostics *PIDDebug_GetDiagnostics(void);

#endif /* PID_DEBUG_H */
