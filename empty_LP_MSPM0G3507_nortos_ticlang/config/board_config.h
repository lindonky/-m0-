#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/*
 * @file board_config.h
 * @brief 硬件接线记录表；这里的数值目前不直接驱动寄存器。
 *
 * 这些宏用于集中记录接线和外设分配，避免接线信息散落在业务代码中。当前统一
 * 设为 CAR_PIN_UNASSIGNED。完成 empty.syscfg 后，应把实际资源记录在此文件，并
 * 在相应 bsp_*.c 中使用 ti_msp_dl_config.h 生成的符号完成 DriverLib 调用。
 *
 * 本记录表暂定端口编号：0=GPIOA，1=GPIOB。编号只用于文档，不应直接强转成
 * GPIO_Regs 指针。
 */
#define CAR_PIN_UNASSIGNED                  (0xFFFFFFFFUL)

/* TB6612：A 通道驱动左轮，B 通道驱动右轮；若实物相反，应同时修改记录和 BSP。 */
#define CAR_TB6612_PWMA_TIMER_INDEX         CAR_PIN_UNASSIGNED
#define CAR_TB6612_PWMA_CC_INDEX            CAR_PIN_UNASSIGNED
#define CAR_TB6612_AIN1_GPIO_PORT           CAR_PIN_UNASSIGNED
#define CAR_TB6612_AIN1_GPIO_PIN            CAR_PIN_UNASSIGNED
#define CAR_TB6612_AIN2_GPIO_PORT           CAR_PIN_UNASSIGNED
#define CAR_TB6612_AIN2_GPIO_PIN            CAR_PIN_UNASSIGNED

#define CAR_TB6612_PWMB_TIMER_INDEX         CAR_PIN_UNASSIGNED
#define CAR_TB6612_PWMB_CC_INDEX            CAR_PIN_UNASSIGNED
#define CAR_TB6612_BIN1_GPIO_PORT           CAR_PIN_UNASSIGNED
#define CAR_TB6612_BIN1_GPIO_PIN            CAR_PIN_UNASSIGNED
#define CAR_TB6612_BIN2_GPIO_PORT           CAR_PIN_UNASSIGNED
#define CAR_TB6612_BIN2_GPIO_PIN            CAR_PIN_UNASSIGNED

#define CAR_TB6612_STBY_GPIO_PORT           CAR_PIN_UNASSIGNED
#define CAR_TB6612_STBY_GPIO_PIN            CAR_PIN_UNASSIGNED
#define CAR_TB6612_PWM_FREQUENCY_HZ         (20000UL)

/* 正交编码器 A/B 相；优先为每个车轮分配一个硬件 QEI 定时器。 */
#define CAR_ENCODER_LEFT_TIMER_INDEX        CAR_PIN_UNASSIGNED
#define CAR_ENCODER_LEFT_A_GPIO_PORT        CAR_PIN_UNASSIGNED
#define CAR_ENCODER_LEFT_A_GPIO_PIN         CAR_PIN_UNASSIGNED
#define CAR_ENCODER_LEFT_B_GPIO_PORT        CAR_PIN_UNASSIGNED
#define CAR_ENCODER_LEFT_B_GPIO_PIN         CAR_PIN_UNASSIGNED
#define CAR_ENCODER_RIGHT_TIMER_INDEX       CAR_PIN_UNASSIGNED
#define CAR_ENCODER_RIGHT_A_GPIO_PORT       CAR_PIN_UNASSIGNED
#define CAR_ENCODER_RIGHT_A_GPIO_PIN        CAR_PIN_UNASSIGNED
#define CAR_ENCODER_RIGHT_B_GPIO_PORT       CAR_PIN_UNASSIGNED
#define CAR_ENCODER_RIGHT_B_GPIO_PIN        CAR_PIN_UNASSIGNED

/* 模拟循迹 ADC 通道，数组顺序必须是物理最左到最右。 */
#define CAR_LINE_ADC_CH_0                    CAR_PIN_UNASSIGNED
#define CAR_LINE_ADC_CH_1                    CAR_PIN_UNASSIGNED
#define CAR_LINE_ADC_CH_2                    CAR_PIN_UNASSIGNED
#define CAR_LINE_ADC_CH_3                    CAR_PIN_UNASSIGNED
#define CAR_LINE_ADC_CH_4                    CAR_PIN_UNASSIGNED
#define CAR_LINE_ADC_CH_5                    CAR_PIN_UNASSIGNED
#define CAR_LINE_ADC_CH_6                    CAR_PIN_UNASSIGNED
#define CAR_LINE_ADC_CH_7                    CAR_PIN_UNASSIGNED

