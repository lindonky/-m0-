#ifndef APP_CAR_H
#define APP_CAR_H

#include <stdint.h>

/**
 * @file app_car.h
 * @brief 整车状态机和周期任务入口。
 *
 * 所有启动、停止、标定和故障动作集中在此层，避免通信、按键或赛道识别模块直接
 * 操作电机。
 */

/** 基础车辆状态；环岛、十字等比赛元素应在后续扩展为子状态。 */
typedef enum {
    CAR_STATE_IDLE = 0,
    CAR_STATE_CALIBRATING,
    CAR_STATE_RUNNING,
    CAR_STATE_LINE_LOST,
    CAR_STATE_STOPPED,
    CAR_STATE_FAULT
} Car_State;

/** @brief 初始化全部 BSP/驱动/控制器，上电后进入 IDLE，电机禁止。 */
void App_Car_Init(void);

/** @brief 1 ms 任务：接收 500 Hz IMU、采样循迹并更新时间戳。 */
void App_Car_SensorTask1ms(void);

/** @brief 5 ms 任务：编码器、所选方向/角度环、速度环和电机输出。 */
void App_Car_ControlTask5ms(void);

/** @brief 10 ms 任务：状态和丢线超时安全检查。 */
void App_Car_StateTask10ms(void);

/** @brief 从非标定、非故障条件开始运行；会复位里程和 PID。 */
void App_Car_Start(void);

/** @brief 正常停止并进入 STOPPED。 */
void App_Car_Stop(void);

/** @brief 立即停机并进入 FAULT。 */
void App_Car_EmergencyStop(void);

/** @brief 禁止电机并开始连续记录每路 ADC 极值。 */
void App_Car_StartCalibration(void);

/** @brief 结束标定，保留 RAM 极值并回到 IDLE。 */
void App_Car_FinishCalibration(void);

Car_State App_Car_GetState(void);

/** @return 距离最近一次检测到线经过的毫秒数，uint32_t 回绕安全。 */
uint32_t App_Car_GetLineLostTimeMs(void);

#endif
