# LP-MSPM0G3507 开发板引脚、跳线与电赛小车资源速查

## 1. 文档用途和证据边界

本文把以下信息整理到一处：

1. `LP-MSPM0G3507引脚功能表.xlsx` 中 60 个 PA/PB 引脚的板级状态；
2. 用户提供的 LP-MSPM0G3507 BoosterPack 接口功能图；
3. 当前循迹车工程 `empty.syscfg` 已经使用的资源；
4. MSPM0 SDK 2.11.00.07 中与第二 UART 相关的可复用示例；
5. 电赛小车后续分配 PWM、QEI、ADC、OLED 和 HC-05 时的避坑规则。

整理日期：2026-07-23。

必须先区分三种信息：

- **Excel 的“是否可以使用”**：描述 LaunchPad 板级占用、跳线和引出条件；
- **板图的 UART/PWM/ADC/SPI 标签**：描述 BoosterPack 接口上的常用功能位置；
- **芯片完整 PinMux**：决定某个引脚能否连接到某个具体 UART/TIMER/ADC 实例。

前两项不能代替芯片 PinMux。最终分配必须在本工程的器件、LQFP-64 封装和当前
SysConfig 中通过无冲突校验，再核对自动生成宏。

## 2. 一眼结论

Excel 共列出 PA0~PA31 和 PB0~PB27，合计 60 个 GPIO 名称：

| 板级分类 | 数量 | 引脚 |
|---|---:|---|
| 标记 `√` | 50 | 可在满足跳线、板载器件和接口条件后使用 |
| 标记 `×` | 9 | PA2~PA6、PA19、PA20、PA21、PA23 |
| 标记“不建议使用” | 1 | PA18 |

最重要的保留和避坑项：

- PA19/PA20 是 XDS SWDIO/SWCLK，正常下载调试时保留；
- PA18 同时涉及 S1、默认 BSL 和 SW2，不建议分配给关键实时信号；
- PA0/PA1 是 5 V Open-Drain 特殊脚，虽然 Excel 标记可用，也不应当作普通推挽
  GPIO/UART 首选；
- PA2~PA6 是 ROSC/LF/HF 晶振相关脚，Excel 明确标记不可使用；
- PA21/PA23 是 VREF-/VREF+，Excel 明确标记不可作为普通 I/O；
- PA10/PA11 已由本项目配置成 IMU UART0，且涉及 J21/J22 到 XDS/BoosterPack 的
  路由，实物上必须检查跳线；
- 任何连接到板载按键、LED、光线传感器或温度传感器的脚，都要先处理对应跳线，
  否则外部模块和板载器件可能同时驱动或加载同一信号。

## 3. BoosterPack 接口图转录

以下顺序均为用户所给图片中“从上到下”。星号表示图片上有选择或复用提示，具体
路由应结合 Excel 的跳线列确认。

### 3.1 左上接口

| 顺序 | 引脚/电源 | 图片标注的常用功能 | 板级备注 |
|---:|---|---|---|
| 1 | 3V3 | 3.3 V 电源 | 外设逻辑电源参考 |
| 2 | PA25 | ADC0.2 | BoosterPack |
| 3 | PB23/PA9* | UART RX | J14 选择，且涉及 SW1 |
| 4 | PA8 | UART TX | BoosterPack |
| 5 | PA26 | 未特别标注 | Excel 标有光线传感器/J18 |
| 6 | PB24 | ADC0.5 | Excel 标有温度传感器/J9 |
| 7 | PB9 | SPI CLK | BoosterPack |
| 8 | PA27 | 未特别标注 | Excel 标有光线传感器/J17 |
| 9 | PB2 | I2C SCL | BoosterPack |
| 10 | PB3 | I2C SDA | BoosterPack |

### 3.2 右上接口

