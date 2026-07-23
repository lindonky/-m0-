# MSPM0G3507 循迹车硬件移植与上板清单

## 1. 这份文档解决什么问题

控制算法已经与具体引脚解耦，当前剩余工作主要是：

1. 在 `empty.syscfg` 中创建所需外设；
2. 把生成的资源宏写入各个 `bsp/*.c`；
3. 根据实际接线修改 `config/board_config.h`；
4. 根据实物修改 `config/car_config.h`；
5. 按本文顺序逐项上板验证。

不要编辑以下生成文件：

```text
Debug/ti_msp_dl_config.c
Debug/ti_msp_dl_config.h
Debug/device_linker.cmd
Debug/makefile
```

它们会在重新运行 SysConfig 或构建时被覆盖。

## 2. 需要先提供或确认的硬件信息

- TB6612 的 PWMA、AIN1、AIN2、PWMB、BIN1、BIN2、STBY 接线；
- 左右电机分别接 TB6612 A 还是 B；
- 左右编码器 A/B 相接线；
- 编码器参数已知为 13 PPR、1:30；仍需确认 QEI 是否四倍频及实测一圈计数；
- 轮径；
- 循迹模块 CH0~CH7 的物理左右排列；
- 循迹模块 OUT 的有效电平，以及 5 V 供电时 OUT 的实际高电平；
- 调试 UART TX/RX 接线；
- 500 Hz IMU UART TX/RX 接线和模块串口电平；协议驱动已经完成；
- OLED 的 SCL/SDA 引脚；驱动已按 128×64 SSD1306、地址 0x3C 移植；

## 3. TB6612 的 SysConfig 配置

### 3.1 PWM

创建两个边沿对齐 PWM 输出：

- PWMA：左电机；
- PWMB：右电机；
- 推荐频率：20 kHz；
- 初始占空比：0%；
- 输出极性应与 `bsp_tb6612.c` 的换算一致。

可以使用同一个定时器的两个 CC 通道，也可以使用两个定时器，但必须检查引脚复用
是否与 QEI、ADC、UART 冲突。

### 3.2 GPIO

创建五个推挽输出：

```text
AIN1 AIN2 BIN1 BIN2 STBY
```

上电初值全部为低。尤其是 STBY 必须默认低，防止 MCU 复位期间电机误动作。

### 3.3 填写 BSP

在 `bsp/bsp_tb6612.c` 完成：

- `BSP_TB6612_SetStandby()`；
- `BSP_TB6612_SetInputs()`；
- `BSP_TB6612_SetPwmPermille()`。

上层传入占空比范围是 `0~1000`，BSP 负责换算成计时器比较值。换算后必须再次
限幅，避免 1000 对应超过 LOAD 的比较值。

## 4. 1 ms 调度定时器

创建一个 1 ms 周期定时器并启用中断。中断中只做：

```c
void YOUR_TIMER_IRQHandler(void)
{
    /* 1. 读取并确认中断来源。 */
    /* 2. 清除周期中断标志。 */
    BSP_Time_Tick1msFromISR();
}
```

不要在该 ISR 中调用 `App_Scheduler_Run()`。控制和通信任务在主循环执行，避免 ISR
过长并保持中断响应可预测。

验收：UART 遥测的第一列应每毫秒递增，长时间运行不停止。

## 5. 左右 QEI 编码器

优先为每个轮子分配一个硬件 QEI 定时器：

- 左编码器 A/B；
- 右编码器 A/B；
- 确认最大计数频率小于定时器输入能力；
- 若计数器位宽不足，使用溢出 ISR 扩展为 32 位。

参考 SDK：

```text
examples/nortos/LP_MSPM0G3507/driverlib/timg_qei_mode
```

在 `bsp/bsp_encoder.c` 完成：

- 启动 QEI；
- 返回左右当前计数；
- 复位硬件计数和软件扩展计数。

验收方法：

