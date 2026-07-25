/*
 * 循迹方向环。
 *
 * 方向 PID 的输出单位是 mm/s 速度差，不是 PWM。这样电池电压和电机左右差异由
 * 下面的轮速闭环处理。线偏得越多，基础前进速度越低，给车辆留下更大转弯余量。
 */
#include "control/line_control.h"
#include "config/car_config.h"

static PID_Controller g_linePid;
static float g_lastPosition;

static float absolute(float value) { return (value < 0.0f) ? -value : value; }
static float maximum(float left, float right) { return (left > right) ? left : right; }

void LineControl_Init(void)
{
    PID_Init(&g_linePid, CAR_LINE_KP, CAR_LINE_KI, CAR_LINE_KD,
             -CAR_LINE_STEERING_LIMIT_MM_S, CAR_LINE_STEERING_LIMIT_MM_S);
    /* 方向环默认 Ki=0；仍限制积分，避免在线打开 Ki 时瞬间积累过大。 */
    PID_SetIntegralLimits(&g_linePid, -1.0f, 1.0f);
    PID_SetDerivativeFilter(&g_linePid, 0.85f);
    g_lastPosition = 0.0f;
}

void LineControl_Reset(void)
{
    PID_Reset(&g_linePid);
    g_lastPosition = 0.0f;
}

LineControl_Output LineControl_Update(const LineSensor_Data *line,
                                      float dtSeconds)
{
    LineControl_Output output;
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
        output.steeringMmS = CAR_LINE_STEERING_POLARITY *
            PID_Update(&g_linePid, 0.0f, -line->position, dtSeconds);

        /* 使用位置绝对值做首版弯道降速，最低不低于配置的曲线速度。 */
        slowdown = CAR_CURVE_SLOWDOWN_GAIN * absolute(line->position);
        output.forwardMmS = maximum(CAR_MIN_CURVE_SPEED_MM_S,
                                    CAR_BASE_SPEED_MM_S - slowdown);
    } else {
        /* 短时丢线：降低前进速度，并向最后出现线的一侧持续搜索。 */
        output.forwardMmS = CAR_MIN_CURVE_SPEED_MM_S;
        output.steeringMmS = CAR_LINE_STEERING_POLARITY *
            ((g_lastPosition >= 0.0f) ?
                (0.70f * CAR_LINE_STEERING_LIMIT_MM_S) :
                (-0.70f * CAR_LINE_STEERING_LIMIT_MM_S));
    }
    return output;
}

PID_Controller *LineControl_GetPID(void) { return &g_linePid; }
