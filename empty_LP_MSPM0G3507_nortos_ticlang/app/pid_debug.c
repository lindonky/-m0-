/*
 * 江协科技蓝牙小程序文本协议和 PID 在线调参。
 *
 * 参考 STM32 例程在 UART ISR 中寻找 '['、']' 并写一个 100 字节全局数组，发送函数
 * 则逐字节等待 TXE。本实现只保留线上的文本协议：ISR 仍然只搬运 UART 字节，括号
 * 状态机、字段解析、浮点转换和 PID 修改全部在 NoRTOS 主循环完成；发送只尝试把
 * 完整结果复制到 512 字节 TX 环形队列，不会忙等 HC-05。
 */
#include "app/pid_debug.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "bsp/bsp_time.h"
#include "bsp/bsp_uart.h"
#include "config/car_config.h"
#include "control/line_control.h"
#include "control/pid.h"
#include "control/speed_control.h"
#include "drivers/line_sensor.h"

#define PID_DEBUG_RX_PACKET_SIZE (96U)
#define PID_DEBUG_TX_TEXT_SIZE    (192U)

static char g_rxPacket[PID_DEBUG_RX_PACKET_SIZE];
static uint16_t g_rxLength;
static uint32_t g_lastRxByteMs;
static uint32_t g_nextPlotMs;
static uint32_t g_nextDisplayMs;
static bool g_receivingPacket;
static bool g_discardingOverflow;
static bool g_displayDirty;
static PIDDebug_Diagnostics g_diagnostics;

static bool due(uint32_t now, uint32_t deadline)
{
    return ((int32_t) (now - deadline) >= 0);
}

static bool ascii_equal_ignore_case(const char *left, const char *right)
{
    if ((left == NULL) || (right == NULL)) return false;

    while ((*left != '\0') && (*right != '\0')) {
        char a = *left++;
        char b = *right++;
        if ((a >= 'A') && (a <= 'Z')) a = (char) (a - 'A' + 'a');
        if ((b >= 'A') && (b <= 'Z')) b = (char) (b - 'A' + 'a');
        if (a != b) return false;
    }
    return (*left == '\0') && (*right == '\0');
}

/** 去掉字段首尾空格，便于兼容手机端偶然发送的 `, 1,`。 */
static char *trim_ascii_spaces(char *text)
{
    char *end;

    if (text == NULL) return NULL;
    while ((*text == ' ') || (*text == '\t')) text++;
    end = text + strlen(text);
    while ((end > text) && ((end[-1] == ' ') || (end[-1] == '\t'))) end--;
    *end = '\0';
    return text;
}

/**
 * 从可修改缓冲区取得下一个逗号字段。本函数不使用 strtok，因此不会占用全局解析
 * 状态，也不会影响工程未来其他协议解析器。
 */
static char *take_field(char **cursor)
{
    char *field;
    char *comma;

    if ((cursor == NULL) || (*cursor == NULL)) return NULL;
    field = *cursor;
    comma = strchr(field, ',');
    if (comma != NULL) {
        *comma = '\0';
        *cursor = comma + 1;
    } else {
        *cursor = NULL;
    }
    return trim_ascii_spaces(field);
}

/**
 * 有界十进制浮点解析，只接受普通 `[-]123.456`，不接受 NaN、Inf 或指数形式。
 * 不调用 atof/strtof，可明确检测非法尾随字符并避免不同 libc 的宽松行为。
 */
static bool parse_decimal(const char *text, float *result)
{
    bool negative = false;
    bool haveDigit = false;
    float value = 0.0f;
    float fractionScale = 0.1f;

    if ((text == NULL) || (result == NULL)) return false;
    if ((*text == '+') || (*text == '-')) {
        negative = (*text == '-');
        text++;
    }

    while ((*text >= '0') && (*text <= '9')) {
        haveDigit = true;
        value = value * 10.0f + (float) (*text - '0');
        if (value > CAR_PID_DEBUG_GAIN_MAX) return false;
        text++;
    }

    if (*text == '.') {
        text++;
        while ((*text >= '0') && (*text <= '9')) {
            haveDigit = true;
            value += (float) (*text - '0') * fractionScale;
            fractionScale *= 0.1f;
            text++;
        }
    }

    if (!haveDigit || (*text != '\0')) return false;
    if (negative) value = -value;
    if ((value < 0.0f) || (value > CAR_PID_DEBUG_GAIN_MAX)) return false;
    *result = value;
    return true;
}