1. 架空车辆；
2. 手动把左轮向车辆前进方向转一圈；
3. 记录计数变化；
4. 右轮重复；
5. 修改极性宏，使两侧向前都为正；
6. 把实测一圈计数写入 `CAR_ENCODER_COUNTS_PER_WHEEL_REV`。

## 6. 八路数字循迹模块

### 6.1 资料结论和接口含义

这块模块确实节省 MCU 引脚，但不是“用四根线读八路 ADC 幅值”。PDF 给出的接口为：

```text
AD0 = 地址最低位 A
AD1 = 地址中间位 B
AD2 = 地址最高位 C
OUT = 当前所选通道比较后的数字 0/1

CH0 = 000, CH1 = 001, ... , CH7 = 111
```

资料参考代码的结果数组也是 `uint8_t ir_results[8]`。因此每路只有检测/未检测两种
状态，不能得到反射强度的真实 12 位 ADC 数值。当前 BSP 为兼容已有循迹算法，把
数字结果映射为 0 或 4095；以后改成真正的模拟阵列时，上层接口可以保持不变。

### 6.2 SysConfig 和接线

在 SysConfig 新建四个普通 GPIO：

```text
LINE_MUX_AD0：Digital Output，初始低
LINE_MUX_AD1：Digital Output，初始低
LINE_MUX_AD2：Digital Output，初始低
LINE_MUX_OUT：Digital Input
```

接线：

```text
模块 AD0 -> LINE_MUX_AD0
模块 AD1 -> LINE_MUX_AD1
模块 AD2 -> LINE_MUX_AD2
模块 OUT -> LINE_MUX_OUT
模块 GND -> MSPM0 GND
模块 5V  -> 5V 电源
```

模块按资料用 5 V 供电。MSPM0 GPIO 不耐受 5 V，连接 OUT 前必须测量其高电平。
如果 OUT 接近 5 V，必须增加电平转换或合适分压；不能因为它是“数字输出”就直接
接入。还要确认模块能把 MSPM0 输出的 3.3 V 可靠识别为 AD0~AD2 高电平。

SysConfig 保存并成功生成四组 `*_PORT`/`*_PIN` 宏后，把 `board_config.h` 中：

```c
#define CAR_LINE_MUX_READY (0U)
```

改为：

```c
#define CAR_LINE_MUX_READY (1U)
```

如果自动生成名与上述实例名不同，只修改 `CAR_LINE_MUX_AD0_*`、`AD1_*`、`AD2_*`
和 `OUT_*` 八个别名，不要手改 `ti_msp_dl_config.h/.c`。

### 6.3 无阻塞扫描时序

资料示例在每次选通后调用 `delay_us(100)`；当前工程明确不添加这种阻塞等待。
调度器每 1 ms 调用一次 `BSP_LineADC_Read()`，其状态机按下面顺序工作：

```text
读取上一次已选通道 -> 保存 -> 选择下一通道 -> 立即返回
```

前七次返回 `false`，读完 CH7 后才把八路工作数组整体复制给上层并返回 `true`。
这样地址建立时间约 1 ms，完整帧周期 8 ms，帧率约 125 Hz，而且上层不会看到
半帧旧数据和半帧新数据。禁止在无时间间隔的紧循环中连续调用该函数。

### 6.4 参数和验收

默认参数：

```c
CAR_LINE_MUX_ACTIVE_LEVEL  = 1U  // OUT 高表示检测到目标线
CAR_LINE_MUX_REVERSE_ORDER = 0U  // CH0 作为数组最左侧
CAR_LINE_MUX_PSEUDO_ADC_MAX = 4095U
```

上板观察：

- `BSP_LineADC_IsReady()` 应为 `true`；
- `BSP_LineADC_GetFrameCount()` 每 8 ms 增加一次，约每秒 125；
- `LineSensor_GetData()->raw[0..7]` 只能是 0 或 4095；
- 目标线通道的 `normalized[]` 接近 1000，背景接近 0；
- 线从左向右移动时 `position` 应从负值变到正值。

