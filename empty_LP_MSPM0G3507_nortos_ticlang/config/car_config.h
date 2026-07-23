#ifndef CAR_CONFIG_H
#define CAR_CONFIG_H

/*
 * @file car_config.h
 * @brief 整车几何、控制周期、限幅和初始控制参数。
 *
 * 本文件只放“与车辆行为有关”的参数。具体 GPIO、定时器和 ADC 通道应记录在
 * board_config.h，并最终通过 SysConfig 生成的符号接入 BSP。
 *
 * 重要单位约定：
 * - 电机指令：有符号千分比，-1000~+1000；
 * - 速度：mm/s；
 * - 距离和轮径：mm；
 * - 控制周期：秒或毫秒，宏名中明确标出；
 * - 循迹位置：约 -1.0~+1.0。
 */

/* 200 Hz 主控制环；MS 与 S 必须保持一致。 */
#define CAR_CONTROL_PERIOD_MS              (5U)
#define CAR_CONTROL_PERIOD_S               (0.005f)

/* 电机限制。SLEW 表示每个 5 ms 控制周期最多变化多少千分比。 */
#define CAR_MOTOR_DUTY_LIMIT_PERMILLE      (950)
#define CAR_MOTOR_SLEW_PER_5MS             (40)
#define CAR_MOTOR_LEFT_POLARITY            (+1)
#define CAR_MOTOR_RIGHT_POLARITY           (+1)
#define CAR_MOTOR_DEADBAND_PERMILLE        (0)

/*
 * 编码器参数。
 * COUNTS_PER_WHEEL_REV 已按铭牌参数计算为理论 1560 count/车轮圈。
 * 该值包含编码器 PPR、AB 相四倍频和减速比；厂家对 PPR 的定义可能不同，
 * 上板后仍必须手转车轮一整圈，用 QEI 实际增量确认后再高速闭环运行。
 */
/*
 * MG513-30 减速电机铭牌参数：12 V、额定约 360 mA、堵转约 2.8 A、减速比
 * 1:30、减速后空载约 365±26 RPM，霍尔 AB 相编码器标称 13 PPR。
 *
 * 若 QEI 对 AB 两相都做上升/下降沿四倍频，则理论值：13×30×4=1560 count/轮圈。
 * 厂家对 PPR 的定义偶有差异，仍必须手转车轮一圈实测确认。
 */
#define CAR_MOTOR_RATED_VOLTAGE_V          (12.0f)
#define CAR_MOTOR_RATED_CURRENT_MA         (360U)
#define CAR_MOTOR_STALL_CURRENT_MA         (2800U)
#define CAR_MOTOR_NO_LOAD_OUTPUT_RPM       (365.0f)
#define CAR_MOTOR_GEAR_RATIO               (30.0f)
#define CAR_MOTOR_ENCODER_PPR              (13.0f)
#define CAR_ENCODER_QUADRATURE_FACTOR      (4.0f)
#define CAR_WHEEL_DIAMETER_MM              (65.0f)
#define CAR_ENCODER_COUNTS_PER_WHEEL_REV   (CAR_MOTOR_ENCODER_PPR * \
                                             CAR_MOTOR_GEAR_RATIO * \
                                             CAR_ENCODER_QUADRATURE_FACTOR)
#define CAR_ENCODER_LEFT_POLARITY          (+1)
#define CAR_ENCODER_RIGHT_POLARITY         (+1)

/* 模拟循迹阵列参数。ADC 通道在数组中的顺序必须是物理最左到最右。 */
#define CAR_LINE_SENSOR_COUNT              (8U)
#define CAR_LINE_ACTIVE_DARK               (1U)    /* 1: black line gives stronger normalized value. */
#define CAR_LINE_BLACK_IS_LOW_RAW          (1U)    /* Change to 0 if black produces a larger ADC value. */
#define CAR_LINE_DETECT_SUM_MIN            (600U)
#define CAR_LINE_ELEMENT_THRESHOLD         (650U)
#define CAR_LINE_LOST_STOP_MS              (500U)

/* 方向环生成速度差；差速混合之后再由每个车轮的速度环跟踪目标。 */
#define CAR_BASE_SPEED_MM_S                (250.0f)
#define CAR_MIN_CURVE_SPEED_MM_S           (100.0f)
#define CAR_MAX_WHEEL_SPEED_MM_S           (600.0f)
#define CAR_CURVE_SLOWDOWN_GAIN            (120.0f)

/* 循迹首版默认使用 PD；所有参数都只是低速联调起点。 */
#define CAR_LINE_KP                        (260.0f)
#define CAR_LINE_KI                        (0.0f)
#define CAR_LINE_KD                        (6.0f)
#define CAR_LINE_STEERING_LIMIT_MM_S       (300.0f)

/* 左右轮速度闭环参数，建议先架空车轮分别整定。 */
#define CAR_SPEED_LEFT_KP                  (1.0f)
#define CAR_SPEED_LEFT_KI                  (0.0f)
#define CAR_SPEED_LEFT_KD                  (0.0f)
#define CAR_SPEED_RIGHT_KP                 (1.0f)
#define CAR_SPEED_RIGHT_KI                 (0.0f)
#define CAR_SPEED_RIGHT_KD                 (0.0f)

/* 人机和遥测任务低频运行，避免占用 200 Hz 控制环时间。 */
#define CAR_DEBUG_PERIOD_MS                (20U)
#define CAR_OLED_PERIOD_MS                 (100U)

#endif /* CAR_CONFIG_H */
