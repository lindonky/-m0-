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
#define CAR_ENCODER_LEFT_POLARITY          (-1)
#define CAR_ENCODER_RIGHT_POLARITY         (+1)

/* 八路真实模拟循迹阵列；公开数组始终按车辆物理最左侧到最右侧排列。 */
#define CAR_LINE_SENSOR_COUNT              (8U)
#define CAR_LINE_ACTIVE_DARK               (1U)    /* 1: black line gives stronger normalized value. */
#define CAR_LINE_BLACK_IS_LOW_RAW          (1U)    /* Change to 0 if black produces a larger ADC value. */
#define CAR_LINE_ADC0_CHANNEL_COUNT        (3U)    /* ADC0 MEM0~2 -> sensor 0~2. */
#define CAR_LINE_ADC1_CHANNEL_COUNT        (5U)    /* ADC1 MEM0~4 -> sensor 3~7. */
#define CAR_LINE_ADC_MAX_VALUE             (4095U)
#define CAR_LINE_ADC_TIMEOUT_POLLS         (5U)    /* LineSensor_Sample() is called every 1 ms. */
#define CAR_LINE_ADC_REVERSE_ORDER         (0U)    /* Set to 1 only if all eight wires are reversed. */
#define CAR_LINE_DETECT_SUM_MIN            (600U)
#define CAR_LINE_ELEMENT_THRESHOLD         (650U)
#define CAR_LINE_LOST_STOP_MS              (500U)

/* 方向环生成速度差；差速混合之后再由每个车轮的速度环跟踪目标。 */
#define CAR_BASE_SPEED_MM_S                (250.0f)
#define CAR_MIN_CURVE_SPEED_MM_S           (100.0f)
#define CAR_MAX_WHEEL_SPEED_MM_S           (600.0f)
#define CAR_CURVE_SLOWDOWN_GAIN            (120.0f)

/* 循迹首版默认使用 PD；所有参数都只是低速联调起点。 */
#define CAR_LINE_STEERING_POLARITY         (-1.0f)
#define CAR_LINE_KP                        (260.0f)
#define CAR_LINE_KI                        (0.0f)
#define CAR_LINE_KD                        (6.0f)
#define CAR_LINE_STEERING_LIMIT_MM_S       (300.0f)

/*
 * 当前控制模式。
 *
 * ANGLE_DEBUG 用于本轮陀螺仪角度闭环联调：暂时忽略灰度位置和丢线停车，只允许
 * 车辆原地旋转。调试完成后把 CAR_CONTROL_MODE 改回 CAR_CONTROL_MODE_LINE，
 * 原有循迹代码和参数仍完整保留，不需要重新移植。
 */
#define CAR_CONTROL_MODE_LINE              (0U)
#define CAR_CONTROL_MODE_ANGLE_DEBUG       (1U)
#define CAR_CONTROL_MODE                   CAR_CONTROL_MODE_ANGLE_DEBUG

/*
 * 相对角度闭环：进入/重新启动时当前车头为 0 deg，正值右转、负值左转。
 * 目标限制为 -180~+180 deg，误差按最短方向折算到同一范围。PID 输出单位为
 * 原地旋转时的单轮目标速度 mm/s，再交给现有左右轮速度 PID 跟踪。
 */
#define CAR_ANGLE_TARGET_MIN_DEG           (-180.0f)
#define CAR_ANGLE_TARGET_MAX_DEG           (+180.0f)
#define CAR_ANGLE_KP                       (2.50f)
#define CAR_ANGLE_KI                       (0.0f)
#define CAR_ANGLE_KD                       (0.35f)
#define CAR_ANGLE_STEERING_LIMIT_MM_S      (180.0f)
#define CAR_ANGLE_WHEEL_SPEED_LIMIT_MM_S   (220.0f)
#define CAR_ANGLE_TOLERANCE_DEG            (2.0f)
#define CAR_ANGLE_RATE_TOLERANCE_DPS       (5.0f)
#define CAR_ANGLE_SETTLE_TIME_MS           (200U)