static PID_Controller *selected_pid(void)
{
    switch (g_diagnostics.selectedLoop) {
    case PID_DEBUG_LOOP_LINE:        return LineControl_GetPID();
    case PID_DEBUG_LOOP_YAW_RATE:    return LineControl_GetYawRatePID();
    case PID_DEBUG_LOOP_LEFT_SPEED:  return SpeedControl_GetLeftPID();
    case PID_DEBUG_LOOP_RIGHT_SPEED: return SpeedControl_GetRightPID();
    default:                         return LineControl_GetPID();
    }
}

static const char *selected_name(void)
{
    switch (g_diagnostics.selectedLoop) {
    case PID_DEBUG_LOOP_LINE:        return "LINE";
    case PID_DEBUG_LOOP_YAW_RATE:    return "YAW";
    case PID_DEBUG_LOOP_LEFT_SPEED:  return "SPEED-L";
    case PID_DEBUG_LOOP_RIGHT_SPEED: return "SPEED-R";
    default:                         return "UNKNOWN";
    }
}

static bool select_loop_from_name(const char *name)
{
    if (ascii_equal_ignore_case(name, "1") ||
        ascii_equal_ignore_case(name, "line")) {
        g_diagnostics.selectedLoop = PID_DEBUG_LOOP_LINE;
    } else if (ascii_equal_ignore_case(name, "2") ||
               ascii_equal_ignore_case(name, "yaw")) {
        g_diagnostics.selectedLoop = PID_DEBUG_LOOP_YAW_RATE;
    } else if (ascii_equal_ignore_case(name, "3") ||
               ascii_equal_ignore_case(name, "left")) {
        g_diagnostics.selectedLoop = PID_DEBUG_LOOP_LEFT_SPEED;
    } else if (ascii_equal_ignore_case(name, "4") ||
               ascii_equal_ignore_case(name, "right")) {
        g_diagnostics.selectedLoop = PID_DEBUG_LOOP_RIGHT_SPEED;
    } else {
        return false;
    }
    g_displayDirty = true;
    return true;
}

static bool valid_key_action(const char *action)
{
    return ascii_equal_ignore_case(action, "up") ||
           ascii_equal_ignore_case(action, "down") ||
           ascii_equal_ignore_case(action, "click") ||
           ascii_equal_ignore_case(action, "press");
}

static void handle_key(char *name, char *action)
{
    if ((name == NULL) || (action == NULL) || !valid_key_action(action)) {
        g_diagnostics.formatErrors++;
        return;
    }
    g_diagnostics.keyPackets++;

    if (select_loop_from_name(name)) return;

    if (ascii_equal_ignore_case(name, "5") ||
        ascii_equal_ignore_case(name, "status")) {
        g_displayDirty = true;
    } else if (ascii_equal_ignore_case(name, "6") ||
               ascii_equal_ignore_case(name, "plot")) {
        g_diagnostics.plotEnabled = !g_diagnostics.plotEnabled;
        g_displayDirty = true;
    } else if (ascii_equal_ignore_case(name, "7") ||
               ascii_equal_ignore_case(name, "reset")) {
        /* 只清当前 PID 的积分/微分历史，不改变已经调好的 Kp/Ki/Kd。 */
        PID_Reset(selected_pid());
        g_displayDirty = true;
    } else {
        g_diagnostics.unknownPackets++;
    }
}

