/*
 * 串口调试协议。
 *
 * 使用单字符命令保证底层刚接通时也容易验证；CSV 使用整数缩放，避免嵌入式 libc
 * 启用体积较大的浮点 printf。发送失败直接丢弃本帧，绝不能阻塞控制环等待。
 */
#include "app/app_debug.h"

#include <stdio.h>
#include <stdint.h>
#include "app/app_car.h"
#include "bsp/bsp_time.h"
#include "bsp/bsp_uart.h"
#include "control/speed_control.h"
#include "drivers/encoder.h"
#include "drivers/imu.h"
#include "drivers/line_sensor.h"
#include "drivers/motor.h"

void App_Debug_Init(void) { BSP_UART_Init(); }

void App_Debug_PollCommands(void)
{
    uint8_t command;
    /* 一次清空当前 RX 队列；底层无数据时立即退出。 */
    while (BSP_UART_TryReadByte(&command)) {
        switch (command) {
        case 'r': case 'R': App_Car_Start(); break;
        case 's': case 'S': App_Car_Stop(); break;
        case 'x': case 'X': App_Car_EmergencyStop(); break;
        case 'c': case 'C': App_Car_StartCalibration(); break;
        case 'e': case 'E': App_Car_FinishCalibration(); break;
        case 'g': case 'G': IMU_ResetYaw(); break;
        case 'i': case 'I': IMU_StartGyroCalibration(); break;
        default: break;
        }
    }
}

void App_Debug_Task(void)
{
    char buffer[160];
    int length;
    const LineSensor_Data *line = LineSensor_GetData();
    const Encoder_Data *left = Encoder_GetLeft();
    const Encoder_Data *right = Encoder_GetRight();
    const Motor_Status *motor = Motor_GetStatus();
    const IMU_Data *imu = IMU_GetData();
    const IMU_Diagnostics *imuDiag = IMU_GetDiagnostics();

    /*
     * 字段顺序：时间、状态、位置×1000、左右速度、左右占空比、是否在线、
     * IMU有效、偏航角×10、角速度×10、IMU CRC错误数。
     * 只格式化整数，避免链接浮点 printf。
     */
    length = snprintf(buffer, sizeof(buffer),
        "%lu,%u,%ld,%ld,%ld,%d,%d,%u,%u,%ld,%ld,%lu\r\n",
        (unsigned long) BSP_Time_GetMs(),
        (unsigned int) App_Car_GetState(),
        (long) (line->position * 1000.0f),
        (long) left->speedMmS,
        (long) right->speedMmS,
        (int) motor->leftAppliedPermille,
        (int) motor->rightAppliedPermille,
        line->lineDetected ? 1U : 0U,
        imu->valid ? 1U : 0U,
        (long) (imu->yawDegrees * 10.0f),
        (long) (imu->gyroZDps * 10.0f),
        (unsigned long) imuDiag->crcErrors);
    if (length > 0) {
        /* snprintf 返回期望长度，缓冲截断时必须限制实际发送长度。 */
        size_t sendLength = (size_t) length;
        if (sendLength >= sizeof(buffer)) sendLength = sizeof(buffer) - 1U;
        /* 队列满时允许丢帧；遥测优先级低于控制。 */
        (void) BSP_UART_TryWrite((const uint8_t *) buffer, sendLength);
    }
}
