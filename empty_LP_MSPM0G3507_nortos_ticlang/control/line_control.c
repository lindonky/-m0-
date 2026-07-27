/*
 * 循迹方向环和 IMU 偏航角速度内环。
 *
 * 灰度方向 PID 的输出单位是 mm/s 速度差，不是 PWM。该基础转向进一步换算为目标
 * 偏航角速度，IMU 角速度 PID 只生成一个有界修正量。这样电池电压和电机左右差异
 * 仍由下面的轮速闭环处理，陀螺仪也不会越过分层直接操作 PWM。
 */
#include "control/line_control.h"
#include "config/car_config.h"

static PID_Controller g_linePid;
static PID_Controller g_yawRatePid;
static float g_lastPosition;
static LineControl_Status g_status;

static float absolute(float value) { return (value < 0.0f) ? -value : value; }
static float maximum(float left, float right) { return (left > right) ? left : right; }
static float clamp(float value, float minimum, float maximumValue)
{
    if (value > maximumValue) return maximumValue;
    if (value < minimum) return minimum;
    return value;
}

void LineControl_Init(void)
{
    PID_Init(&g_linePid, CAR_LINE_KP, CAR_LINE_KI, CAR_LINE_KD,
             -CAR_LINE_STEERING_LIMIT_MM_S, CAR_LINE_STEERING_LIMIT_MM_S);
    /* 方向环默认 Ki=0；仍限制积分，避免在线打开 Ki 时瞬间积累过大。 */
    PID_SetIntegralLimits(&g_linePid, -1.0f, 1.0f);
    PID_SetDerivativeFilter(&g_linePid, 0.85f);

    /*
     * 角速度环的输入单位是 deg/s，输出单位是附加转向 mm/s。
     * 首版 Kd=0，仍设置滤波参数，方便后续在线打开 Kd 时抑制串口量化噪声。
     */
    PID_Init(&g_yawRatePid, CAR_YAW_RATE_KP, CAR_YAW_RATE_KI,
             CAR_YAW_RATE_KD, -CAR_YAW_RATE_CORRECTION_LIMIT_MM_S,
             CAR_YAW_RATE_CORRECTION_LIMIT_MM_S);
    PID_SetIntegralLimits(&g_yawRatePid,
                          -CAR_YAW_RATE_CORRECTION_LIMIT_MM_S,
                          CAR_YAW_RATE_CORRECTION_LIMIT_MM_S);
    PID_SetDerivativeFilter(&g_yawRatePid, 0.85f);
    g_lastPosition = 0.0f;
    g_status = (LineControl_Status) {0};
}

void LineControl_Reset(void)
{
    PID_Reset(&g_linePid);
    PID_Reset(&g_yawRatePid);
    g_lastPosition = 0.0f;
    g_status = (LineControl_Status) {0};
}