/* 预留给其他传感器的硬件 I2C；当前这只 500 Hz IMU 实际使用独立 UART。 */
#define CAR_I2C_INSTANCE_INDEX               CAR_PIN_UNASSIGNED
#define CAR_I2C_SCL_GPIO_PORT                CAR_PIN_UNASSIGNED
#define CAR_I2C_SCL_GPIO_PIN                 CAR_PIN_UNASSIGNED
#define CAR_I2C_SDA_GPIO_PORT                CAR_PIN_UNASSIGNED
#define CAR_I2C_SDA_GPIO_PIN                 CAR_PIN_UNASSIGNED
#define CAR_I2C_BITRATE_HZ                   (400000UL)

/*
 * 500 Hz 串口 IMU：115200、8 数据位、无校验、1 停止位（8N1）。
 * 在 SysConfig 中建议把 UART 实例命名为 IMU_UART，并启用 RX 中断。参考例程
 * 使用 UART3/PB2(TX)/PB3(RX)，但这里不抢占引脚；确认整车接线后再把 READY 改为 1。
 * IMU 与调试终端必须使用不同 UART，防止二进制帧和 CSV/命令互相污染。
 */
#ifndef CAR_IMU_UART_READY
#define CAR_IMU_UART_READY                  (1U)
#endif
#define CAR_IMU_UART_BAUD_RATE              (115200UL)

#if CAR_IMU_UART_READY
/* TODO：若 SysConfig 生成名不同，只修改以下三个别名和中断入口别名。 */
#ifndef CAR_IMU_UART_INST
#define CAR_IMU_UART_INST                   (IMU_UART_INST)
#endif
#ifndef CAR_IMU_UART_IRQN
#define CAR_IMU_UART_IRQN                   (IMU_UART_INST_INT_IRQN)
#endif
#ifndef CAR_IMU_UART_IRQ_HANDLER
#define CAR_IMU_UART_IRQ_HANDLER            IMU_UART_INST_IRQHandler
#endif
#endif

/*
 * OLED 使用软件 I2C，不占用上面的硬件 I2C 控制器。
 * 在 SysConfig 中把两个引脚配置为普通数字 GPIO，并确保总线上有上拉电阻。
 * 建议把 SysConfig 引脚实例命名为 OLED_SCL 和 OLED_SDA，随后把 READY 改为 1。
 * BSP 通过“输出低/切换为输入高阻”模拟开漏，不会主动推挽输出高电平。
 */
#ifndef CAR_OLED_SOFT_I2C_READY
#define CAR_OLED_SOFT_I2C_READY              (0U)
#endif
#ifndef CAR_OLED_I2C_ADDRESS_7BIT
#define CAR_OLED_I2C_ADDRESS_7BIT            (0x3CU)
#endif
#ifndef CAR_OLED_SOFT_I2C_DELAY_CYCLES
#define CAR_OLED_SOFT_I2C_DELAY_CYCLES       (40U)
#endif

#if CAR_OLED_SOFT_I2C_READY
/* TODO：若你的 SysConfig 生成名不同，只修改以下四个别名。 */
#ifndef CAR_OLED_SCL_PORT
#define CAR_OLED_SCL_PORT                    (OLED_SCL_PORT)
#endif
#ifndef CAR_OLED_SCL_PIN
#define CAR_OLED_SCL_PIN                     (OLED_SCL_PIN)
#endif
#ifndef CAR_OLED_SDA_PORT
#define CAR_OLED_SDA_PORT                    (OLED_SDA_PORT)
#endif
#ifndef CAR_OLED_SDA_PIN
#define CAR_OLED_SDA_PIN                     (OLED_SDA_PIN)
#endif
#endif


/* 调试和后续在线调参 UART；不能与上面的 500 Hz IMU UART 共用。 */
#define CAR_UART_INSTANCE_INDEX              CAR_PIN_UNASSIGNED
#define CAR_UART_TX_GPIO_PORT                CAR_PIN_UNASSIGNED
#define CAR_UART_TX_GPIO_PIN                 CAR_PIN_UNASSIGNED
#define CAR_UART_RX_GPIO_PORT                CAR_PIN_UNASSIGNED
#define CAR_UART_RX_GPIO_PIN                 CAR_PIN_UNASSIGNED
#define CAR_UART_BAUD_RATE                   (115200UL)

/* 可选启停按键与电池电压 ADC；相关驱动尚未实现。 */
#define CAR_KEY_GPIO_PORT                    CAR_PIN_UNASSIGNED
#define CAR_KEY_GPIO_PIN                     CAR_PIN_UNASSIGNED
#define CAR_BATTERY_ADC_CHANNEL              CAR_PIN_UNASSIGNED

#endif /* BOARD_CONFIG_H */
