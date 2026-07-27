/*
 * 串口调试协议。
 *
 * 使用单字符命令保证底层刚接通时也容易验证；CSV 使用整数缩放，避免嵌入式 libc
 * 启用体积较大的浮点 printf。发送失败直接丢弃本帧，绝不能阻塞控制环等待。
 */
#include "app/app_debug.h"
#include "app/pid_debug.h"

#include <stdio.h>
#include <stdint.h>
#include "app/app_car.h"
#include "bsp/bsp_encoder.h"
#include "bsp/bsp_line_adc.h"
#include "bsp/bsp_time.h"
#include "bsp/bsp_uart.h"
#include "config/board_config.h"
#include "control/line_control.h"
#include "control/speed_control.h"
#include "drivers/encoder.h"
#include "drivers/imu.h"
#include "drivers/line_sensor.h"
#include "drivers/motor.h"
#include "drivers/oled.h"

#if CAR_OLED_SOFT_I2C_READY
/* SSD1306 的 64 像素高度正好分成 8 个页，每页高 8 像素。 */
#define APP_OLED_PAGE_COUNT (8U)

static uint8_t g_oledNextPage;
static uint8_t g_oledView;
static bool g_oledConnected;

/** @brief 在 RAM 显存生成 ADC/循迹画面；本函数本身不访问 I2C。 */
static void render_oled_line_diagnostics(void)
{
    const LineSensor_Data *line = LineSensor_GetData();

    OLED_Clear();
    OLED_Printf(0, 0, OLED_6X8, "MS:%10lu",
                (unsigned long) BSP_Time_GetMs());
    OLED_Printf(0, 8, OLED_6X8, "FRAME:%8lu",
                (unsigned long) BSP_LineADC_GetFrameCount());
    OLED_Printf(0, 16, OLED_6X8, "RST:%5lu IRQ:%5lu",
                (unsigned long) BSP_LineADC_GetRestartCount(),
                (unsigned long) BSP_LineADC_GetUnexpectedIrqCount());
    OLED_Printf(0, 24, OLED_6X8, "0:%4u 1:%4u",
                (unsigned int) line->raw[0], (unsigned int) line->raw[1]);
    OLED_Printf(0, 32, OLED_6X8, "2:%4u 3:%4u",
                (unsigned int) line->raw[2], (unsigned int) line->raw[3]);
    OLED_Printf(0, 40, OLED_6X8, "4:%4u 5:%4u",
                (unsigned int) line->raw[4], (unsigned int) line->raw[5]);
    OLED_Printf(0, 48, OLED_6X8, "6:%4u 7:%4u",
                (unsigned int) line->raw[6], (unsigned int) line->raw[7]);
    OLED_Printf(0, 56, OLED_6X8, "STATE:%u LINE:%u",
                (unsigned int) App_Car_GetState(),
                line->lineDetected ? 1U : 0U);
}

/** @brief 在 RAM 显存生成电机/编码器画面；整数显示避免浮点 printf。 */
static void render_oled_motor_diagnostics(void)
{
    const Encoder_Data *left = Encoder_GetLeft();
    const Encoder_Data *right = Encoder_GetRight();
    const Motor_Status *motor = Motor_GetStatus();
    const SpeedControl_Status *speed = SpeedControl_GetStatus();
    BSP_Encoder_Diagnostics encoderDiag;

    BSP_Encoder_GetDiagnostics(&encoderDiag);
    OLED_Clear();
    OLED_Printf(0, 0, OLED_6X8, "ENC/MOTOR S:%u",
                (unsigned int) App_Car_GetState());
    OLED_Printf(0, 8, OLED_6X8, "LC:%7ld RC:%7ld",
                (long) left->count, (long) right->count);
    OLED_Printf(0, 16, OLED_6X8, "LD:%6ld RD:%6ld",
                (long) left->deltaCount, (long) right->deltaCount);
    OLED_Printf(0, 24, OLED_6X8, "LV:%6ld RV:%6ld",
                (long) left->speedMmS, (long) right->speedMmS);
    OLED_Printf(0, 32, OLED_6X8, "VT:%6ld %6ld",
                (long) speed->leftTargetMmS, (long) speed->rightTargetMmS);
    OLED_Printf(0, 40, OLED_6X8, "MT:%6d %6d",
                (int) motor->leftTargetPermille,
                (int) motor->rightTargetPermille);
    OLED_Printf(0, 48, OLED_6X8, "MA:%6d %6d E:%u",
                (int) motor->leftAppliedPermille,
                (int) motor->rightAppliedPermille,
                motor->enabled ? 1U : 0U);
    OLED_Printf(0, 56, OLED_6X8, "IRQ:%7lu ERR:%lu",
                (unsigned long) encoderDiag.edgeInterrupts,
                (unsigned long) encoderDiag.invalidTransitions);
}

