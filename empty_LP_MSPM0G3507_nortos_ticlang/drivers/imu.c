/*
 * 500 Hz 串口陀螺仪驱动。
 *
 * 参考程序把 CRC 放在 motor_crc.c，并在每次接收字节时重复生成测试数据。本实现
 * 将协议真正需要的 Modbus CRC16 收进 IMU 内部，删除 512 字节查表和无关测试
 * 全局变量。500 Hz×9 字节的负载很小，紧凑位算法足以满足 MSPM0G3507 实时性。
 */
#include "drivers/imu.h"

#include <stddef.h>
#include "bsp/bsp_imu_uart.h"
#include "config/car_config.h"

#define IMU_FRAME_SIZE              (9U)
#define IMU_CRC_DATA_SIZE           (7U)
#define IMU_FRAME_ADDRESS           (0x0AU)
#define IMU_FRAME_FUNCTION          (0x03U)
#define IMU_FRAME_PAYLOAD_SIZE      (0x04U)
#define IMU_CONFIG_COMMAND_SIZE     (7U)

typedef struct {
    uint8_t bytes[IMU_FRAME_SIZE];
    uint8_t index;
} IMU_Parser;

/*
 * 完整帧发布区。sequence 最后写入；若将 IMU_PushRxByte 放在 ISR 中，主循环可用
 * 前后两次 sequence 检查避免读到一半更新的数据。
 */
static volatile int16_t g_publishedAngleRaw;
static volatile int16_t g_publishedDpsRaw;
static volatile uint32_t g_publishedSequence;

static IMU_Parser g_parser;
static IMU_Data g_data;
static IMU_Diagnostics g_diagnostics;
static uint32_t g_consumedSequence;
static int16_t g_lastAngleRaw;
static bool g_haveLastAngle;
static float g_sampleAgeSeconds;
static float g_configElapsedSeconds;
static bool g_uartReady;
static float g_calibrationSumDps;

/**
 * Modbus CRC16：初值 0xFFFF，反射多项式 0xA001。
 * 返回的 uint16_t 数值可直接按低字节、高字节顺序放入串口帧。
 */
static uint16_t crc16_modbus(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFFU;
    size_t i;

    for (i = 0U; i < length; ++i) {
        uint8_t bit;
        crc ^= data[i];
        for (bit = 0U; bit < 8U; ++bit) {
            if ((crc & 1U) != 0U) {
                crc = (uint16_t) ((crc >> 1U) ^ 0xA001U);
            } else {
                crc >>= 1U;
            }
        }
    }
    return crc;
}

static int16_t read_i16_be(const uint8_t *data)
{
    return (int16_t) (((uint16_t) data[0] << 8U) | data[1]);
}

/** 在帧头匹配失败时保留可能重叠的 0x0A，缩短噪声后的重新同步时间。 */
static void parser_restart_from(uint8_t byte)
{
    if (byte == IMU_FRAME_ADDRESS) {
        g_parser.bytes[0] = byte;
        g_parser.index = 1U;
    } else {
        g_parser.index = 0U;
    }
}

bool IMU_PushRxByte(uint8_t byte)
{
    uint16_t crcCalculated;
    uint16_t crcReceived;

    switch (g_parser.index) {
    case 0U:
        if (byte == IMU_FRAME_ADDRESS) {
            g_parser.bytes[0] = byte;
            g_parser.index = 1U;
        }
        return false;

    case 1U:
        if (byte != IMU_FRAME_FUNCTION) {
            g_diagnostics.formatErrors++;
            parser_restart_from(byte);
            return false;
        }
        g_parser.bytes[g_parser.index++] = byte;
        return false;

    case 2U:
        if (byte != IMU_FRAME_PAYLOAD_SIZE) {
            g_diagnostics.formatErrors++;
            parser_restart_from(byte);
            return false;
        }
        g_parser.bytes[g_parser.index++] = byte;
        return false;

    default:
        g_parser.bytes[g_parser.index++] = byte;
        break;
    }

    if (g_parser.index < IMU_FRAME_SIZE) return false;
    g_parser.index = 0U;

    crcCalculated = crc16_modbus(g_parser.bytes, IMU_CRC_DATA_SIZE);
    crcReceived = (uint16_t) g_parser.bytes[7] |
                  ((uint16_t) g_parser.bytes[8] << 8U);
    if (crcCalculated != crcReceived) {
        g_diagnostics.crcErrors++;
        return false;
    }

    /* 两个字段全部写完后再递增序号，向主循环原子发布这帧。 */
    g_publishedAngleRaw = read_i16_be(&g_parser.bytes[3]);
    g_publishedDpsRaw = read_i16_be(&g_parser.bytes[5]);
    g_publishedSequence++;
    g_diagnostics.validFrames++;
    return true;
}

/** 使用内部 CRC 生成参考程序中的 AA 06 01 01 01 AD 00 配置命令。 */
static bool try_queue_report_rate_command(void)
{
    uint8_t command[IMU_CONFIG_COMMAND_SIZE] = {
        0xAAU, 0x06U, 0x01U, 0x01U, 0x01U, 0x00U, 0x00U
    };
    uint16_t crc = crc16_modbus(command, 5U);

    command[5] = (uint8_t) crc;
    command[6] = (uint8_t) (crc >> 8U);
    return BSP_IMU_UART_TryWrite(command, sizeof(command));
}

