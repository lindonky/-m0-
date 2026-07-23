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

/*
 * 八路灰度模块使用 3 位地址 + 1 位数字输出，并不直接输出八路模拟 ADC：
 *   AD0=A/最低位，AD1=B，AD2=C/最高位，OUT=当前所选通道的比较器状态。
 *
 * 在 SysConfig 中添加 3 个普通推挽输出和 1 个数字输入，建议实例名分别为：
 * LINE_MUX_AD0、LINE_MUX_AD1、LINE_MUX_AD2、LINE_MUX_OUT。确认实际接线并成功
 * 生成以下 PORT/PIN 宏后，再把 READY 改成 1。模块接 5 V 供电，但连接 MSPM0
 * 前必须实测 OUT 高电平不超过 3.3 V；MSPM0 GPIO 不能承受 5 V 输入。
 */
#ifndef CAR_LINE_MUX_READY
#define CAR_LINE_MUX_READY                  (0U)
#endif

#if CAR_LINE_MUX_READY
#ifndef CAR_LINE_MUX_AD0_PORT
#define CAR_LINE_MUX_AD0_PORT               (LINE_MUX_AD0_PORT)
#endif
#ifndef CAR_LINE_MUX_AD0_PIN
#define CAR_LINE_MUX_AD0_PIN                (LINE_MUX_AD0_PIN)
#endif
#ifndef CAR_LINE_MUX_AD1_PORT
#define CAR_LINE_MUX_AD1_PORT               (LINE_MUX_AD1_PORT)
#endif
#ifndef CAR_LINE_MUX_AD1_PIN
#define CAR_LINE_MUX_AD1_PIN                (LINE_MUX_AD1_PIN)
#endif
#ifndef CAR_LINE_MUX_AD2_PORT
#define CAR_LINE_MUX_AD2_PORT               (LINE_MUX_AD2_PORT)
#endif
#ifndef CAR_LINE_MUX_AD2_PIN
#define CAR_LINE_MUX_AD2_PIN                (LINE_MUX_AD2_PIN)
#endif
#ifndef CAR_LINE_MUX_OUT_PORT
#define CAR_LINE_MUX_OUT_PORT               (LINE_MUX_OUT_PORT)
#endif
#ifndef CAR_LINE_MUX_OUT_PIN
#define CAR_LINE_MUX_OUT_PIN                (LINE_MUX_OUT_PIN)
#endif
#endif

/* 预留给其他传感器的硬件 I2C；当前这只 500 Hz IMU 实际使用独立 UART。 */
#define CAR_I2C_INSTANCE_INDEX               CAR_PIN_UNASSIGNED
#define CAR_I2C_SCL_GPIO_PORT                CAR_PIN_UNASSIGNED
#define CAR_I2C_SCL_GPIO_PIN                 CAR_PIN_UNASSIGNED
#define CAR_I2C_SDA_GPIO_PORT                CAR_PIN_UNASSIGNED
#define CAR_I2C_SDA_GPIO_PIN                 CAR_PIN_UNASSIGNED
#define CAR_I2C_BITRATE_HZ                   (400000UL)

/*
 * 500 Hz 串口 IMU：115200、8 数据位、无校验、1 停止位（8N1）。
 * 当前 SysConfig 实例名为 IMU_UART，使用 UART0、PA10 TX、PA11 RX，并启用
 * FIFO 和 RX 中断。PA10/PA11 涉及 LaunchPad J21/J22 路由，上板前必须检查。
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


/*
 * HC-05 蓝牙串口透传（调试、在线调参和手机命令）。
 *
 * HC-05 在“数据透传模式”下不需要专用通信协议：MCU 从 UART 发出的每个字节都会
 * 通过蓝牙发送给手机，手机发来的字节也会原样出现在 UART RX。因此本工程只需要
 * 一套可靠的全双工 UART 环形缓冲驱动，不需要给 HC-05 编写寄存器驱动。
 *
 * 在 SysConfig 中新增第二个 UART，并把实例名设为 HC05_UART：
 *   - 必须与 IMU_UART 使用不同外设；当前 IMU 已占用 UART0；
 *   - 8 数据位、无校验、1 停止位（8N1），无硬件流控；
 *   - TX 和 RX 都启用，开启 FIFO，RX FIFO 阈值选择 >= 1 entry；
 *   - 打开 RX interrupt；TX interrupt 会由 BSP 在有待发数据时动态开关；
 *   - SysConfig 波特率必须与 HC-05 数据模式波特率一致。
 *
 * HC-05 常见出厂数据模式波特率是 9600，而本车 20 ms 一次的 CSV 遥测推荐
 * 115200。若模块仍是 9600，请先在 AT 模式修改模块波特率，或者同时把 SysConfig
 * 和下面的记录值改为 9600；只改本宏不会改变 UART 寄存器，实际硬件以 SysConfig
 * 为准。当前 empty.syscfg 已配置 UART1、PB4 TX、PB5 RX。PB4 在板卡资料中也标为 PWM
 * 候选，因此后续分配 TB6612 PWM 时必须避开 PB4；PB5 位于下方未焊接接口区域，
 * 请确认实物已经引出并保证 HC-05 与 MSPM0 共地。
 */
#ifndef CAR_HC05_UART_READY
#define CAR_HC05_UART_READY                 (1U)
#endif
#define CAR_HC05_UART_BAUD_RATE             (115200UL)

#if CAR_HC05_UART_READY
/* 若 SysConfig 实例不是 HC05_UART，只需要修改下面三个别名。 */
#ifndef CAR_HC05_UART_INST
#define CAR_HC05_UART_INST                  (HC05_UART_INST)
#endif
#ifndef CAR_HC05_UART_IRQN
#define CAR_HC05_UART_IRQN                  (HC05_UART_INST_INT_IRQN)
#endif
#ifndef CAR_HC05_UART_IRQ_HANDLER
#define CAR_HC05_UART_IRQ_HANDLER           HC05_UART_INST_IRQHandler
#endif
#endif

/* 可选启停按键与电池电压 ADC；相关驱动尚未实现。 */
#define CAR_KEY_GPIO_PORT                    CAR_PIN_UNASSIGNED
#define CAR_KEY_GPIO_PIN                     CAR_PIN_UNASSIGNED
#define CAR_BATTERY_ADC_CHANNEL              CAR_PIN_UNASSIGNED

#endif /* BOARD_CONFIG_H */
