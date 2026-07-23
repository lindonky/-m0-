#ifndef APP_DEBUG_H
#define APP_DEBUG_H

/**
 * @file app_debug.h
 * @brief UART 单字符命令和低频 CSV 遥测应用层。
 *
 * 新增 IMU 命令：G=当前偏航角清零，I=开始静止角速度零偏标定。
 */

void App_Debug_Init(void);

/** @brief 高频非阻塞轮询 RX，把命令转换为整车状态机请求。 */
void App_Debug_PollCommands(void);

/** @brief 由调度器低频调用，尝试输出一行状态。 */
void App_Debug_Task(void);

#endif