static void handle_slider(char *name, char *textValue)
{
    PID_Controller *pid;
    float value;

    if ((name == NULL) || !parse_decimal(textValue, &value)) {
        g_diagnostics.rejectedValues++;
        return;
    }

    pid = selected_pid();
    if (ascii_equal_ignore_case(name, "1") ||
        ascii_equal_ignore_case(name, "kp")) {
        PID_SetTunings(pid, value, pid->ki, pid->kd);
    } else if (ascii_equal_ignore_case(name, "2") ||
               ascii_equal_ignore_case(name, "ki")) {
        PID_SetTunings(pid, pid->kp, value, pid->kd);
    } else if (ascii_equal_ignore_case(name, "3") ||
               ascii_equal_ignore_case(name, "kd")) {
        PID_SetTunings(pid, pid->kp, pid->ki, value);
    } else {
        g_diagnostics.unknownPackets++;
        return;
    }

    g_diagnostics.sliderPackets++;
    g_displayDirty = true;
}

static void handle_packet(char *packet)
{
    char *cursor = packet;
    char *tag = take_field(&cursor);
    char *name = take_field(&cursor);
    char *valueOrAction = take_field(&cursor);

    g_diagnostics.receivedPackets++;
    if ((tag == NULL) || (*tag == '\0')) {
        g_diagnostics.formatErrors++;
        return;
    }

    if (ascii_equal_ignore_case(tag, "key")) {
        handle_key(name, valueOrAction);
    } else if (ascii_equal_ignore_case(tag, "slider") ||
               ascii_equal_ignore_case(tag, "slide")) {
        handle_slider(name, valueOrAction);
    } else {
        g_diagnostics.unknownPackets++;
    }
}

void PIDDebug_Init(void)
{
    uint32_t now = BSP_Time_GetMs();

    g_rxPacket[0] = '\0';
    g_rxLength = 0U;
    g_lastRxByteMs = now;
    g_receivingPacket = false;
    g_discardingOverflow = false;
    g_displayDirty = true;
    g_diagnostics = (PIDDebug_Diagnostics) {0};
    g_diagnostics.selectedLoop = PID_DEBUG_LOOP_LINE;
    g_diagnostics.plotEnabled = (CAR_PID_DEBUG_PLOT_DEFAULT != 0U);
    g_nextPlotMs = now + CAR_PID_DEBUG_PLOT_PERIOD_MS;
    g_nextDisplayMs = now + CAR_PID_DEBUG_DISPLAY_PERIOD_MS;
}

bool PIDDebug_PushRxByte(uint8_t byte, uint32_t nowMs)
{
#if CAR_PID_DEBUG_ENABLE
    if (!g_receivingPacket) {
        if (byte != (uint8_t) '[') return false;
        g_receivingPacket = true;
        g_discardingOverflow = false;
        g_rxLength = 0U;
        g_lastRxByteMs = nowMs;
        return true;
    }

    g_lastRxByteMs = nowMs;
    if (byte == (uint8_t) '[') {
        /* 新起始符说明上一包损坏；直接从新包重新同步。 */
        g_diagnostics.formatErrors++;
        g_discardingOverflow = false;
        g_rxLength = 0U;
        return true;
    }

    if (byte == (uint8_t) ']') {
        if (g_discardingOverflow) {
            g_diagnostics.overflowPackets++;
        } else {
            g_rxPacket[g_rxLength] = '\0';
            handle_packet(g_rxPacket);
        }
        g_receivingPacket = false;
        g_discardingOverflow = false;
        g_rxLength = 0U;
        return true;
    }

    if (!g_discardingOverflow) {
        if (g_rxLength < (PID_DEBUG_RX_PACKET_SIZE - 1U)) {
            g_rxPacket[g_rxLength++] = (char) byte;
        } else {
            /* 超长包持续丢弃到 ']'，避免包尾内容被误当作 R/S 等旧命令。 */
            g_discardingOverflow = true;
        }
    }
    return true;
#else
    (void) byte;
    (void) nowMs;
    return false;
#endif
}

void PIDDebug_CheckTimeout(uint32_t nowMs)
{
#if CAR_PID_DEBUG_ENABLE
    if (g_receivingPacket &&
        ((uint32_t) (nowMs - g_lastRxByteMs) >
         CAR_PID_DEBUG_PACKET_TIMEOUT_MS)) {
        g_receivingPacket = false;
        g_discardingOverflow = false;
        g_rxLength = 0U;
        g_diagnostics.timeoutPackets++;
    }
#else
    (void) nowMs;
#endif
}

