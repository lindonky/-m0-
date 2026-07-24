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
#include "bsp/bsp_line_adc.h"
#include "bsp/bsp_time.h"
#include "bsp/bsp_uart.h"
#include "config/board_config.h"
#include "control/speed_control.h"
#include "drivers/encoder.h"
#include "drivers/imu.h"
#include "drivers/line_sensor.h"
#include "drivers/motor.h"
#include "drivers/oled.h"

#if CAR_OLED_SOFT_I2C_READY
/* SSD1306 的 64 像素高度正好分成 8 个页，每页高 8 像素。 */
#define APP_OLED_PAGE_COUNT (8U)

/*
 * 每行实际使用的像素宽度。按内容截短更新区域，可进一步减少软件 I2C 占用；
 * 第 2 行含 RST/IRQ 两个计数，最长，因此保留 120 像素。
 */
static const uint8_t g_oledPageWidth[APP_OLED_PAGE_COUNT] = {
    84U, 90U, 120U, 84U, 84U, 84U, 84U, 96U
};

static uint8_t g_oledNextPage;
static bool g_oledConnected;

/** @brief 在 RAM 显存中生成完整诊断画面；本函数本身不访问 I2C。 */
static void render_oled_diagnostics(void)
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
#endif

void App_Debug_Init(void)
{
    BSP_UART_Init();

#if CAR_OLED_SOFT_I2C_READY
    /*
     * OLED_Init 会发送初始化命令并清屏，只在上电初始化执行一次。此时车辆仍保持
     * IDLE 且 TB6612 未使能，故一次完整清屏不会影响运动控制时序。
     */
    g_oledNextPage = 0U;
    OLED_Init();
    g_oledConnected = OLED_IsConnected();
#endif
}

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
    OLED_UpdateArea(0, (int16_t) page * 8, g_oledPageWidth[page], 8U);
    g_oledConnected = OLED_IsConnected();
    g_oledNextPage = (uint8_t) ((page + 1U) % APP_OLED_PAGE_COUNT);
#endif
}