如果黑白/有效状态完全相反，先把 `CAR_LINE_MUX_ACTIVE_LEVEL` 从 1 改为 0；如果位置
左右相反，把 `CAR_LINE_MUX_REVERSE_ORDER` 从 0 改为 1。不要为了翻转方向改质心
权重或 PID 符号。由于输入只有二值，位置和置信度分辨率会低于真实模拟 ADC 阵列，
循迹 PID 必须重新实车整定。

## 7. 调试 UART

调试链路当前通过 HC-05 做透明串口。`bsp_uart.c` 已实现 512 字节 TX、256 字节 RX
环形缓冲、RX/TX FIFO 中断和错误统计。用户已在 SysConfig 完成当前分配：

推荐配置：

```text
实例名：HC05_UART
外设：UART1
TX：PB4
RX：PB5
115200 baud, 8 data bits, no parity, 1 stop bit
TX + RX，无硬件流控
FIFO：开启
RX FIFO threshold：>= 1 entry
中断：RX；TX 中断由 BSP 按发送队列状态动态开关
```

`board_config.h` 当前已经启用：

```c
#define CAR_HC05_UART_READY 1U
```

现有非阻塞接口：

- `BSP_UART_TryWrite()`：只入队，队列满时返回 false；
- `BSP_UART_TryWriteString()`：发送不包含结尾 `\0` 的字符串；
- `BSP_UART_TryWriteByte()`：发送一个字节；
- `BSP_UART_TryReadByte()`：RX 队列有数据才返回 true；
- `BSP_UART_Read()`：批量取走当前已经收到的数据；
- 三组诊断计数可区分 RX 溢出、硬件错误和 TX 队列拒绝；
- 不允许等待一个完整字符串发送完毕。

上层命令：R 开始、S 停止、X 紧急停止、C/E 循迹标定、G 偏航角清零、
I 开始 IMU 静止零偏标定。

HC-05 数据模式波特率必须与 SysConfig 一致；常见模块出厂可能为 9600，而当前
20 ms CSV 遥测推荐 115200。PB4 已被 UART1 TX 占用，后续 TB6612 PWM 不得再选
PB4；PB5 位于板卡下方未焊接接口区域，需确认实物引出。如果自动生成文件里暂时找
不到 `HC05_UART_*` 宏，保存 SysConfig 并执行一次 Clean + Build 让生成器刷新，不能
手工补写生成文件。引脚和接线详见 `BOARD_PINOUT.md`。

## 8. 500 Hz 串口 IMU

当前 IMU 不是 I2C 芯片，而是自带解算和串口协议的模块。参考工程确认参数如下：

```text
UART：115200 baud，8N1，无流控
上报频率：500 Hz（每 2 ms 一帧）
每帧：9 bytes
帧格式：0A 03 04 AngleH AngleL DpsH DpsL CRCL CRCH
角度比例：0.1 degree/LSB
角速度比例：0.1 degree/second/LSB
CRC：Modbus CRC16，初值 FFFF，多项式 A001，低字节先传
配置命令：AA 06 01 01 01 AD 00
```

### 8.1 接线与 UART 分配

必须给 IMU 使用独立 UART，不能与调试 CSV 串口共用：

```text
IMU VCC -> 按模块额定电压连接（先核对模块说明）
IMU GND -> MSPM0 GND
IMU TX  -> MSPM0 的 IMU_UART_RX
IMU RX  -> MSPM0 的 IMU_UART_TX
```

本工程当前已经配置 UART0、PA10 作为 MCU TX、PA11 作为 MCU RX；PA10/PA11 在
LaunchPad 上涉及 J21/J22 到 XDS/BoosterPack 的路由，上板前必须检查跳线。还需要
确认模块串口电平是 3.3 V TTL；若模块 TX 是 5 V，不得直接接 MSPM0 RX。

