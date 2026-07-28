#ifndef APP_DEBUG_H
#define APP_DEBUG_H

#include <stdint.h>

/**
 * @file app_debug.h
 * @brief UART 单字符命令、PID 文本调试协议、可选 CSV 和 OLED 诊断显示应用层。
 *
 * 新增 IMU 命令：G=当前偏航角清零，I=开始静止角速度零偏标定。
 * 方括号 `[key/slider,...]` 数据包由 pid_debug.c 单独解析；包内字符不会再被解释为
 * R/S/X/C/E/G/I，因此两套协议可以共用同一个 HC-05 RX 队列。
 */

void App_Debug_Init(void);

/** @brief 高频非阻塞轮询 RX，把命令转换为整车状态机请求。 */
void App_Debug_PollCommands(void);

/** @brief 由调度器低频调用，尝试输出一行状态。 */
void App_Debug_Task(void);

/**
 * @brief 刷新 OLED 诊断画面的一行。
 *
 * 软件 I2C 为有界同步传输，因此每次只发送一个 8 像素页；当前每 40 ms 调用一次，
 * 八行约 0.32 秒刷新一遍。诊断画面不再自动轮换，由蓝牙 key 手动切换。
 */
void App_Debug_OLEDTask(void);

/** @brief 切换到下一张 OLED 诊断画面并从第 0 行开始刷新，返回 0~2 页面号。 */
uint8_t App_Debug_NextOLEDView(void);

#endif
