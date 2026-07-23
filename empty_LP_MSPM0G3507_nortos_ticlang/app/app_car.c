/*
 * 整车应用编排。
 *
 * 此文件只组合已有模块，不包含 GPIO/ADC 寄存器细节。运行状态下的数据通路是：
 * LineSensor → LineControl → VehicleMixer → SpeedControl → Motor → TB6612。
 */
#include "app/app_car.h"

#include "bsp/bsp_board.h"
#include "bsp/bsp_time.h"
#include "config/car_config.h"
#include "control/line_control.h"
#include "control/speed_control.h"
#include "control/vehicle_mixer.h"
#include "drivers/encoder.h"
#include "drivers/line_sensor.h"
#include "drivers/motor.h"

static Car_State g_state;
static uint32_t g_lastLineSeenMs;

void App_Car_Init(void)
{
    /* 先清软件时间，再启动可能产生中断的板级外设。 */
    BSP_Time_Init();
    BSP_Board_Init();
    Motor_Init();
    Encoder_Init();
    LineSensor_Init();
    LineControl_Init();
    SpeedControl_Init();
    g_lastLineSeenMs = BSP_Time_GetMs();
    /* 上电明确停在 IDLE，不自动调用 Start，防止下载后车辆突然运动。 */
    g_state = CAR_STATE_IDLE;
}

void App_Car_SensorTask1ms(void)
{
    /* 只有完整新帧且确认在线，才刷新丢线看门时间。 */
    if (LineSensor_Sample() && LineSensor_GetData()->lineDetected) {
        g_lastLineSeenMs = BSP_Time_GetMs();
    }
}

void App_Car_ControlTask5ms(void)
{
    const Encoder_Data *left;
    const Encoder_Data *right;
    LineControl_Output lineOutput;
    WheelSpeed_Targets targets;

    /* 编码器无论车辆状态如何都更新，便于停车时观察外力推动和诊断。 */
    Encoder_Update(CAR_CONTROL_PERIOD_S);
    left = Encoder_GetLeft();
    right = Encoder_GetRight();

    if (g_state == CAR_STATE_RUNNING) {
        /* 方向环给出前进/转向速度，再混合为左右轮目标。 */
        lineOutput = LineControl_Update(LineSensor_GetData(),
                                        CAR_CONTROL_PERIOD_S);
        targets = VehicleMixer_Mix(lineOutput.forwardMmS,
                                   lineOutput.steeringMmS,
                                   CAR_MAX_WHEEL_SPEED_MM_S);
        SpeedControl_Update(targets.leftMmS, targets.rightMmS,
                            left->speedMmS, right->speedMmS,
                            CAR_CONTROL_PERIOD_S);
    } else {
        /* 非运行状态持续清积分，保证下一次启动没有历史输出。 */
        SpeedControl_Reset();
    }

    /* Motor_Update 最终执行斜坡和 TB6612 写入。 */
    Motor_Update();
}

void App_Car_StateTask10ms(void)
{
    /* 短时丢线由 LineControl 搜索，超过阈值才转入安全停止状态。 */
    if ((g_state == CAR_STATE_RUNNING) &&
        (App_Car_GetLineLostTimeMs() >= CAR_LINE_LOST_STOP_MS)) {
        Motor_SetStopMode(TB6612_STOP_BRAKE);
        Motor_Enable(false);
        SpeedControl_Reset();
        g_state = CAR_STATE_LINE_LOST;
    }
}

void App_Car_Start(void)
{
    /* 标定时禁止启动；FAULT 需先通过明确策略解除，当前 R 不会从 FAULT 启动。 */
    if ((g_state == CAR_STATE_FAULT) || LineSensor_IsCalibrating()) return;
    Encoder_Reset();
    LineControl_Reset();
    SpeedControl_Reset();
    Motor_ClearEmergencyStop();
    Motor_SetStopMode(TB6612_STOP_COAST);
    /* 所有历史量复位完成后，最后才释放 TB6612 STBY。 */
    Motor_Enable(true);
    g_lastLineSeenMs = BSP_Time_GetMs();
    g_state = CAR_STATE_RUNNING;
}

void App_Car_Stop(void)
{
    /* 拉低 STBY 后最终为高阻安全停机；BRAKE 模式只影响禁用前的零指令过程。 */
    Motor_SetStopMode(TB6612_STOP_BRAKE);
    Motor_Enable(false);
    SpeedControl_Reset();
    g_state = CAR_STATE_STOPPED;
}

void App_Car_EmergencyStop(void)
{
    /* EmergencyStop 在电机层锁存，必须由后续明确恢复流程清除。 */
    Motor_EmergencyStop();
    SpeedControl_Reset();
    g_state = CAR_STATE_FAULT;
}

void App_Car_StartCalibration(void)
{
    /* 标定要求人工让传感器扫过黑白区域，期间绝不允许电机运行。 */
    Motor_Enable(false);
    LineSensor_StartCalibration();
    g_state = CAR_STATE_CALIBRATING;
}

void App_Car_FinishCalibration(void)
{
    LineSensor_FinishCalibration();
    g_state = CAR_STATE_IDLE;
}

Car_State App_Car_GetState(void) { return g_state; }

uint32_t App_Car_GetLineLostTimeMs(void)
{
    return BSP_Time_GetMs() - g_lastLineSeenMs;
}