### 8.2 SysConfig

在 `empty.syscfg` 新增一个 UART，建议实例名为 `IMU_UART`：

```text
波特率：115200
数据位：8
校验：None
停止位：1
方向：TX + RX
中断：RX（BSP 运行时还会按需启停 TX 中断）
```

然后在 `board_config.h` 修改：

```c
#define CAR_IMU_UART_READY 1U
```

默认别名期望 SysConfig 生成：

```c
IMU_UART_INST
IMU_UART_INST_INT_IRQN
IMU_UART_INST_IRQHandler
```

若名字不同，只修改 `CAR_IMU_UART_INST`、`CAR_IMU_UART_IRQN` 和
`CAR_IMU_UART_IRQ_HANDLER`。BSP 会通过最后一个宏生成真正的中断入口；如果你已
在其他文件定义同名 IRQHandler，必须只保留一个，并从已有入口调用
`BSP_IMU_UART_IRQHandler()`。

### 8.3 软件数据流和实时性

```text
UART RX ISR
  -> 64 字节 RX 环形缓冲
  -> 1 ms App_Car_SensorTask1ms
  -> IMU_Update(0.001f)
  -> 帧头同步 / CRC / 大端有符号数
  -> 角度回绕 / 零偏 / 超时
  -> IMU_GetData()
```

- 中断中不做浮点计算，也不做 CRC；
- 1 ms 任务单次最多处理 32 字节，正常 500 Hz 数据只约 4.5 bytes/ms；
- 原 `motor_crc.c` 的 512 字节查表没有带入，CRC 已用紧凑位算法融合进 `imu.c`；
- 原参考程序的 3 秒阻塞延时已改为分时计时，到期后非阻塞发送配置命令；
- 连续 20 ms 没有正确新帧时 `valid=false`；
- 原始累计角度跨越 `32767 -> -32768` 时会按模运算连续展开；
- `CAR_IMU_YAW_POLARITY` 可统一反转角度和角速度方向；
- `IMU_StartGyroCalibration()` 默认静止收集 500 帧，约 1 秒完成；
- `IMU_ResetYaw()` 只把应用层相对偏航角清零，不写模块内部寄存器。

上板时先让模块静止，观察调试 CSV 中 `imu_valid`、`yaw_x10`、`gyro_x10` 和
`imu_crc_errors`。若 CRC 持续增长，优先检查波特率、TX/RX 交叉、共地、电平和
串口干扰，不要用未经验证的角度参与车辆转向控制。

## 9. OLED 软件 I2C 和可选硬件 I2C

OLED 已使用独立软件 I2C，不占用硬件 I2C Controller，也不占用 IMU 的 UART。当前驱动
对应 128×64 SSD1306、7 位地址 0x3C，并保留江协科技 OLED V2.0 的显存、字体、
中文和绘图 API。

### 9.1 OLED 接线与电气要求

```text
OLED VCC -> 3.3 V
OLED GND -> GND
OLED SCL -> 任意可用 GPIO（待指定）
OLED SDA -> 任意可用 GPIO（待指定）
```

建议给 OLED 使用 3.3 V，避免某些模块把 I2C 上拉到 5 V。检查模块是否已有上拉；
如果没有，在 SCL/SDA 各加约 4.7 kΩ 到 3.3 V。

### 9.2 SysConfig

在 SysConfig 添加两个普通数字 GPIO，建议命名为 `OLED_SCL` 和 `OLED_SDA`。BSP
使用输出使能模拟开漏：输出低时主动下拉，输出高时关闭输出并由电阻上拉。因此
不需要硬件 I2C 外设，也不能把总线高电平做成推挽输出。

然后在 `board_config.h`：

```c
#define CAR_OLED_SOFT_I2C_READY 1U
```

