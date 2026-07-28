#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/*
 * @file board_config.h
 * @brief SysConfig 生成符号到整车代码的集中硬件映射。
 *
 * 业务层不直接使用 SysConfig 实例名，所有硬件名字先在这里变成 CAR_* 别名。
 * 将来换引脚或重命名实例时，优先只改 empty.syscfg 和本文件，避免改动控制算法。
 * 本文件引用的实例、端口和引脚符号均由 ti_msp_dl_config.h 自动生成。
 */
#define CAR_PIN_UNASSIGNED                  (0xFFFFFFFFUL)

/*
 * TB6612FNG：A 通道驱动左轮，B 通道驱动右轮。
 * TIMG12 在当前 80 MHz 时钟树下直接使用 80 MHz BUSCLK；LOAD=4000，因此
 * PWM=80 MHz/4000=20 kHz。向下计数 PWM 的 0% 对应 CC=LOAD，100% 对应 CC=0。
 */
#ifndef CAR_TB6612_READY
#define CAR_TB6612_READY                    (1U)
#endif

#if CAR_TB6612_READY
#define CAR_TB6612_PWM_INST                 (MOTOR_PWM_INST)
#define CAR_TB6612_PWM_A_CC_INDEX           (GPIO_MOTOR_PWM_C0_IDX)
#define CAR_TB6612_PWM_B_CC_INDEX           (GPIO_MOTOR_PWM_C1_IDX)
#define CAR_TB6612_PWM_PERIOD_COUNTS        (4000U)
#define CAR_TB6612_PWM_FREQUENCY_HZ         (20000UL)

#define CAR_TB6612_AIN1_GPIO_PORT           (AIN1_PORT)
#define CAR_TB6612_AIN1_GPIO_PIN            (AIN1_PIN_0_PIN)
#define CAR_TB6612_AIN2_GPIO_PORT           (AIN2_PORT)
#define CAR_TB6612_AIN2_GPIO_PIN            (AIN2_PIN_1_PIN)
#define CAR_TB6612_BIN1_GPIO_PORT           (BIN1_PORT)
#define CAR_TB6612_BIN1_GPIO_PIN            (BIN1_PIN_2_PIN)
#define CAR_TB6612_BIN2_GPIO_PORT           (BIN2_PORT)
#define CAR_TB6612_BIN2_GPIO_PIN            (BIN2_PIN_3_PIN)
#define CAR_TB6612_STBY_GPIO_PORT           (STBY_PORT)
#define CAR_TB6612_STBY_GPIO_PIN            (STBY_PIN_4_PIN)
#endif

/*
 * 编码器：左轮 PB6/PB7 由 TIMG8 的 2-input QEI 硬件计数；右轮 PB8/PB9
 * 由 GPIO 双边沿中断软件 AB 解码。MSPM0G3507 的 GPIOB 向量属于共享 GROUP1，
 * 因此 ISR 名必须是 GROUP1_IRQHandler，不能写成 GPIOB_IRQHandler。
 */
#ifndef CAR_ENCODER_READY
#define CAR_ENCODER_READY                   (1U)
#endif

#if CAR_ENCODER_READY
#define CAR_ENCODER_LEFT_QEI_INST           (LEFT_ENCODER_QEI_INST)

#define CAR_ENCODER_RIGHT_GPIO_PORT         (RIGHT_ENCODER_GPIO_PORT)
#define CAR_ENCODER_RIGHT_A_GPIO_PIN        (RIGHT_ENCODER_GPIO_A_PIN)
#define CAR_ENCODER_RIGHT_B_GPIO_PIN        (RIGHT_ENCODER_GPIO_B_PIN)
#define CAR_ENCODER_RIGHT_IRQN              (RIGHT_ENCODER_GPIO_INT_IRQN)
#define CAR_ENCODER_RIGHT_GROUP_IIDX        (RIGHT_ENCODER_GPIO_INT_IIDX)
#define CAR_ENCODER_GROUP_IRQ_HANDLER       GROUP1_IRQHandler
#endif