| 顺序 | 引脚/电源 | 图片标注的常用功能 | 板级备注 |
|---:|---|---|---|
| 1 | GND | 地 | 所有外设必须共地 |
| 2 | PB12 | PWM | 具体定时器/CC 由 SysConfig 决定 |
| 3 | PB17 | SPI CS | 也可在 PinMux 允许时作其他功能 |
| 4 | PB15 | 未特别标注 | Excel 无板载占用 |
| 5 | NRST | 复位 | 不作为普通 GPIO |
| 6 | PB8 | SPI PICO | 控制器输出/外设输入 |
| 7 | PB7 | SPI POCI | SDK 示例也可把它用作 UART1 RX |
| 8 | PB6 | SPI CS | SDK 示例也可把它用作 UART1 TX |
| 9 | PB0 | SPI CS | BoosterPack |
| 10 | PB16 | 未特别标注 | Excel 无板载占用 |

### 3.3 左下接口

| 顺序 | 引脚/电源 | 图片标注的常用功能 | 板级备注 |
|---:|---|---|---|
| 1 | 5V | 5 V 电源 | 不能直接当 3.3 V 逻辑电平 |
| 2 | GND | 地 | 所有外设必须共地 |
| 3 | PB19 | ADC1.6 | BoosterPack |
| 4 | PA22 | ADC0.7 | Excel 标有光线传感器/J16 |
| 5 | PB18 | ADC1.5 | BoosterPack |
| 6 | PA18 | ADC1.3 | 默认 BSL/S1/SW2，不建议使用 |
| 7 | PA24 | ADC0.3 | BoosterPack |
| 8 | PA17 | ADC1.2 | BoosterPack |
| 9 | PA16/PA18* | ADC1.1/ADC1.3* | J15 选择 SW2 路由；优先 PA16 |
| 10 | PA15 | DAC OUT | BoosterPack |

### 3.4 右下接口

| 顺序 | 引脚 | 图片标注的常用功能 | 当前工程状态 |
|---:|---|---|---|
| 1 | PB4 | PWM | 未分配 |
| 2 | PB1 | PWM | 未分配 |
| 3 | PA28 | PWM | 未分配 |
| 4 | PA31 | PWM | 未分配 |
| 5 | PB20 | PWM | 未分配 |
| 6 | PB13 | PWM | 未分配 |
| 7 | PA10 | LINTX | 已配置为 IMU UART0 TX |
| 8 | PA11 | LINRX | 已配置为 IMU UART0 RX |
| 9 | PA12 | CANTX | 未分配 |
| 10 | PA13 | CANRX | 未分配 |

图片上的 LIN/CAN/SPI/PWM 是接口常用角色，不表示这些引脚只能做这一种功能。

## 4. Excel 完整板级引脚表

### 4.1 GPIOA：PA0~PA31

| 引脚 | LaunchPad 板级功能 | 相关跳线 | Excel结论 | 引出位置 |
|---|---|---|---|---|
| PA0 | 5V Open-Drain 引脚 | J19 设置上拉/J4 连接 LED1 | √，但属特殊脚 | 下方未焊接区 |
| PA1 | 5V Open-Drain 引脚 | J20 设置上拉 | √，但属特殊脚 | 下方未焊接区 |
| PA2 | ROSC 引脚 | 无 | × | 未列出 |
| PA3 | LFXIN 引脚 | 无 | × | 未列出 |
| PA4 | LFXOUT 引脚 | 无 | × | 未列出 |
| PA5 | HFXIN 引脚 | 无 | × | 未列出 |
| PA6 | HFXOUT 引脚 | 无 | × | 未列出 |
| PA7 | 无 | 无 | √ | 下方未焊接区 |
| PA8 | 无 | 无 | √ | BoosterPack |
| PA9 | SW1 | J14 选择 SW1 引脚 PB23/PA9 | √，需确认 J14 | BoosterPack |
| PA10 | XDS UART TX | J21 设置引出至 XDS/BoosterPack | √，需确认 J21 | BoosterPack |
| PA11 | XDS UART RX | J22 设置引出至 XDS/BoosterPack | √，需确认 J22 | BoosterPack |
| PA12 | 无 | 无 | √ | BoosterPack |
| PA13 | 无 | 无 | √ | BoosterPack |
| PA14 | 无 | 无 | √ | 下方未焊接区 |
| PA15 | 无 | 无 | √ | BoosterPack |
| PA16 | SW2 | J15 选择 SW2 引脚 PA16/PA18 | √，需确认 J15 | BoosterPack |
| PA17 | 无 | 无 | √ | BoosterPack |
| PA18 | S1/默认 BSL/SW2 | J8 连接 S1/J15 选择 PA16/PA18 | 不建议使用 | BoosterPack |
| PA19 | XDS SWDIO | J101 连接 XDS110 | × | 未列出 |
| PA20 | XDS SWCLK | J101 连接 XDS110 | × | 未列出 |
| PA21 | VREF- | 无 | × | 未列出 |
| PA22 | 光线传感器 | J16 连接光线传感器 | √，需确认 J16 | BoosterPack |
| PA23 | VREF+ | 无 | × | 未列出 |
| PA24 | 无 | 无 | √ | BoosterPack |
| PA25 | 无 | 无 | √ | BoosterPack |
| PA26 | 光线传感器 | J18 连接光线传感器 | √，需确认 J18 | BoosterPack |
| PA27 | 光线传感器 | J17 连接光线传感器 | √，需确认 J17 | BoosterPack |
| PA28 | 无 | 无 | √ | BoosterPack |
| PA29 | QEI | J12 未焊接 | √，接口可能需焊接 | QEI Interface |
| PA30 | QEI | J12 未焊接 | √，接口可能需焊接 | QEI Interface |
| PA31 | 无 | 无 | √ | BoosterPack |