/**
 * @brief 在 RAM 显存生成 IMU/角速度内环画面。
 *
 * 所有浮点量先乘 10 转成整数：例如 G10=123 表示实际角速度 12.3 deg/s。
 * 这样无需给嵌入式 printf 链接浮点格式化支持，也方便直接判断物理方向。
 */
static void render_oled_imu_diagnostics(void)
{
    const IMU_Data *imu = IMU_GetData();
    const IMU_Diagnostics *imuDiag = IMU_GetDiagnostics();
    const LineControl_Status *lineControl = LineControl_GetStatus();

    OLED_Clear();
    OLED_Printf(0, 0, OLED_6X8, "IMU/YAW S:%u",
                (unsigned int) App_Car_GetState());
    OLED_Printf(0, 8, OLED_6X8, "V:%u A:%u CAL:%u",
                imu->valid ? 1U : 0U,
                lineControl->yawRateControlActive ? 1U : 0U,
                imuDiag->calibrating ? 1U : 0U);
    /* 128/6 只能完整容纳 21 字符；Y/G 数值仍然都是实际量乘 10。 */
    OLED_Printf(0, 16, OLED_6X8, "Y:%7ld G:%7ld",
                (long) (imu->yawDegrees * 10.0f),
                (long) (imu->gyroZDps * 10.0f));
    OLED_Printf(0, 24, OLED_6X8, "T10:%6ld C:%6ld",
                (long) (lineControl->targetYawRateDps * 10.0f),
                (long) lineControl->yawRateCorrectionMmS);
    OLED_Printf(0, 32, OLED_6X8, "LS:%6ld OUT:%6ld",
                (long) lineControl->lineSteeringMmS,
                (long) lineControl->finalSteeringMmS);
    OLED_Printf(0, 40, OLED_6X8, "BIAS10:%6ld EN:%3lu",
                (long) (imu->gyroBiasDps * 10.0f),
                (unsigned long) (lineControl->imuEngageRatio * 100.0f));
    OLED_Printf(0, 48, OLED_6X8, "FR:%7lu CRC:%5lu",
                (unsigned long) imuDiag->validFrames,
                (unsigned long) imuDiag->crcErrors);
    OLED_Printf(0, 56, OLED_6X8, "DROP:%5lu OVF:%5lu",
                (unsigned long) imuDiag->droppedFrames,
                (unsigned long) imuDiag->uartRxOverflows);
}

/** @brief 只在完整 8 页边界切换画面，避免同一轮刷新混入两套页面内容。 */
static void render_oled_diagnostics(void)
{
    if (g_oledView == 0U) {
        render_oled_line_diagnostics();
    } else if (g_oledView == 1U) {
        render_oled_motor_diagnostics();
    } else {
        render_oled_imu_diagnostics();
    }
}
#endif

void App_Debug_Init(void)
{
    BSP_UART_Init();
    PIDDebug_Init();

#if CAR_OLED_SOFT_I2C_READY
    /*
     * OLED_Init 会发送初始化命令并清屏，只在上电初始化执行一次。此时车辆仍保持
     * IDLE 且 TB6612 未使能，故一次完整清屏不会影响运动控制时序。
     */
    g_oledNextPage = 0U;
    g_oledView = 0U;
    OLED_Init();
    g_oledConnected = OLED_IsConnected();
#endif
}