/*
 * IMU 转向角速度内环。
 *
 * 普通循迹不能永久锁定一个“世界坐标绝对角度”，否则进入弯道后角度环会强迫车辆
 * 回到起跑方向，与灰度循迹互相打架。因此这里使用级联结构：
 *
 *   灰度位置环输出基础转向速度差
 *       -> 按比例换算为目标偏航角速度
 *       -> IMU Z 轴角速度 P/PI 环生成附加转向修正
 *       -> 与基础转向叠加后送入左右轮差速混合
 *
 * 这既保留了当前已经实车验证的灰度循迹，又能用陀螺仪抑制甩尾、外力扰动和
 * 左右轮动态差异。累计 yawDegrees 留给直角、环岛、定角转向和短时丢线保持等
 * 明确知道目标角度的状态使用，不在普通连续循迹中强行锁定。
 *
 * 重要方向约定：经过 CAR_IMU_YAW_POLARITY 处理后，车辆实际向右转时
 * IMU gyroZDps 必须为正。若架空车轮手动向右旋转车身时读数为负，只修改
 * CAR_IMU_YAW_POLARITY，不要修改电机、编码器或循迹传感器极性。
 */
#define CAR_YAW_RATE_CONTROL_ENABLE        (1U)
/* 基础转向 1 mm/s 对应多少 deg/s 目标角速度；首版只作低速保守起点。 */
#define CAR_YAW_RATE_TARGET_GAIN           (0.40f)
#define CAR_YAW_RATE_TARGET_LIMIT_DPS      (120.0f)
/* PID 输出单位为附加转向速度差 mm/s；Ki 首版为 0，先调稳 Kp 再考虑积分。 */
#define CAR_YAW_RATE_KP                    (0.50f)
#define CAR_YAW_RATE_KI                    (0.0f)
#define CAR_YAW_RATE_KD                    (0.0f)
#define CAR_YAW_RATE_CORRECTION_LIMIT_MM_S (80.0f)
/* IMU 从无效变为有效后，用该时间把修正从 0 平滑增加到 100%。 */
#define CAR_YAW_RATE_ENGAGE_TIME_S         (0.10f)

/* 左右轮速度闭环参数，建议先架空车轮分别整定。 */
#define CAR_SPEED_LEFT_KP                  (2.0f)
#define CAR_SPEED_LEFT_KI                  (0.0f)
#define CAR_SPEED_LEFT_KD                  (0.02f)
#define CAR_SPEED_RIGHT_KP                 (2.0f)
#define CAR_SPEED_RIGHT_KI                 (0.0f)
#define CAR_SPEED_RIGHT_KD                 (0.02f)

/*
 * 串口 IMU 参数。
 * 参考驱动中角度和角速度原始值都乘 0.1；帧频由用户确认为 500 Hz。
 * 上电配置延迟沿用参考程序的 3000 ms，但采用分时状态机，绝不阻塞主循环。
 */
#define CAR_IMU_REPORT_RATE_HZ             (500U)
#define CAR_IMU_RAW_TO_DEG                 (0.1f)
/* 必须调成“车身实际右转时 yawDegrees 和 gyroZDps 都增加”。 */
#define CAR_IMU_YAW_POLARITY               (+1.0f)
#define CAR_IMU_DATA_TIMEOUT_MS            (20U)
#define CAR_IMU_STARTUP_CONFIG_DELAY_MS    (3000U)
#define CAR_IMU_CALIBRATION_SAMPLES        (500U)
#define CAR_IMU_MAX_BYTES_PER_UPDATE       (32U)

/*
 * HC-05 调试协议。
 *
 * PID_DEBUG 使用江协蓝牙小程序的 `[display]`、`[plot]`、`[key]` 和 `[slider]`
 * 文本包。旧 CSV 默认关闭，避免无方括号的 CSV 数据污染小程序界面；需要电脑记录
 * 原始 CSV 时可重新打开，也可以同时开启两种发送格式。
 */
#define CAR_PID_DEBUG_ENABLE               (1U)
#define CAR_DEBUG_CSV_ENABLE               (0U)
#define CAR_PID_DEBUG_PLOT_DEFAULT         (1U)
#define CAR_PID_DEBUG_PLOT_PERIOD_MS       (100U)
#define CAR_PID_DEBUG_DISPLAY_PERIOD_MS    (500U)
#define CAR_PID_DEBUG_PACKET_TIMEOUT_MS    (250U)
#define CAR_PID_DEBUG_GAIN_MAX             (10000.0f)

/* 人机和遥测任务低频运行，避免占用 200 Hz 控制环时间。 */
#define CAR_DEBUG_PERIOD_MS                (20U)
#define CAR_OLED_PERIOD_MS                 (40U)

#endif /* CAR_CONFIG_H */