### 4.2 GPIOB：PB0~PB27

| 引脚 | LaunchPad 板级功能 | 相关跳线 | Excel结论 | 引出位置 |
|---|---|---|---|---|
| PB0 | 无 | 无 | √ | BoosterPack |
| PB1 | 无 | 无 | √ | BoosterPack |
| PB2 | 无 | 无 | √ | BoosterPack |
| PB3 | 无 | 无 | √ | BoosterPack |
| PB4 | 无 | 无 | √ | BoosterPack |
| PB5 | 无 | 无 | √ | 下方未焊接区 |
| PB6 | 无 | 无 | √ | BoosterPack |
| PB7 | 无 | 无 | √ | BoosterPack |
| PB8 | 无 | 无 | √ | BoosterPack |
| PB9 | 无 | 无 | √ | BoosterPack |
| PB10 | 无 | 无 | √ | 下方未焊接区 |
| PB11 | 无 | 无 | √ | 下方未焊接区 |
| PB12 | 无 | 无 | √ | BoosterPack |
| PB13 | 无 | 无 | √ | BoosterPack |
| PB14 | QEI | J12 未焊接 | √，接口可能需焊接 | QEI Interface |
| PB15 | 无 | 无 | √ | BoosterPack |
| PB16 | 无 | 无 | √ | BoosterPack |
| PB17 | 无 | 无 | √ | BoosterPack |
| PB18 | 无 | 无 | √ | BoosterPack |
| PB19 | 无 | 无 | √ | BoosterPack |
| PB20 | 无 | 无 | √ | BoosterPack |
| PB21 | S2 按键 | 无 | √，有板载按键负载 | 下方未焊接区 |
| PB22 | RGB Blue LED | J5 连接 LED | √，需确认 J5 | 下方未焊接区 |
| PB23 | SW1 | J14 选择 SW1 引脚 PB23/PA9 | √，需确认 J14 | BoosterPack |
| PB24 | 温度传感器 | J9 选择传感器连接 PA26/PB24 | √，需确认 J9 | BoosterPack |
| PB25 | SMA/ADC | 可焊接 SMA 接口 | √，需焊接时确认负载 | 下方未焊接区 |
| PB26 | RGB Red LED | J6 连接 LED | √，需确认 J6 | 下方未焊接区 |
| PB27 | RGB Green LED | J7 连接 LED | √，需确认 J7 | 下方未焊接区 |

## 5. 跳线和板载器件速查