LineControl_Output LineControl_Update(const LineSensor_Data *line,
                                      float measuredYawRateDps,
                                      bool yawRateValid,
                                      float dtSeconds)
{
    LineControl_Output output;
    float lineSteeringMmS;
    float targetYawRateDps;
    float rawCorrectionMmS;
    float slowdown;

    if (line->lineDetected) {
        g_lastPosition = line->position;

        /*
         * PID 的定义是 setpoint-measurement。传入 -position，使右偏（position>0）
         * 得到正 steering，匹配“右转时左轮快、右轮慢”的混合符号约定。
         *
         * CAR_LINE_STEERING_POLARITY 只适配整车实际安装后的转向极性，不改变 ADC
         * 黑白极性、位置左右定义、电机前进极性或编码器方向。当前实车验证需要
         * -1.0f，因此在 PID 输出形成后整体翻转 P/I/D 三项，避免只反转其中一项。
         */
        lineSteeringMmS = CAR_LINE_STEERING_POLARITY *
            PID_Update(&g_linePid, 0.0f, -line->position, dtSeconds);

        /* 使用位置绝对值做首版弯道降速，最低不低于配置的曲线速度。 */
        slowdown = CAR_CURVE_SLOWDOWN_GAIN * absolute(line->position);
        output.forwardMmS = maximum(CAR_MIN_CURVE_SPEED_MM_S,
                                    CAR_BASE_SPEED_MM_S - slowdown);
    } else {
        /* 短时丢线：降低前进速度，并向最后出现线的一侧持续搜索。 */
        output.forwardMmS = CAR_MIN_CURVE_SPEED_MM_S;
        lineSteeringMmS = CAR_LINE_STEERING_POLARITY *
            ((g_lastPosition >= 0.0f) ?
                (0.70f * CAR_LINE_STEERING_LIMIT_MM_S) :
                (-0.70f * CAR_LINE_STEERING_LIMIT_MM_S));
    }

    /*
     * 灰度转向是已经验证可工作的前馈量。按经验比例将其映射为目标角速度，再让
     * IMU 内环只修正“命令转弯速度”和“实际转弯速度”的差，不锁死全局航向。
     */
    targetYawRateDps = clamp(lineSteeringMmS * CAR_YAW_RATE_TARGET_GAIN,
                             -CAR_YAW_RATE_TARGET_LIMIT_DPS,
                             CAR_YAW_RATE_TARGET_LIMIT_DPS);

#if CAR_YAW_RATE_CONTROL_ENABLE
    if (yawRateValid && (dtSeconds > 0.0f)) {
        rawCorrectionMmS = PID_Update(&g_yawRatePid, targetYawRateDps,
                                      measuredYawRateDps, dtSeconds);

        /*
         * 新收到有效 IMU 后逐步接入修正。即使车辆在 IMU 上电配置阶段已经启动，
         * 角速度反馈也不会从 0 突然跳到最大修正。
         */
        if (CAR_YAW_RATE_ENGAGE_TIME_S > 0.0f) {
            g_status.imuEngageRatio +=
                dtSeconds / CAR_YAW_RATE_ENGAGE_TIME_S;
            g_status.imuEngageRatio =
                clamp(g_status.imuEngageRatio, 0.0f, 1.0f);
        } else {
            g_status.imuEngageRatio = 1.0f;
        }
        g_status.yawRateCorrectionMmS =
            rawCorrectionMmS * g_status.imuEngageRatio;
        g_status.yawRateControlActive = true;
    } else {
        /*
         * 串口超时、CRC 连续失败或正在静止标定时，立即清除内环历史并回到已经
         * 验证的灰度转向。Motor 层仍会对最终占空比做 5 ms 斜坡限制。
         */
        PID_Reset(&g_yawRatePid);
        g_status.imuEngageRatio = 0.0f;
        g_status.yawRateCorrectionMmS = 0.0f;
        g_status.yawRateControlActive = false;
    }
#else
    (void) yawRateValid;
    (void) dtSeconds;
    PID_Reset(&g_yawRatePid);
    g_status.imuEngageRatio = 0.0f;
    g_status.yawRateCorrectionMmS = 0.0f;
    g_status.yawRateControlActive = false;
#endif

    /* 基础转向和 IMU 修正共用同一最终限幅，不能突破原方向环的安全边界。 */
    output.steeringMmS = clamp(lineSteeringMmS +
                                  g_status.yawRateCorrectionMmS,
                              -CAR_LINE_STEERING_LIMIT_MM_S,
                              CAR_LINE_STEERING_LIMIT_MM_S);

    g_status.lineSteeringMmS = lineSteeringMmS;
    g_status.targetYawRateDps = targetYawRateDps;
    g_status.measuredYawRateDps = measuredYawRateDps;
    g_status.finalSteeringMmS = output.steeringMmS;
    return output;
}

PID_Controller *LineControl_GetPID(void) { return &g_linePid; }
PID_Controller *LineControl_GetYawRatePID(void) { return &g_yawRatePid; }
const LineControl_Status *LineControl_GetStatus(void) { return &g_status; }