/*
 * 八路模拟循迹阵列：使用 ADC0 和 ADC1 的两个非重复序列采集真实 12 位模拟量。
 * 推荐实例名为 LINE_ADC0 和 LINE_ADC1；默认分组为 ADC0/MEM0~2 三路、
 * ADC1/MEM0~4 五路。两个序列都由软件触发，最后一个 MEM 完成时产生中断。
 *
 * 当前 empty.syscfg 已正式加入这两个 ADC 实例，并已核对 3+5 序列、引脚和末尾
 * 完成中断；READY 因此正式启用。若以后删除或重命名实例，应先恢复为 0。
 * 不要手改 SysConfig 自动生成文件；若实例名不同，只修改下面的别名。
 */
#ifndef CAR_LINE_ADC_READY
#define CAR_LINE_ADC_READY                  (1U)
#endif

#if CAR_LINE_ADC_READY
#ifndef CAR_LINE_ADC0_INST
#define CAR_LINE_ADC0_INST                  (LINE_ADC0_INST)
#endif
#ifndef CAR_LINE_ADC0_IRQN
#define CAR_LINE_ADC0_IRQN                  (LINE_ADC0_INST_INT_IRQN)
#endif
#ifndef CAR_LINE_ADC0_IRQ_HANDLER
#define CAR_LINE_ADC0_IRQ_HANDLER           LINE_ADC0_INST_IRQHandler
#endif

#ifndef CAR_LINE_ADC1_INST
#define CAR_LINE_ADC1_INST                  (LINE_ADC1_INST)
#endif
#ifndef CAR_LINE_ADC1_IRQN
#define CAR_LINE_ADC1_IRQN                  (LINE_ADC1_INST_INT_IRQN)
#endif
#ifndef CAR_LINE_ADC1_IRQ_HANDLER
#define CAR_LINE_ADC1_IRQ_HANDLER           LINE_ADC1_INST_IRQHandler
#endif

/* 默认序列长度为 3+5；若改变分组，必须同步修改完成中断 IIDX 和 MASK。 */
#ifndef CAR_LINE_ADC0_DONE_IIDX
#define CAR_LINE_ADC0_DONE_IIDX             DL_ADC12_IIDX_MEM2_RESULT_LOADED
#endif
#ifndef CAR_LINE_ADC0_DONE_INTERRUPT
#define CAR_LINE_ADC0_DONE_INTERRUPT        DL_ADC12_INTERRUPT_MEM2_RESULT_LOADED
#endif
#ifndef CAR_LINE_ADC1_DONE_IIDX
#define CAR_LINE_ADC1_DONE_IIDX             DL_ADC12_IIDX_MEM4_RESULT_LOADED
#endif
#ifndef CAR_LINE_ADC1_DONE_INTERRUPT
#define CAR_LINE_ADC1_DONE_INTERRUPT        DL_ADC12_INTERRUPT_MEM4_RESULT_LOADED
#endif
#endif

/*
 * NoRTOS 1 ms 单调时基：TICK_TIMER 使用 TIMG0，BUSCLK/1、周期 1 ms、ZERO 中断。
 * SysConfig 只完成静态配置并保持 Counter 停止；BSP_Time_Init() 清零软件计数、
 * 清除旧中断、使能 NVIC 后再启动 Counter，避免初始化期间出现不确定的首个 tick。
 */
#ifndef CAR_TIMEBASE_READY
#define CAR_TIMEBASE_READY                  (1U)
#endif

#if CAR_TIMEBASE_READY
#ifndef CAR_TIMEBASE_INST
#define CAR_TIMEBASE_INST                   (TICK_TIMER_INST)
#endif
#ifndef CAR_TIMEBASE_IRQN
#define CAR_TIMEBASE_IRQN                   (TICK_TIMER_INST_INT_IRQN)
#endif
#ifndef CAR_TIMEBASE_IRQ_HANDLER
#define CAR_TIMEBASE_IRQ_HANDLER            TICK_TIMER_INST_IRQHandler
#endif
#endif

/*
 * 为后续传感器预留硬件 I2C1：PB2=SCL、PB3=SDA。
 * 这里仅记录已经通过 PinMux 验证的推荐资源，尚未要求 empty.syscfg 创建实例；
 * 因此业务代码不能仅凭这些数值访问外设。SCL/SDA 必须上拉到 3.3 V。
 */