void App_Debug_PollCommands(void)
{
    uint8_t command;
    uint32_t now = BSP_Time_GetMs();

    /*
     * 不完整的 '[' 包超过 250 ms 后自动退出，防止一次丢失的 ']' 永久吞掉后续
     * R/S/X 等单字符命令。超时时间由 car_config.h 集中配置。
     */
    PIDDebug_CheckTimeout(now);
    /* 一次清空当前 RX 队列；底层无数据时立即退出。 */
    while (BSP_UART_TryReadByte(&command)) {
        now = BSP_Time_GetMs();
        /* 方括号包中的 R/C/I 等字符只属于文本协议，绝不能触发旧车辆命令。 */
        if (PIDDebug_PushRxByte(command, now)) continue;

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
#if CAR_DEBUG_CSV_ENABLE
    char buffer[160];
    int length;
    const LineSensor_Data *line = LineSensor_GetData();
    const Encoder_Data *left = Encoder_GetLeft();
    const Encoder_Data *right = Encoder_GetRight();
    const Motor_Status *motor = Motor_GetStatus();
    const IMU_Data *imu = IMU_GetData();
    const IMU_Diagnostics *imuDiag = IMU_GetDiagnostics();
    const LineControl_Status *lineControl = LineControl_GetStatus();

    /*
     * 字段顺序：时间、状态、位置×1000、左右速度、左右占空比、是否在线、
     * IMU有效、偏航角×10、角速度×10、IMU CRC错误数、角速度内环是否生效、
     * 目标角速度×10、IMU 修正量和最终转向量。
     * 只格式化整数，避免链接浮点 printf。
     */
    length = snprintf(buffer, sizeof(buffer),
        "%lu,%u,%ld,%ld,%ld,%d,%d,%u,%u,%ld,%ld,%lu,%u,%ld,%ld,%ld\r\n",
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
        (unsigned long) imuDiag->crcErrors,
        lineControl->yawRateControlActive ? 1U : 0U,
        (long) (lineControl->targetYawRateDps * 10.0f),
        (long) lineControl->yawRateCorrectionMmS,
        (long) lineControl->finalSteeringMmS);
    if (length > 0) {
        /* snprintf 返回期望长度，缓冲截断时必须限制实际发送长度。 */
        size_t sendLength = (size_t) length;
        if (sendLength >= sizeof(buffer)) sendLength = sizeof(buffer) - 1U;
        /* 队列满时允许丢帧；遥测优先级低于控制。 */
        (void) BSP_UART_TryWrite((const uint8_t *) buffer, sendLength);
    }
#endif

#if CAR_PID_DEBUG_ENABLE
    /* display/plot 格式化和 PID 状态读取只在 20 ms 低频主循环任务进行。 */
    PIDDebug_Task();
#endif
}

void App_Debug_OLEDTask(void)
{
#if CAR_OLED_SOFT_I2C_READY
    uint8_t page;

    /* 初始化阶段任意字节 NACK 后停止周期刷新，避免无设备时持续占用 CPU。 */
    if (!g_oledConnected) return;

    render_oled_diagnostics();
    page = g_oledNextPage;

    /*
     * 只把当前 8 像素页从 RAM 显存发送到 SSD1306。软件 I2C 仍是同步协议时序，
     * 但单次传输从全屏 1024 字节降到最多 120 字节。
     */
    /* 两套画面长度不同，整页 128 像素刷新可彻底清掉上一画面残留字符。 */
    OLED_UpdateArea(0, (int16_t) page * 8, 128U, 8U);
    g_oledConnected = OLED_IsConnected();
    g_oledNextPage = (uint8_t) ((page + 1U) % APP_OLED_PAGE_COUNT);
    /* 三个诊断画面依次轮换：ADC/循迹 → 电机/编码器 → IMU/角速度环。 */
    if (g_oledNextPage == 0U) {
        g_oledView = (uint8_t) ((g_oledView + 1U) % 3U);
    }
#endif
}