若生成符号不是 `OLED_SCL_PORT/OLED_SCL_PIN` 和
`OLED_SDA_PORT/OLED_SDA_PIN`，只修改其下方四个别名。

`CAR_OLED_SOFT_I2C_DELAY_CYCLES` 控制位间隔，应使用示波器观察 SCL 后调整。该短
忙等只形成软件 I2C 时序，CPU 不会进入休眠。

### 9.3 调用限制

- `OLED_Init()` 应在供电稳定后、车辆起跑前调用；
- `OLED_IsConnected()` 可检查初始化/最近写入是否全部收到 ACK；
- 完整 `OLED_Update()` 要发送 1 KiB，软件 I2C 下耗时较长；
- 车辆运行时优先使用 `OLED_UpdateArea()` 更新小区域；
- 禁止在 ISR、1 ms 传感器任务或 5 ms 控制任务中刷新 OLED；
- OLED 无应答不能影响电机控制。

OLED 软件 I2C 和 IMU UART 互不占用同一个外设实例，但仍应避免把 OLED 整屏刷新
放进 1 ms IMU 接收任务或 5 ms 控制任务。

## 10. 首次通电安全顺序

MG513 铭牌堵转电流约 2.8 A，而 TB6612 常用连续输出能力约为 1.2 A/通道。不要把
TB6612 的短时峰值能力当成可持续电流；调试时应限制占空比、避免卡死车轮，并关注
驱动芯片温升。后续建议增加堵转判断或电流检测。

1. 不接电机电源，只给 MCU 供电；
2. 确认 STBY 上电保持低；
3. 确认 PWMA/PWMB 上电为低；
4. 接 TB6612 逻辑电源，仍不接电机；
5. 用示波器检查两个 PWM 输出；
6. 架空车轮再接电机电源；
7. 先将占空比限制设低；
8. 单独测试左轮；
9. 单独测试右轮；
10. 确认紧急停止会立刻关闭 STBY；
11. 最后让车轮接地测试。

不要在理论 1560 count/圈尚未实测确认时直接高速闭环运行。

## 11. 建议联调阶段和完成标准

### 阶段 A：基础通信

- 1 ms 时间正确；
- 调试 UART 能收发；
- IMU UART 在 500 Hz 下不溢出且 CRC 稳定；
- R/S/X/C/E/G/I 命令行为正确。

灰度 GPIO 可以与通信阶段并行验收：先不接电机电源，观察完整帧计数约 125 Hz，
逐个遮挡 CH0~CH7，确认只有对应数组元素变化，再验证左右顺序。

### 阶段 B：电机开环

- 两轮分别正反转；
- `Motor_SetTargetPermille()` 正值都使车辆向前；
- 占空比斜坡可观察；
- S/X 能安全停机。

### 阶段 C：编码器

- 向前都为正；
- 一圈计数准确；
- 低速没有异常巨大跳变；
- 高速不丢计数。

### 阶段 D：速度闭环

- 分别调左右 PI；
- 阶跃响应不过度振荡；
- 堵住轮子时输出会限幅；
- 松手后能恢复；
- 相同目标速度下车辆基本直行。

### 阶段 E：循迹

- OUT 有效电平、CH0~CH7 顺序和二值映射正确；
- 位置符号正确；
- 先只开 P，低速跑通；
- 再加入 D 抑制摆动；
- 最后增加弯道降速和直线速度。

## 12. 工程构建注意事项

复制或新增目录后，在 CCS Theia 中：

1. 右键工程执行 Refresh；
2. 确认 app/bsp/config/control/drivers 都显示；
3. 执行 Clean Project；
4. 执行 Build Project；
5. 如果 SysConfig 报引脚冲突，先解决复用冲突，不要改生成文件。

整个工程明确不使用 MCU 睡眠、WFI、Standby 或低功耗策略。TB6612 的 STBY 只是
电机驱动使能信号，与 MCU 低功耗无关。