| 跳线/焊位 | 相关引脚 | 作用和风险 |
|---|---|---|
| J4/J19 | PA0 | LED1/5 V Open-Drain 上拉配置；普通 GPIO 使用前先确认 |
| J20 | PA1 | 5 V Open-Drain 上拉配置 |
| J14 | PA9/PB23 | 选择 SW1/接口信号路由；不能只看丝印猜当前连接 |
| J21 | PA10 | XDS UART TX 与 BoosterPack 路由；IMU TX 使用前必须检查 |
| J22 | PA11 | XDS UART RX 与 BoosterPack 路由；IMU RX 使用前必须检查 |
| J15 | PA16/PA18 | SW2 引脚选择；PA18 还涉及默认 BSL |
| J8 | PA18 | 连接板载 S1；关键控制信号不建议使用 PA18 |
| J101 | PA19/PA20 | XDS110 SWD 下载调试连接，正常开发阶段保留 |
| J16/J18/J17 | PA22/PA26/PA27 | 板载光线传感器连接；外接 ADC 前避免并联负载 |
| J12 | PA29/PA30/PB14 | QEI Interface，Excel 标记“未焊接” |
| J5/J6/J7 | PB22/PB26/PB27 | RGB 蓝/红/绿 LED；用作外部信号时断开或评估 LED 负载 |
| J9 | PA26/PB24 | 温度传感器选择；与 PA26 光线传感器资源一起检查 |
| SMA 焊位 | PB25 | 可焊 SMA 到 ADC；未焊时仍要确认焊盘和外部负载 |

## 6. 当前循迹车工程资源占用

| 功能 | 外设/引脚 | 状态 | 注意事项 |
|---|---|---|---|
| IMU TX | UART0 TX / PA10 | SysConfig 已生成 | MCU PA10 -> IMU RX；检查 J21 路由 |
| IMU RX | UART0 RX / PA11 | SysConfig 已生成 | MCU PA11 <- IMU TX；检查 J22 路由 |
| SWD 下载调试 | PA19/PA20 | 保留 | 不参与整车外设分配 |
| HC-05 TX | UART1 TX / PB4 | SysConfig 已配置 | MCU PB4 -> HC-05 RX；PB4 不再用于 PWM |
| HC-05 RX | UART1 RX / PB5 | SysConfig 已配置 | MCU PB5 <- HC-05 TX；确认未焊接区引出 |
| OLED | 两个普通 GPIO | 软件 I2C 驱动完成、未选引脚 | 需要上拉，避免 5 V 上拉 |
| TB6612 PWM | 两个 PWM 输出 | 未配置 | 优先同一定时器两个 CC，20 kHz |
| TB6612 方向/STBY | 五个普通 GPIO | 未配置 | STBY 上电默认低 |
| 左右编码器 | 四个 QEI 信号 | 未配置 | 两轮共需 A/B 四路，不要只按 J12 三脚假定足够 |
| 八路循迹 | AD0/AD1/AD2 输出 + OUT 输入 | 驱动完成、GPIO 未配置 | 四线轮询，OUT 每路只有 0/1 |
| 1 ms 时基 | 一个定时器 | 未配置 | ISR 只调用 `BSP_Time_Tick1msFromISR()` |

## 7. 面向循迹小车的候选分配思路

这里给的是“候选和筛选顺序”，不是最终接线。只有 SysConfig 保存、生成和构建通过
后，才能把候选写成正式分配。

### 7.1 HC-05 第二 UART

用户当前已经在 SysConfig 保存的正式分配为：

```text
实例名 = HC05_UART
UART1 TX = PB4
UART1 RX = PB5
115200 baud, 8N1, FIFO enabled
RX FIFO threshold = >= 1 entry
RX interrupt = enabled
```

PB4 是 BoosterPack 引脚，也在板图中标为 PWM 候选；配置为 HC-05 TX 后，后续 TB6612
PWM 不得再使用 PB4。PB5 在表格中位于下方未焊接区，应确认开发板实物是否已经焊接
排针或使用可靠飞线。该分配避开了 PA18 的默认 BSL/S1/SW2 风险。

保留的替代引脚参考（仅在 PB4/PB5 接线不便或发生资源冲突时考虑）：

```text
UART1 TX = PB6
UART1 RX = PB7
```

SDK 的 LP_MSPM0G3507 BSL UART 示例使用这组映射；它们占用图片标注的 SPI
CS/POCI 位置。是否可换必须让当前 SysConfig PinMux 在全部外设同时存在时判定。

### 7.2 TB6612 PWM