/** 尝试取得一组前后一致的已发布原始值。 */
static bool copy_latest_raw(int16_t *angleRaw, int16_t *dpsRaw,
                            uint32_t *sequence)
{
    uint32_t before;
    uint32_t after;
    uint8_t attempts;

    for (attempts = 0U; attempts < 3U; ++attempts) {
        before = g_publishedSequence;
        *angleRaw = g_publishedAngleRaw;
        *dpsRaw = g_publishedDpsRaw;
        after = g_publishedSequence;
        if (before == after) {
            *sequence = after;
            return true;
        }
    }
    return false;
}

/** 把最新原始帧转换为工程统一单位，并完成回绕、零偏和丢帧处理。 */
static bool consume_latest_sample(void)
{
    int16_t angleRaw;
    int16_t dpsRaw;
    uint32_t sequence;
    float measuredDps;

    if (!copy_latest_raw(&angleRaw, &dpsRaw, &sequence) ||
        (sequence == g_consumedSequence)) {
        return false;
    }

    if ((g_consumedSequence != 0U) &&
        ((uint32_t) (sequence - g_consumedSequence) > 1U)) {
        g_diagnostics.droppedFrames +=
            (uint32_t) (sequence - g_consumedSequence - 1U);
    }
    g_consumedSequence = sequence;

    g_data.sensorAngleDegrees = (float) angleRaw * CAR_IMU_RAW_TO_DEG *
                                CAR_IMU_YAW_POLARITY;
    measuredDps = (float) dpsRaw * CAR_IMU_RAW_TO_DEG *
                  CAR_IMU_YAW_POLARITY;

    if (!g_haveLastAngle) {
        g_lastAngleRaw = angleRaw;
        g_haveLastAngle = true;
    } else {
        /*
         * 先按 uint16_t 做模减法，再解释为 int16_t，可正确处理 32767→-32768
         * 和反方向回绕。500 Hz 下相邻角度不可能真实跳变超过 3276.8°。
         */
        int16_t deltaRaw =
            (int16_t) ((uint16_t) angleRaw - (uint16_t) g_lastAngleRaw);
        g_data.yawDegrees += (float) deltaRaw * CAR_IMU_RAW_TO_DEG *
                             CAR_IMU_YAW_POLARITY;
        g_lastAngleRaw = angleRaw;
    }

    if (g_diagnostics.calibrating) {
        g_calibrationSumDps += measuredDps;
        g_diagnostics.calibrationSamples++;
        if (g_diagnostics.calibrationSamples >=
            CAR_IMU_CALIBRATION_SAMPLES) {
            g_data.gyroBiasDps = g_calibrationSumDps /
                                 (float) g_diagnostics.calibrationSamples;
            g_diagnostics.calibrating = false;
        }
    }

    g_data.gyroZDps = measuredDps - g_data.gyroBiasDps;
    g_data.valid = true;
    g_data.newSample = true;
    g_sampleAgeSeconds = 0.0f;
    return true;
}

bool IMU_Init(void)
{
    g_parser = (IMU_Parser) {0};
    g_data = (IMU_Data) {0};
    g_diagnostics = (IMU_Diagnostics) {0};
    g_publishedAngleRaw = 0;
    g_publishedDpsRaw = 0;
    g_publishedSequence = 0U;
    g_consumedSequence = 0U;
    g_lastAngleRaw = 0;
    g_haveLastAngle = false;
    g_sampleAgeSeconds = 0.0f;
    g_configElapsedSeconds = 0.0f;
    g_calibrationSumDps = 0.0f;
    g_uartReady = BSP_IMU_UART_Init();
    return g_uartReady;
}

bool IMU_Update(float dtSeconds)
{
    uint8_t byte;
    uint16_t parsedBytes = 0U;
    bool newSample;

    if (dtSeconds < 0.0f) dtSeconds = 0.0f;
    g_data.newSample = false;

    /*
     * 沿用参考程序“上电后等待 3 秒再配置”的意图，但这里仅累计时间，不忙等。
     * TX 队列暂满时下次 1 ms 任务继续尝试。
     */
    if (g_uartReady && !g_diagnostics.configCommandQueued) {
        g_configElapsedSeconds += dtSeconds;
        if (g_configElapsedSeconds >=
            ((float) CAR_IMU_STARTUP_CONFIG_DELAY_MS * 0.001f)) {
            g_diagnostics.configCommandQueued =
                try_queue_report_rate_command();
        }
    }

    while ((parsedBytes < CAR_IMU_MAX_BYTES_PER_UPDATE) &&
           BSP_IMU_UART_TryReadByte(&byte)) {
        (void) IMU_PushRxByte(byte);
        parsedBytes++;
    }

    newSample = consume_latest_sample();
    if (!newSample) {
        g_sampleAgeSeconds += dtSeconds;
        if (g_sampleAgeSeconds >
            ((float) CAR_IMU_DATA_TIMEOUT_MS * 0.001f)) {
            g_data.valid = false;
        }
    }

    g_diagnostics.uartRxOverflows = BSP_IMU_UART_GetRxOverflowCount();
    return newSample;
}

void IMU_StartGyroCalibration(void)
{
    g_calibrationSumDps = 0.0f;
    g_diagnostics.calibrationSamples = 0U;
    g_diagnostics.calibrating = true;
}

void IMU_ResetYaw(void)
{
    g_data.yawDegrees = 0.0f;
}

const IMU_Data *IMU_GetData(void)
{
    return &g_data;
}

const IMU_Diagnostics *IMU_GetDiagnostics(void)
{
    return &g_diagnostics;
}
