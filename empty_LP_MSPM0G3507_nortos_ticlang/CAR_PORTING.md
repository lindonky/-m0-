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
- 循迹传感器通道数、模拟/数字类型、左右排列；
- 黑线相对背景是 ADC 更高还是更低；
- UART TX/RX 接线；
- OLED 的 SCL/SDA 引脚；驱动已按 128×64 SSD1306、地址 0x3C 移植；
- IMU 型号和总线，仅在需要时提供。

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

## 6. 循迹 ADC

对于 8 路模拟传感器，建议配置 ADC 序列：

- 通道顺序必须与物理最左到最右一致；
- 可用定时器触发 ADC；
- 首版可先中断读取，稳定后再使用 DMA；
- `BSP_LineADC_Read()` 必须一次返回同一帧数据；
- 没有新帧时返回 `false`，不能重复伪装成新数据。

参考 SDK：

```text
adc12_sequence_conversion
adc12_triggered_by_timer_event
adc12_max_freq_dma
```

验收：

- 白底和黑线之间每路都有稳定跨度；
- 通道顺序确实是左到右；
- 电机运行时 ADC 噪声仍可接受；
- 完成 C/E 标定后，黑线目标通道接近 1000，背景接近 0；
- 线从左向右移动时 `position` 从负值连续变化到正值。

## 7. UART

推荐配置：

```text
115200 baud, 8 data bits, no parity, 1 stop bit
```

在 `bsp/bsp_uart.c` 实现非阻塞接口：

- `BSP_UART_TryWrite()`：只入队，队列满时返回 false；
- `BSP_UART_TryReadByte()`：RX 队列有数据才返回 true；
- TX/RX 使用中断环形缓冲或 DMA；
- 不允许等待一个完整字符串发送完毕。

上层命令：R 开始、S 停止、X 紧急停止、C 开始标定、E 结束标定。

## 8. OLED 软件 I2C 和可选硬件 I2C

OLED 已使用独立软件 I2C，不占用以后给 IMU 使用的硬件 I2C Controller。当前驱动
对应 128×64 SSD1306、7 位地址 0x3C，并保留江协科技 OLED V2.0 的显存、字体、
中文和绘图 API。

### 8.1 OLED 接线与电气要求

```text
OLED VCC -> 3.3 V
OLED GND -> GND
OLED SCL -> 任意可用 GPIO（待指定）
OLED SDA -> 任意可用 GPIO（待指定）
```

建议给 OLED 使用 3.3 V，避免某些模块把 I2C 上拉到 5 V。检查模块是否已有上拉；
如果没有，在 SCL/SDA 各加约 4.7 kΩ 到 3.3 V。

### 8.2 SysConfig

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

### 8.3 调用限制

- `OLED_Init()` 应在供电稳定后、车辆起跑前调用；
- `OLED_IsConnected()` 可检查初始化/最近写入是否全部收到 ACK；
- 完整 `OLED_Update()` 要发送 1 KiB，软件 I2C 下耗时较长；
- 车辆运行时优先使用 `OLED_UpdateArea()` 更新小区域；
- 禁止在 ISR、1 ms 传感器任务或 5 ms 控制任务中刷新 OLED；
- OLED 无应答不能影响电机控制。

IMU 仍建议使用独立硬件 I2C Controller 和 repeated-start，优先级高于 OLED。

## 9. 首次通电安全顺序

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

## 10. 建议联调阶段和完成标准

### 阶段 A：基础通信

- 1 ms 时间正确；
- UART 能收发；
- R/S/X/C/E 都能改变状态。

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

- 标定正确；
- 位置符号正确；
- 先只开 P，低速跑通；
- 再加入 D 抑制摆动；
- 最后增加弯道降速和直线速度。

## 11. 工程构建注意事项

复制或新增目录后，在 CCS Theia 中：

1. 右键工程执行 Refresh；
2. 确认 app/bsp/config/control/drivers 都显示；
3. 执行 Clean Project；
4. 执行 Build Project；
5. 如果 SysConfig 报引脚冲突，先解决复用冲突，不要改生成文件。

整个工程明确不使用 MCU 睡眠、WFI、Standby 或低功耗策略。TB6612 的 STBY 只是
电机驱动使能信号，与 MCU 低功耗无关。