接口图标出的 PWM 候选包括：

```text
PB12, PB4, PB1, PA28, PA31, PB20, PB13
```

从中选择两路时优先：

1. 能属于同一个定时器的两个 CC 通道；
2. 不与 QEI 和 1 ms 时基抢同一计时器资源；
3. 不与已用 UART0、未来 HC-05 UART 冲突；
4. 上电初始比较值可以可靠保持为 0。

图片只证明接口位置标注为 PWM 候选，不证明任意两脚都能组成同一个定时器。

### 7.3 八路数字循迹模块

根据模块 PDF，接口并不是八路模拟 ADC。模块内部用三位地址选择八路比较器结果：

```text
AD0 = 地址最低位 A
AD1 = 地址中间位 B
AD2 = 地址最高位 C
OUT = 当前所选通道的数字输出

CH0 = 000, CH1 = 001, ... , CH7 = 111
```

因此 MCU 只需要三个普通数字输出和一个数字输入，共四根信号线。用户还没有指定
最终 GPIO，建议在 SysConfig 中创建以下实例并选择无板载冲突的普通 GPIO：

```text
LINE_MUX_AD0：Digital Output，初始低
LINE_MUX_AD1：Digital Output，初始低
LINE_MUX_AD2：Digital Output，初始低
LINE_MUX_OUT：Digital Input
```

模块资料要求 5 V 供电，而 MSPM0 GPIO 只允许 3.3 V 逻辑。接 OUT 前必须用万用表或
示波器测量其高电平；若接近 5 V，增加电平转换或按输入电流/阈值正确计算的分压，
不能直接接 MCU。AD0~AD2 从 MCU 输出 3.3 V 时，还需确认模块能可靠识别为高电平。

软件每 1 ms 读取一路并切换到下一地址，8 ms 才提交完整八路帧，约 125 帧/秒，
没有阻塞延时。若 CH0 实际在车辆右侧，设置 `CAR_LINE_MUX_REVERSE_ORDER=1`；若
OUT 有效电平相反，修改 `CAR_LINE_MUX_ACTIVE_LEVEL`，不要改质心公式。

### 7.4 左右编码器

Excel 在 QEI Interface 列出 PA29、PA30、PB14，且 J12 未焊接；两只 AB 相编码器
实际需要四路输入。因此流程应是：

1. 先检查 J12 实际原理图和焊接状态；
2. 在 SysConfig 中分别尝试两个硬件 QEI 实例及各自 A/B PinMux；
3. 若板载 QEI Interface 不足四路，再从普通引出脚选择剩余复用脚；
4. 最后确认最大边沿频率、计数方向和溢出扩展方案。

### 7.5 OLED 和 TB6612 普通 GPIO

OLED 软件 I2C、AIN1/AIN2/BIN1/BIN2/STBY 对外设复用要求较低，应在 UART、PWM、
QEI 和 ADC 分配完成后再选普通 GPIO。选择原则：

- 优先 Excel 标记“无、无、√、BoosterPack”的引脚；
- 避开 PA0/PA1、PA18、PA19/PA20 和 VREF/晶振脚；
- 避免板载 LED/按键/传感器，除非明确断开相应跳线；
- OLED SCL/SDA 均需上拉到 3.3 V，不能上拉到 5 V；
- STBY 必须在复位和初始化阶段保持低。

## 8. 推荐的正式分配顺序

1. 固定保留 PA19/PA20、供电、GND、NRST 和特殊脚；
2. 锁定现有 IMU UART0/PA10/PA11，并检查 J21/J22；
3. 保留并验证 HC-05 UART1/PB4/PB5，确认 PB5 实物引出并从 PWM 候选中移除 PB4；
4. 为两只编码器选择两个 QEI A/B 组合；
5. 为 TB6612 选择两路 PWM，同时保留 1 ms 定时器；
6. 为循迹模块分配 AD0/AD1/AD2/OUT 四个普通 GPIO；
7. 最后分配 TB6612 五个 GPIO 和 OLED 两个 GPIO；
8. 把每个最终选择写入下面的正式记录表和 `config/board_config.h`；
9. 保存 SysConfig，核对生成宏，Clean + Build；
10. 用万用表/示波器/逻辑分析仪逐项上板确认。