bool PIDDebug_Printf(const char *format, ...)
{
    char text[PID_DEBUG_TX_TEXT_SIZE];
    va_list arguments;
    int length;

    if (format == NULL) {
        g_diagnostics.txRejected++;
        return false;
    }

    va_start(arguments, format);
    length = vsnprintf(text, sizeof(text), format, arguments);
    va_end(arguments);

    if (length < 0) {
        g_diagnostics.txRejected++;
        return false;
    }
    if ((size_t) length >= sizeof(text)) {
        /* 不发送被截断的半个方括号包，否则手机端会一直等待不存在的 ']'. */
        g_diagnostics.txTruncated++;
        return false;
    }
    if (!BSP_UART_TryWrite((const uint8_t *) text, (size_t) length)) {
        g_diagnostics.txRejected++;
        return false;
    }
    return true;
}

bool PIDDebug_DisplayText(int16_t x, int16_t y, const char *text)
{
    if (text == NULL) return false;
    return PIDDebug_Printf("[display,%d,%d,%s]", (int) x, (int) y, text);
}

bool PIDDebug_Plot2(float y1, float y2)
{
    return PIDDebug_Printf("[plot,%.3f,%.3f]", (double) y1, (double) y2);
}

/** 一次原子发送三条连续 display 包，避免 TX 空间不足时只更新半套参数。 */
static bool send_parameter_display(void)
{
    const PID_Controller *pid = selected_pid();

    return PIDDebug_Printf(
        "[display,0,0,PID:%s]"
        "[display,0,20,Kp:%.3f Ki:%.3f]"
        "[display,0,40,Kd:%.3f Plot:%u]",
        selected_name(), (double) pid->kp, (double) pid->ki,
        (double) pid->kd, g_diagnostics.plotEnabled ? 1U : 0U);
}

static void get_plot_values(float *target, float *measured)
{
    const LineControl_Status *lineStatus;
    const SpeedControl_Status *speedStatus;

    switch (g_diagnostics.selectedLoop) {
    case PID_DEBUG_LOOP_LINE:
        *target = 0.0f;
        *measured = LineSensor_GetData()->position;
        break;
    case PID_DEBUG_LOOP_YAW_RATE:
        lineStatus = LineControl_GetStatus();
        *target = lineStatus->targetYawRateDps;
        *measured = lineStatus->measuredYawRateDps;
        break;
    case PID_DEBUG_LOOP_LEFT_SPEED:
        speedStatus = SpeedControl_GetStatus();
        *target = speedStatus->leftTargetMmS;
        *measured = speedStatus->leftMeasuredMmS;
        break;
    case PID_DEBUG_LOOP_RIGHT_SPEED:
        speedStatus = SpeedControl_GetStatus();
        *target = speedStatus->rightTargetMmS;
        *measured = speedStatus->rightMeasuredMmS;
        break;
    default:
        *target = 0.0f;
        *measured = 0.0f;
        break;
    }
}

void PIDDebug_Task(void)
{
#if CAR_PID_DEBUG_ENABLE
    uint32_t now = BSP_Time_GetMs();

    /* 周期重发参数，使手机在晚于 MCU 上电时连接也能自动得到当前页面内容。 */
    if (due(now, g_nextDisplayMs)) {
        g_nextDisplayMs = now + CAR_PID_DEBUG_DISPLAY_PERIOD_MS;
        g_displayDirty = true;
    }
    if (g_displayDirty && send_parameter_display()) g_displayDirty = false;

    if (g_diagnostics.plotEnabled && due(now, g_nextPlotMs)) {
        float target;
        float measured;
        g_nextPlotMs = now + CAR_PID_DEBUG_PLOT_PERIOD_MS;
        get_plot_values(&target, &measured);
        (void) PIDDebug_Plot2(target, measured);
    }
#endif
}

const PIDDebug_Diagnostics *PIDDebug_GetDiagnostics(void)
{
    return &g_diagnostics;
}