#define CAR_I2C_INSTANCE_INDEX               (1U)
#define CAR_I2C_SCL_GPIO_PORT                (1U)  /* 1 = GPIOB. */
#define CAR_I2C_SCL_GPIO_PIN                 (2U)  /* PB2. */
#define CAR_I2C_SDA_GPIO_PORT                (1U)  /* 1 = GPIOB. */
#define CAR_I2C_SDA_GPIO_PIN                 (3U)  /* PB3. */
#define CAR_I2C_BITRATE_HZ                   (400000UL)

/*
 * 为后续串口模块预留 UART2：PB15=TX、PB16=RX。
 * UART0 已给 IMU，UART1 已给 HC-05；本记录不表示 UART2 已在 SysConfig 中启用。
 */
#define CAR_SPARE_UART_INSTANCE_INDEX        (2U)
#define CAR_SPARE_UART_TX_GPIO_PORT          (1U)  /* 1 = GPIOB. */
#define CAR_SPARE_UART_TX_GPIO_PIN            (15U) /* PB15. */
#define CAR_SPARE_UART_RX_GPIO_PORT          (1U)  /* 1 = GPIOB. */
#define CAR_SPARE_UART_RX_GPIO_PIN            (16U) /* PB16. */

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
/* 若以后修改 SysConfig 实例名，只需要同步下面三个别名和中断入口别名。 */
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
 * OLED 使用软件 I2C，不占用上面为后续传感器预留的硬件 I2C1。
 * 正式 SysConfig 已建立 OLED_GPIO 组：PA12=SCL、PA13=SDA；两脚均配置为
 * Output、Initial Set、No Resistor、Low Drive、Hi-Z Enable、无中断。
 *
 * GPIO 输出锁存值为 1 时，MSPM0 的 Hi-Z 功能会释放线路；锁存值为 0 时主动
 * 下拉。BSP 另外使用输出使能控制确保逻辑 1 始终为高阻，不会主动推挽到高电平。
 * SCL/SDA 必须由 OLED 模块或外部电阻上拉到 3.3 V，禁止上拉到 5 V。
 */
#ifndef CAR_OLED_SOFT_I2C_READY
#define CAR_OLED_SOFT_I2C_READY              (1U)
#endif
#ifndef CAR_OLED_I2C_ADDRESS_7BIT
#define CAR_OLED_I2C_ADDRESS_7BIT            (0x3CU)
#endif
#ifndef CAR_OLED_SOFT_I2C_DELAY_CYCLES
/* 32 MHz 时实物验证值为 40 cycles；按 CPUCLK 等比例换算，80 MHz 时为 100。 */
#define CAR_OLED_SOFT_I2C_DELAY_CYCLES       (CPUCLK_FREQ / 800000UL)
#endif

#if CAR_OLED_SOFT_I2C_READY
/* OLED_GPIO 为 SysConfig 组名；SCL/SDA 为该组内的 Pin Name。 */
#ifndef CAR_OLED_SCL_PORT
#define CAR_OLED_SCL_PORT                    (OLED_GPIO_PORT)
#endif
#ifndef CAR_OLED_SCL_PIN
#define CAR_OLED_SCL_PIN                     (OLED_GPIO_SCL_PIN)
#endif
#ifndef CAR_OLED_SDA_PORT
#define CAR_OLED_SDA_PORT                    (OLED_GPIO_PORT)
#endif
#ifndef CAR_OLED_SDA_PIN
#define CAR_OLED_SDA_PIN                     (OLED_GPIO_SDA_PIN)
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
 * 用户给出的江协 HC-05 参考工程使用 9600；当前为优先完成实物联调，MSPM0 也使用
 * 9600。若以后已通过 AT 模式把模块改为 115200，必须同时修改 empty.syscfg 和下面
 * 的记录值；只改本宏不会改变 UART 寄存器，实际硬件以 SysConfig 为准。
 * 当前 empty.syscfg 已配置 UART1、PB4 TX、PB5 RX。PB4 在板卡资料中也标为 PWM
 * 候选，因此后续分配 TB6612 PWM 时必须避开 PB4；PB5 位于下方未焊接接口区域，
 * 请确认实物已经引出并保证 HC-05 与 MSPM0 共地。
 */
#ifndef CAR_HC05_UART_READY
#define CAR_HC05_UART_READY                 (1U)
#endif
#define CAR_HC05_UART_BAUD_RATE             (9600UL)

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