## 9. 最终接线记录表（待逐项填写）

| 模块 | 信号 | 最终 MCU 引脚 | SysConfig 实例/通道 | 跳线/接口 | 上板验证 |
|---|---|---|---|---|---|
| IMU | TX -> IMU RX | PA10 | UART0 TX | J21 待确认 | 待验证 |
| IMU | RX <- IMU TX | PA11 | UART0 RX | J22 待确认 | 待验证 |
| HC-05 | TX -> HC-05 RX | PB4 | UART1 / `HC05_UART` TX | BoosterPack | 待验证 |
| HC-05 | RX <- HC-05 TX | PB5 | UART1 / `HC05_UART` RX | 下方未焊接区 | 待验证 |
| TB6612 | PWMA | 待定 | PWM/CC 待定 | 待定 | 待验证 |
| TB6612 | PWMB | 待定 | PWM/CC 待定 | 待定 | 待验证 |
| TB6612 | AIN1/AIN2 | 待定 | GPIO | 待定 | 待验证 |
| TB6612 | BIN1/BIN2 | 待定 | GPIO | 待定 | 待验证 |
| TB6612 | STBY | 待定 | GPIO | 待定 | 待验证 |
| 左编码器 | A/B | 待定 | QEI 待定 | 待定 | 待验证 |
| 右编码器 | A/B | 待定 | QEI 待定 | 待定 | 待验证 |
| 循迹 | AD0/AD1/AD2 | 待定 | 三个普通 GPIO 输出 | 待定 | 待验证 |
| 循迹 | OUT | 待定 | 普通 GPIO 输入 | 先测高电平 | 待验证 |
| OLED | SCL/SDA | 待定 | 普通 GPIO | 4.7 kΩ 到 3.3 V | 待验证 |
| 系统 | 1 ms tick | 无外接 | Timer 待定 | 无 | 待验证 |

## 10. 每次改引脚后的核对清单

- [ ] 没有占用 PA19/PA20 SWD；
- [ ] 若使用 PA18，已明确评估默认 BSL、S1/SW2 和 J8/J15；
- [ ] PA10/PA11 与 J21/J22 的物理路由正确；
- [ ] 同一 UART 的 TX/RX 确实属于同一外设实例；
- [ ] PWM 的定时器和 CC 通道与 QEI、1 ms tick 不冲突；
- [ ] 灰度 OUT 高电平不超过 3.3 V，CH0~CH7 物理左右顺序正确；
- [ ] 板载按键、LED、光线/温度传感器跳线已处理；
- [ ] 外设逻辑电平为 3.3 V，所有模块共地；
- [ ] SysConfig 没有红色冲突并成功保存；
- [ ] 自动生成的 `*_INST`、`*_PIN`、`*_IRQN`、`*_IRQHandler` 与 BSP 别名一致；
- [ ] `READY=0` 阶段仍可安全编译；
- [ ] 改成 `READY=1` 后 Clean + Build 完整链接；
- [ ] 上电先不接电机电源，测完静态电平和 PWM 再接负载；
- [ ] 文档和 `board_config.h` 已同步更新。

## 11. 来源

- 原始表格：`C:/Users/林~/Downloads/LP-MSPM0G3507引脚功能表.xlsx`，Sheet1 A1:E61；
- 板卡图片：用户提供的 LP-MSPM0G3507 BoosterPack 接口功能图；
- 当前工程：`empty.syscfg`，IMU UART0/PA10/PA11，HC-05 UART1/PB4/PB5；
- 八路灰度资料：用户提供的 `八路灰度模块.pdf`，确认三位地址加单路数字 OUT；
- SDK 示例：`examples/nortos/LP_MSPM0G3507/bsl/`
  `bsl_host_mcu_to_mspm0g1x0x_g3x0x_target_uart/bsl_host_mcu_uart.syscfg`，
  其中 UART1 使用 PB6 TX/PB7 RX。

如果后续拿到 LaunchPad 官方原理图或用户指南，应优先用官方网络名和跳线真值表
复核本文，再把复核日期写入本节；不要静默覆盖现有结论。
