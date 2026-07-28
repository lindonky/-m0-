# MSPM0G3507 电赛循迹车工程交接说明

## 1. 文档用途

本文档用于把当前工程完整交接给下一位开发者，回答以下问题：

- 当前工程为什么这样分层；
- 已经实现了哪些功能；
- 哪些代码只是接口或安全占位；
- 系统从传感器到电机的完整数据流是什么；
- 当前工程为什么能编译、但还不能直接驱动实车；
- 下一步应当先配置什么、怎样逐项验收；
- 哪些参数必须根据实物修改；
- 当前版本有哪些已知限制和风险。

硬件接线、SysConfig 外设创建和 BSP 填写步骤请同时阅读
`CAR_PORTING.md`。

## 2. 工程基本信息

- 目标 MCU：TI MSPM0G3507，Cortex-M0+；
- 开发板：LP_MSPM0G3507；
- SDK：MSPM0 SDK 2.11.00.07；
- 编译器：TI Arm Clang 5.1.1.LTS；
- 工程类型：NoRTOS；
- 电机驱动：TB6612FNG；
- 车辆结构：左右双直流电机差速车；
- 当前循迹传感器：八路独立模拟灰度阵列，使用 ADC0 三路和 ADC1 五路；
- 外设：AB 相编码器、500 Hz 串口 IMU、PA12/PA13 软件 I2C SSD1306 OLED、调试 UART。

工程路径：

```text
C:\ti\workspace_ccstheia\empty_LP_MSPM0G3507_nortos_ticlang
```

SDK 参考路径：

```text
C:\ti\mspm0_sdk_2_11_00_07
```

## 3. 本次开发做了什么

原工程最初只有 `empty.c` 和只启用系统时钟的 `empty.syscfg`。本次工作在不修改
SDK、不手改 SysConfig 生成文件的前提下，建立了完整软件骨架；随后与用户逐项通过
SysConfig GUI 确认并正式配置 ADC、UART、1 ms Timer 和 OLED GPIO。

文件变更规模：新建约 50 个文件并修改原入口 `empty.c`；其中包含 24 个 C 源文件。
`empty.syscfg` 现已由用户在 GUI 中正式加入 ADC0/1、UART0/1、TIMG0 和 OLED GPIO。

主要工作如下：

1. 建立配置层、BSP 层、器件驱动层、控制层和应用层；
2. 为 TB6612 实现双通道正反转、滑行、短刹车和 STBY 控制逻辑；
3. 实现电机有符号占空比、极性、限幅、死区、斜坡和紧急停止；
4. 实现编码器增量、速度、RPM、累计距离和低通滤波；
5. 实现八路真实 ADC 的逐通道标定、归一化和加权位置解算；使用 ADC0/ADC1
   双序列完成中断组成完整帧，包含超时重启与诊断计数；
6. 实现通用 PID，包含输出限幅、积分限幅、抗饱和和微分滤波；
7. 实现循迹方向环、弯道降速、差速混合和左右轮速度环；
8. 实现 NoRTOS 毫秒任务调度和整车状态机；
9. 实现 UART 调试协议骨架和整数缩放 CSV 遥测；
10. 移植 500 Hz 串口 IMU 协议，融合 Modbus CRC16、非阻塞 UART 环形缓冲、
    角度回绕、超时、零偏标定和诊断计数；
11. 把江协科技 OLED V2.0 软件 I2C 驱动、字库和绘图函数完整移植到 MSPM0G3507；
12. 修改 `empty.c`，把全部模块接入持续运行的主循环；
13. 使用工程实际 TI Arm Clang 编译全部源码并完成完整链接验证；
14. 检查工程中不存在 MCU 睡眠、WFI 或低功耗策略调用。
15. 核对并接入用户已完成的 HC-05 SysConfig 配置：UART1、PB4 TX、PB5 RX、
    115200 8N1、FIFO、RX `>= 1 entry` 中断。
16. 因硬件改为八路独立模拟灰度，移除原数字复用方案，重写双 ADC 非阻塞 BSP，
    并通过完整候选 SysConfig 验证 ADC 3+5 分组、备用 I2C1 和备用 UART2 可共存。
17. 核对并接入 OLED SysConfig：`OLED_GPIO`、PA12 SCL、PA13 SDA、Initial Set、
    Hi-Z Enable；增加 100 ms 低优先级逐页诊断刷新。

## 4. 当前完成度总览

| 模块 | 状态 | 是否可直接上硬件 | 说明 |
|---|---|---:|---|
| 工程分层和配置文件 | 已完成 | 是 | 参数集中管理 |
| TB6612 上层逻辑 | 已完成 | 等待台架验证 | A/B 通道、方向、刹车、滑行，真实 GPIO/PWM BSP 已接入 |
| 电机控制 | 已完成 | 等待台架验证 | 极性、限幅、斜坡、死区、安全停机 |
| 编码器数学处理 | 已完成 | 等待台架验证 | 左硬件 QEI、右软件 AB、速度、RPM、距离、滤波 |
| 循迹算法 | 已完成首版 | 等待上板验证 | 真实模拟量、标定、质心、丢线 |
| 通用 PID | 已完成 | 是 | 仍需实车调参 |
| 左右速度闭环 | 已完成首版 | 编码器接入后可用 | 默认参数只是起点 |
| 循迹方向闭环 | 已完成首版 | 传感器接入后可用 | 默认使用 PD |
| 差速混合 | 已完成 | 是 | 超限时按比例缩放 |
| 任务调度器 | 软件和 1 ms Timer 已完成 | 等待上板校时 | TIMG0、1 ms ZERO ISR；无休眠、无阻塞延时 |
| 整车状态机 | 已完成基础状态 | 是 | 尚无环岛等比赛元素 |
| HC-05 调试 UART 应用协议 | 已完成 | 等待上板验证 | R/S/X/C/E/G/I 和 CSV |
| HC-05 调试 UART 底层 | 软件、SysConfig、构建已通过 | 等待上板验证 | UART1、PB4/PB5、FIFO/ISR |
| IMU UART 底层 | 软件、SysConfig、构建已通过 | 等待上板验证 | UART0、PA10/PA11、RX/TX 环形缓冲 |
| PWM/GPIO 底层 | 已实现 | 等待台架验证 | TIMG12 PB13/PA31、20 kHz；五根控制线已映射 |
| 编码器底层 | 已实现 | 等待台架验证 | TIMG8 PB6/PB7 + GPIOB PB8/PB9、GROUP1 ISR |
| 八路循迹 ADC BSP | 软件、SysConfig 已完成 | 等待上板验证 | ADC0 3路 + ADC1 5路、非阻塞完整帧 |
| 500 Hz 串口 IMU | 协议完成 | UART 配置后可用 | 9 字节帧、CRC、回绕、超时、标定 |
| SSD1306 OLED 驱动 | 软件、GPIO和实物显示已通过 | 80 MHz 后待复测 | PA12/PA13、0x3C、ADC与电机/编码器双诊断画面 |
| 按键、电池、Flash | 未实现 | 否 | 后续增强项 |
| 十字/环岛/停车线 | 未实现 | 否 | 依赛题规则开发 |

“已完成首版”表示软件结构和基本算法已经存在，但必须经过实车数据验证和参数
调整，不能理解为比赛参数已经调好。

### 4.1 进度估算

以下百分比用于交接排期，不代表比赛完成度可以简单相加：

| 工作包 | 当前估算 | 判定依据 |
|---|---:|---|
| 软件架构和模块接口 | 100% | 分层、接口和主循环已建立并链接通过 |
| 与引脚无关的基础算法 | 90% | 首版已实现，仍缺实车数据验证 |
| MSPM0 外设/SysConfig 接入 | 95% | IMU、HC-05、ADC、TIMG0、OLED、PWM、QEI/右编码器 GPIO 均已配置；剩余实物验证 |
| 电机和编码器台架验证 | 0% | 尚未连接实物 |
| 基础速度闭环 | 30% | 软件存在，硬件未接入、参数未整定 |
| 基础连续线循迹 | 60% | 双 ADC 已正式启用，解算和控制存在；缺传感器接线、标定与实车验证 |
| 比赛赛道元素 | 0% | 尚未取得具体赛题规则 |
| IMU | 80% | 协议/UART BSP/诊断完成，缺实际引脚和上板数据验证 |
| OLED | 90% | 驱动/字库/PA12/PA13/诊断页完成，缺实物 ACK、波形和显示验证 |
| 安全与诊断增强 | 20% | 有丢线/急停，缺欠压、堵转、超时等 |

当前里程碑可定义为：**“软件框架 V0.1 完成，等待接线表进入硬件移植阶段”**。

## 5. 软件分层与依赖方向

```text
empty.c
  └─ app/                  整车状态和任务调度
      ├─ control/          PID、方向环、速度环、差速混合
      │   └─ drivers/      TB6612、电机、编码器、循迹、IMU、OLED
      │       └─ bsp/      MSPM0 外设边界，唯一允许接触具体引脚的位置
      └─ config/           车辆参数和接线记录
```

依赖只能从上往下：

- 应用层可以调用控制层和驱动层；
- 控制层不能直接访问 MSPM0 寄存器；
- 驱动层不能直接依赖业务状态机；
- 只有 BSP 应该使用 `ti_msp_dl_config.h` 和 DriverLib；
- `Debug/ti_msp_dl_config.*` 是生成文件，禁止手改。

这样做的目的，是让更换引脚、定时器、ADC 通道甚至传感器型号时，不需要重写
PID、循迹和整车状态机。

## 6. 运行时完整数据流

### 6.1 循迹方向链路

```text
ADC0 MEM0~2 + ADC1 MEM0~4 软件触发
  → 两个末尾 MEM 完成中断分别置位
  → 两组均完成后提交八路真实 12 位结果
  → 每通道黑白标定
  → 0~1000 归一化强度
  → 加权质心位置 position(-1~+1)
  → 循迹 PD/PID
  → steeringMmS
```

位置符号约定：

- `position < 0`：赛道线偏车辆左侧；
- `position = 0`：线位于阵列中心；
- `position > 0`：赛道线偏车辆右侧。

正 `steeringMmS` 表示右转，差速混合会提高左轮目标速度并降低右轮目标速度。

当前模块输出连续模拟电压，`raw[0..7]` 是真实 0~4095 ADC 结果。默认 ADC0 负责
数组 0~2，ADC1 负责数组 3~7；两个序列都完成后才提交，避免跨 ADC 半新半旧。
每次提交后立即重启下一帧，1 ms 任务下目标帧率接近 1 kHz。任一 ADC 连续五次
任务轮询仍未完成时，BSP 会重新武装两个序列并增加重启诊断计数，不会阻塞主循环。

### 6.2 速度闭环链路

```text
forwardMmS + steeringMmS
  → 左右目标轮速
  → 与左右编码器实测速度比较
  → 左右独立 PI/PID
  → -1000~+1000 电机指令
  → 电机极性/限幅/斜坡/死区
  → TB6612 A/B 通道
```

### 6.3 IMU 链路

```text
115200 8N1 / 500 Hz / 9 字节帧
  → UART RX ISR 排空硬件 FIFO
  → 64 字节软件环形缓冲
  → 1 ms 任务帧头同步
  → Modbus CRC16
  → 0.1° 和 0.1°/s 缩放
  → 16 位角度回绕展开
  → 零偏、超时和诊断
  → IMU_GetData()
```

IMU 是独立 UART，不能与调试 CSV 串口共用。CRC 错误、协议头错误、软件缓冲
溢出和主任务漏取帧都有独立计数，方便区分接线问题和调度负载问题。

### 6.4 丢线处理

短时间丢线时：

- 循迹模块保留最后一次有效位置；
- 方向控制器降低前进速度；
- 按最后一次偏移方向继续搜线。

超过 `CAR_LINE_LOST_STOP_MS` 后：

- 状态转为 `CAR_STATE_LINE_LOST`；
- 禁止 TB6612 输出；
- 复位速度环；
- 需要重新发送启动命令才能运行。

## 7. 调度和实时性

调度器以 `BSP_Time_GetMs()` 为时间基准：

| 周期 | 任务 | 设计原因 |
|---:|---|---|
| 1 ms | IMU UART 解析、领取循迹 ADC 完整帧 | 接住 500 Hz IMU；灰度目标约 1 kHz 帧率 |
| 5 ms | 编码器、方向环、速度环、电机 | 200 Hz 控制周期 |
| 10 ms | 状态和安全检查 | 状态逻辑无需进入高速 ISR |
| 20 ms | UART 遥测 | 避免调试输出占用过多带宽 |
| 100 ms | OLED 一页诊断数据 | 每次最多 120 字节；八页约 0.8 秒轮换一次 |

当前 `TICK_TIMER/TIMG0` 已按 1 ms 周期配置。定时器 ISR 只做：

1. 判断并清除 1 ms 定时器中断标志；
2. 调用 `BSP_Time_Tick1msFromISR()`；
3. 立即退出。

IMU UART ISR 只排空 RX FIFO到环形缓冲、填充 TX FIFO并退出；CRC、浮点缩放、
偏航角和标定全部在 1 ms 主循环任务中处理。

禁止在 ISR 中执行：

- OLED 全屏刷新；
- `printf`/`snprintf`；
- 阻塞 I2C；
- 长循环；
- PID 调参协议解析；
- 任何延时。

主循环始终调用 `App_Scheduler_Run()`，不存在 `WFI`、Sleep 或 Standby。

## 8. 状态机说明

| 状态 | 进入条件 | 电机行为 | 离开方式 |
|---|---|---|---|
| IDLE | 上电初始化完成 | STBY 禁止 | R/C/X |
| CALIBRATING | 收到 C | 电机禁止，持续采样极值 | E/X |
| RUNNING | 收到 R | 正常闭环控制 | S/X/长时间丢线 |
| LINE_LOST | 丢线超时 | 电机禁止 | R/S/X |
| STOPPED | 收到 S | 电机禁止 | R/C/X |
| FAULT | 收到 X | 紧急停机 | 当前未开放运行时恢复，需复位或后续新增明确恢复接口 |

上电不会自动启动，避免下载程序后车辆突然运动。

## 9. 参数和单位约定

### 9.1 电机指令

- 类型：`int16_t`；
- 范围：`-1000~+1000`；
- 单位：千分比 PWM；
- 正值：逻辑前进；
- 负值：逻辑后退；
- 实际物理方向由左右极性宏修正。

### 9.2 速度和距离

- 轮速：mm/s；
- 累计距离：mm；
- 电机转速显示：RPM；
- 控制周期：秒，例如 `0.005f`。

### 9.3 循迹位置

- 归一化强度：`0~1000`；
- 位置：约 `-1.0~+1.0`；
- 置信度：`0.0~1.0`。

### 9.4 方向控制输出

方向 PID 输出不是直接 PWM，而是左右轮之间的速度差，单位为 mm/s。这样方向环
和速度环可以分别调节，也更容易适应电池电压变化。

## 10. 必须替换的占位参数

以下参数目前不能视为实车定值：

```c
CAR_WHEEL_DIAMETER_MM
CAR_ENCODER_COUNTS_PER_WHEEL_REV
CAR_MOTOR_LEFT_POLARITY
CAR_MOTOR_RIGHT_POLARITY
CAR_ENCODER_LEFT_POLARITY
CAR_ENCODER_RIGHT_POLARITY
CAR_LINE_ADC_REVERSE_ORDER
CAR_LINE_BLACK_IS_LOW_RAW
CAR_LINE_DETECT_SUM_MIN
CAR_LINE_ELEMENT_THRESHOLD
所有 PID 参数
```

根据 MG513 铭牌参数，`CAR_ENCODER_COUNTS_PER_WHEEL_REV` 当前按
`13 PPR × 30 减速比 × AB 四倍频 = 1560` 推导。该值已经比占位值更合理，但厂家
对 PPR 的定义可能不同，仍必须用 QEI 手转车轮一圈实测确认。

## 11. UART 调试约定

控制命令：

| 字符 | 功能 |
|---|---|
| R/r | 开始运行 |
| S/s | 正常停止 |
| X/x | 紧急停止并进入 FAULT |
| C/c | 开始传感器标定 |
| E/e | 结束传感器标定 |
| G/g | 当前 IMU 相对偏航角清零 |
| I/i | 开始 IMU 静止角速度零偏标定 |

CSV 字段：

```text
time_ms,state,line_position_x1000,left_speed_mm_s,right_speed_mm_s,
left_pwm_permille,right_pwm_permille,line_detected,imu_valid,
yaw_degrees_x10,gyro_z_dps_x10,imu_crc_errors
```

这里指的是通过 HC-05 给手机输出 CSV 的调试 UART。`bsp_uart.c` 已实现 512 字节
TX、256 字节 RX 环形缓冲、FIFO 中断搬运和错误统计。用户已在 `empty.syscfg` 配置
`HC05_UART = UART1`、PB4 TX、PB5 RX、115200 8N1、FIFO 和 RX `>= 1 entry`
中断，因此 `CAR_HC05_UART_READY=1`。IMU 独占 UART0，已经在 `bsp_imu_uart.c`
实现独立环形缓冲和中断服务。两路串口不能共用同一个外设实例；调试数据队列满时
允许丢帧，控制任务绝不能等待发送完成。

PB4 在板卡资料中同时是 PWM 候选，后续给 TB6612 分配 PWM 时必须避开它；PB5 位于
下方未焊接接口区域，需要确认实物引出方式。当前分配已经避开 PA18 的默认 BSL 风险。

## 12. 已完成的验证

完成日期：2026-07-23。

验证内容：

- 24 个 C 源文件使用 TI Arm Clang 编译成功且 `-Wall -Wextra` 零告警；
- 打开 `-Wall -Wextra`；
- 使用 MSPM0G3507 的设备宏；
- 与现有 SysConfig 生成对象、启动文件、DriverLib 和 libc 完整链接；
- 主应用已接入 IMU，但尚未调用 OLED；验证镜像约 11504 字节代码、967 字节 BSS；
- 强制打开 `CAR_IMU_UART_READY=1` 并用 MSPM0G3507 UART0/IRQ 宏编译了真实
  DriverLib 分支，确认环形缓冲、中断入口和 FIFO API 均可编译、完整链接；启用
  硬件分支后的验证镜像约 12056 字节代码、1063 字节 BSS；
- 协议向量验证得到配置命令 CRC=`0x00AD`，线上顺序=`AD 00`；正反方向
  `int16_t` 角度回绕增量分别验证为 `+1/-1`；
- 额外使用 OLED 初始化、字库、浮点显示、圆弧、Printf 和整屏刷新做了强制链接
  探针，约 22144 字节代码、1541 字节 BSS，证明 OLED 的数学库和格式化依赖完整；
- HC-05 UART 的 `READY=0` 安全分支零告警编译通过；临时映射 UART1 后，真实
  RX/TX FIFO、中断环形缓冲和 `UART1_IRQHandler` 分支也完整编译链接，且不与
  IMU 的 `UART0_IRQHandler` 冲突；模拟启用后的镜像约 13072 字节代码、
  1851 字节 BSS；
- 在用户加入 HC-05 SysConfig 之前，曾对 `IMU READY=1 / HC-05 READY=0` 配置执行
  原工程增量构建，
  `bsp_uart.c`、`bsp_imu_uart.c`、`app_debug.c` 和整车最终链接均成功；
- 八路模拟 ADC 驱动分别在 `CAR_LINE_ADC_READY=0` 安全分支、临时 ADC0/ADC1 宏
  映射后的 `READY=1` 分支，以 TI Arm Clang、`-Wall -Wextra -Werror` 编译通过；
- 建立独立验证 SysConfig，确认推荐八路 ADC、现有 UART0/UART1、SWD、预留
  I2C1/PB2/PB3 和 UART2/PB15/PB16 可以同时生成且无 PinMux 冲突；
- 使用该验证配置实际生成的 `LINE_ADC0/1` 宏再次编译真实 DriverLib 分支成功；
- ADC ISR 只置完成标志，主任务在两组均完成后提交；实现中没有阻塞等待或延时；
- 用户已在原工程正式保存 `LINE_ADC0/ADC0`、`LINE_ADC1/ADC1`、`IMU_UART/UART0`
  和 `HC05_UART/UART1`；`CAR_LINE_ADC_READY=1` 后执行 Clean 全量构建并链接成功；
- 最终 map 明确包含来自 `bsp_line_adc.o` 的 `ADC0_IRQHandler`、`ADC1_IRQHandler`，
  以及互不冲突的 `UART0_IRQHandler`、`UART1_IRQHandler`；
- 用户已正式保存 `TICK_TIMER/TIMG0`：BUSCLK/1、Load=31999、Periodic、1 ms、
  ZERO 中断、SysConfig 不自动启动；`BSP_Time_Init()` 负责清状态、开 NVIC 和启动；
- TIMG0 接入后再次执行 Clean 全量构建成功，最终 map 中 `TIMG0_IRQHandler` 明确
  来自 `bsp_time.o`，并与 ADC0/ADC1/UART0/UART1 四个已有 ISR 同时存在；
- 2026-07-24 用户正式保存 `OLED_GPIO`，磁盘 `.syscfg` 明确为 PA12/PA13、Initial
  Set、Hi-Z Enable；使用正式完整配置重新运行 SysConfig 1.28.0 验证和生成成功；
- 生成结果明确包含 `OLED_GPIO_PORT`、`OLED_GPIO_SCL_PIN`、`OLED_GPIO_SDA_PIN`，且
  两脚均为 `DL_GPIO_HIZ_ENABLE`、无内部电阻、低驱动强度；
- 启用 `CAR_OLED_SOFT_I2C_READY=1` 后，`app_debug.c`、`app_scheduler.c`、
  `bsp_oled_soft_i2c.c`、`oled.c` 已使用正式生成头和器件选项完成 TI Arm Clang
  单文件编译检查；随后 9 个代码/文档文件已通过 SHA-256 核对同步到原工程；
- OLED 接入后执行 `gmake clean` 与 `gmake -j4 all`，SysConfig 重新生成、24 个应用
  C 源文件编译和最终链接全部成功；最终 ELF 为 322352 字节，map 报告 Flash 实际
  使用 `0x61D0`（25040 字节）、SRAM 区域使用 `0x0B55`（2901 字节）；
- 最终 map 同时包含 `ADC0_IRQHandler`、`ADC1_IRQHandler`、`TIMG0_IRQHandler`、
  `UART0_IRQHandler`、`UART1_IRQHandler`，以及 `App_Debug_OLEDTask`、`OLED_Init`、
  `OLED_UpdateArea` 和四个软件 I2C BSP 函数；
- 真实原工程构建已触发 SysConfig 重新生成并完整链接成功；生成结果明确为
  `HC05_UART_INST=UART1`、`UART1_IRQHandler`、RX=GPIOB/PB5、TX=GPIOB/PB4，
  115200 baud、FIFO 和 RX one-entry threshold；
- 链接映射中 `UART0_IRQHandler`（IMU）与 `UART1_IRQHandler`（HC-05）同时存在，
  地址不同且无重复定义；
- 没有未解析符号和重复定义；
- 没有 MCU 休眠或低功耗入口。

这项验证只能证明软件结构和链接关系正确，不能代替示波器、电机台架和赛道实测。

## 13. 还差什么

### 第一优先级：让基础硬件产生可信数据

1. 在 `empty.syscfg` 继续创建双 PWM、方向 GPIO和双 QEI；`TICK_TIMER/TIMG0`、
   `LINE_ADC0/1`、IMU UART0 和 HC-05 UART1 已正式分配；
2. ADC 按 PA25、PA24、PA22、PA15、PA17、PB17、PB18、PB19 从最左到最右配置；
3. 保留 I2C1/PB2/PB3 和 UART2/PB15/PB16，不再把它们分配给普通 GPIO；
4. 填写所有 BSP TODO；
5. 按传感器最左到最右确认八个 MEM 顺序、黑白电压范围，并断开 PA22 的 J16 负载；
6. 测量编码器每车轮一圈实际计数；
7. 验证左右电机和编码器极性；
8. 用串口/Watch 确认时间、灰度完整帧计数和编码器数据连续更新。

### 第二优先级：完成基础闭环

1. 架空车轮测试每个电机的正反转；
2. 单独调左轮速度 PI；
3. 单独调右轮速度 PI；
4. 让车辆在无循迹控制时稳定直行；
5. 完成黑白标定并观察归一化值；
6. 低速调循迹 PD；
7. 再逐步增加基础速度和弯道减速强度。

### 第三优先级：比赛功能

1. 根据赛题定义十字、直角、环岛、坡道和停车线；
2. 为每种元素增加明确状态和超时保护；
3. 加入电池电压、堵转和传感器超时保护；
4. 上板验证 IMU 方向、CRC、零偏和角度回绕后，再用于定角转向；
5. 最后加入 OLED 菜单和 Flash 参数保存。

## 14. 当前已知限制

- MG513 标称堵转电流约 2.8 A，明显高于 TB6612 常用约 1.2 A/通道连续能力；
  虽然峰值规格可能短时覆盖，但堵转会快速发热，必须避免长时间堵转并考虑硬件保护；
- PWM/GPIO/编码器 BSP 已接入真实资源，但尚未完成电机和编码器台架验证；上电默认
  仍保持 STBY 低、方向低、PWM 0%，不会自动运行；
- 八路 ADC 左到右 0～7 和 OLED 显示已经实物确认；黑白极性、动态标定和电机干扰
  工况仍需验证；
- 默认 PID 未经实车调节；
- 编码器速度滤波系数固定在源码中；
- 循迹路口判断只是基于活跃通道数量的初步启发式；
- 丢线恢复只使用最后方向，没有结合 IMU；
- FAULT 清除策略较简单，后续可要求长按按键或明确复位命令；
- HC-05 已分配 UART1/PB4/PB5且完整构建通过，但尚未连接实物验证透传收发；
- PB4 已被 HC-05 TX 占用，后续 TB6612 PWM 不能再选 PB4；PB5 的物理引出需确认；
- 八路模拟输出尚未上板测量；每路都必须限制在 0~3.3 V，不能把 5 V 模拟量直接
  输入 MSPM0 ADC；
- 推荐 ADC 方案只有 PA22 涉及板载光线传感器，使用前必须确认/断开 J16；
- 标定结果只保存在 RAM，上电后丢失；
- OLED 已按 128×64 SSD1306/0x3C 移植，并在 PA12/PA13 上实物显示正常；CPU 改为
  80 MHz 后位延时已自动换算，仍需复测显示稳定性和 SCL 波形；
- OLED 使用同步软件 I2C，完整 1 KiB 刷新不能放进实时控制路径；当前每 100 ms
  只更新一页，正式竞速前仍要测量单页耗时并评估是否进一步降频或关闭显示；
- 串口 IMU 已配置 UART0、PA10 TX、PA11 RX，但尚未上板确认 J21/J22 路由、CRC、
  零偏和物理方向；
- 没有单元测试框架，验证主要为编译、链接和后续硬件测试。

## 15. 下一位开发者的推荐入口

首次接手时建议按以下顺序阅读：

1. `PROJECT_HANDOFF.md`：了解整体状态和工程方法；
2. `PIN_ALLOCATION_RECOMMENDATION.md`：按已验证方案配置八路 ADC，并保留 I2C/UART；
3. `BOARD_PINOUT.md`：查 LaunchPad 引脚、跳线、冲突和当前占用；
4. `CAR_PORTING.md`：完成硬件资源；
5. `config/car_config.h`：核对车辆参数；
6. `config/board_config.h`：记录接线；
7. `bsp/bsp_*.c`：完成实际 DriverLib 调用；
8. `app/app_car.c`：理解整车数据流；
9. `drivers/imu.c` 和 `bsp/bsp_imu_uart.c`：确认 IMU 协议和中断入口；
10. `control/line_control.c` 和 `control/speed_control.c`：开始调参；
11. `drivers/line_sensor.c`：验证传感器方向和位置符号。

## 16. 交接验收标准

只有满足以下条件，才应把“基础循迹功能”标记为完成：

- 上电电机不误转；
- 1 ms 时间连续且无明显抖动；
- 两轮逻辑正速度都对应车辆前进；
- 两个编码器逻辑正计数都对应车辆前进；
- 车轮转一圈的计数误差在可接受范围；
- 每路循迹传感器黑白跨度足够且无明显坏道；
- `position` 左负右正与控制方向一致；
- 左右速度闭环分别稳定；
- 低速循迹不持续振荡；
- 丢线超时能够可靠停机；
- 调试 UART、IMU UART 或 OLED 故障不会造成永久等待；OLED 单页同步传输必须实测耗时；
- 运行代码中仍没有阻塞延时和低功耗入口。

## 17. 工程做事方法与后续协作规范

这一节记录本项目采用的工作方法。目的不是规定个人写法，而是让下一位开发者在
缺少上下文、任务中断或更换硬件时，仍能沿着同一套证据链继续推进，不把“能编译”
误认为“已经能比赛”。

### 17.1 先区分事实、推导、选择和待验证项

开始实现前，先把信息分成四类：

| 类型 | 例子 | 处理方式 |
|---|---|---|
| 已知事实 | MG513 13 PPR、减速比 30、IMU 500 Hz | 原样记录来源，不擅自改写 |
| 软件推导 | `13×30×4=1560 count/rev` | 写出公式和前提，保留实测复核项 |
| 设计选择 | TX 队列 512 B、控制周期 5 ms | 说明为什么这样选以及改变后的影响 |
| 硬件未知 | TB6612 引脚、ADC 顺序、HC-05 第二 UART | 使用宏和 `READY` 开关，不猜引脚 |

只要一个结论仍依赖实物，就明确写成“待上板验证”，不使用“已经完成”掩盖不确定性。

### 17.2 先读现状，再做最小范围修改

每次接手应先检查：

1. 当前源文件、`empty.syscfg` 和自动生成宏；
2. CCS 实际纳入构建的源文件；
3. 已经启用的外设、中断入口和引脚；
4. 工作区里是否存在用户未提交或未交接的修改；
5. 交接文档是否有落后于代码的描述。

修改只覆盖本任务需要的范围。SysConfig 负责 PinMux、时钟和静态外设参数；BSP
负责 DriverLib 和中断；驱动负责设备协议；控制层不直接访问寄存器。不要为了修一处
UART 顺手重排整个工程，也不要覆盖与本任务无关的用户修改。

### 17.3 分层方向必须单向

本工程坚持以下依赖方向：

```text
app -> control/drivers -> bsp -> SysConfig/DriverLib -> hardware
```

- `app/` 只组织状态和任务；
- `control/` 只处理控制数学和单位；
- `drivers/` 处理器件语义、协议、诊断和数据有效性；
- `bsp/` 隔离 MSPM0 外设和中断；
- `config/` 集中记录参数、极性、资源别名和 `READY` 开关；
- 自动生成文件不手改。

这样换引脚通常只改 SysConfig 或 `board_config.h` 别名，换传感器只改驱动，不需要
重写 PID 和整车状态机。

### 17.4 先定义实时性契约，再写函数

电赛控制代码中，每个函数都要先回答：

- 能否在 ISR 调用；
- 是否会等待硬件；
- 最坏执行时间是否有界；
- 队列满、设备掉线或数据错误时怎样退化；
- 错误会不会阻止电机控制继续运行。

本工程的默认原则是：

- ISR 只搬运数据、确认中断、更新时间戳；
- CRC、浮点、格式化和状态机留在主循环；
- 通信发送只入队，空间不足返回 `false`；
- 遥测可以丢帧，控制数据不能被遥测阻塞；
- 不使用 `WFI`、Sleep、Standby 或 MCU 低功耗等待；
- 不在 1 ms/5 ms 实时任务里整屏刷新 OLED；
- 不使用无超时的等待循环。

### 17.5 用安全占位分支隔离未配置硬件

硬件资源未确定时保留：

```c
#define CAR_xxx_READY (0U)
```

`READY=0` 分支必须仍能编译、链接并安全返回；只有 SysConfig 已生成真实实例、引脚
无冲突且宏名称核对完成后，才改为 `1U`。这能保证软件模块可以提前完成，同时避免
下载后误转电机、误开中断或引用尚不存在的自动生成符号。

### 17.6 注释解释“为什么”，接口表达“怎么用”

代码注释重点写：

- 电气或协议限制；
- 并发关系和生产者/消费者；
- 为什么选择某个缓冲长度、阈值或超时；
- 失败策略和安全后果；
- 哪些参数必须实测。

避免逐行翻译语法。接口命名应表达行为，例如 `TryWrite` 明确表示可能因队列满而
失败；单位写进变量或宏名，例如 `_MS`、`_HZ`、`MmS`、`Permille`，避免裸数字和
隐式单位换算。

### 17.7 验证分层，结论也分级

完成度按以下证据等级报告：

1. **静态检查通过**：接口、数组边界、单位和中断命名已检查；
2. **编译通过**：目标编译器和真实设备宏下零错误；
3. **链接通过**：整车镜像无未解析符号和重复中断；
4. **SysConfig 生成通过**：真实实例、PinMux、IRQ 和 Makefile 已生成；
5. **台架验证通过**：示波器、逻辑分析仪或架空车轮测得正确；
6. **实车验证通过**：负载、干扰、赛道和异常工况下满足要求。

必要时分别验证 `READY=0` 和强制 `READY=1` 两个分支。强制映射实例的编译链接
只能证明 API 和符号关系正确，不能代替真实 PinMux 和接线验证。

### 17.8 每次交接必须回答五个问题

1. 这次具体修改了哪些文件和接口；
2. 哪些内容已经完成，完成到哪一级证据；
3. 哪些内容还没做，为什么没做；
4. 下一步应该按什么顺序操作；
5. 出问题时应观察哪些计数、波形或状态量。

文档中的“当前状态”必须随代码一起更新。任务被网络或工具中断后，先重新读取实际
文件、生成宏和最近构建结果，再从断点继续；不要凭聊天记忆重做已经完成的修改。

### 17.9 引脚分配的固定流程

1. 先看 `BOARD_PINOUT.md` 中的板级占用、跳线和接口位置；
2. 在一张资源表里保留 SWD、已用 UART、供电和特殊启动脚；
3. 给必须使用特定复用的外设先选引脚，例如 QEI、PWM、UART；
4. 普通 GPIO 最后分配；
5. 让 SysConfig 验证同一外设实例和 PinMux 是否成立；
6. 保存后核对生成的 `*_INST`、`*_PIN`、`*_IRQN` 和 `*_IRQHandler`；
7. Clean + Build；
8. 最后按信号逐项上板测量。

板卡图片上的“UART/PWM/ADC/SPI”等文字是接口常用角色，不等同于芯片完整复用表；
Excel 中的“可以使用”也只表示板级条件允许。最终能否组合为同一个外设实例，必须
以当前封装下 SysConfig 的无冲突结果为准。

## 18. 2026-07-25 最终电机、编码器和 80 MHz 接入记录

### 18.1 本轮已经完成的代码

- `config/board_config.h`：正式启用 `CAR_TB6612_READY=1`、
  `CAR_ENCODER_READY=1`，把 TIMG12、五根 TB6612 GPIO、TIMG8 QEI 和右编码器
  GPIOB 生成宏集中映射为 `CAR_*` 别名；
- `bsp/bsp_tb6612.c`：实现安全初始化、STBY、两组方向脚和 0～1000 千分比 PWM；
- `bsp/bsp_encoder.c/.h`：实现左轮 16 位 QEI 软件扩展、右轮 PB8/PB9 双边沿 AB
  查表、共享 `GROUP1_IRQHandler()` 分派、原子复位和诊断快照；
- `app/app_debug.c`：OLED 增加电机/编码器诊断画面，显示累计/周期计数、轮速、
  速度目标、电机目标/实际占空比、使能、右轮 IRQ 和非法跳变；
- `bsp/bsp_board.c`：移除已经过期的重复启动 TODO，明确每个 BSP 独立负责启动；
- OLED 软件 I2C 延时改为 `CPUCLK_FREQ/800000`，80 MHz 下自动得到 100 cycles。

### 18.2 已核对的正式资源

| 功能 | 最终配置 |
|---|---|
| 主时钟 | MCLK/CPUCLK 80 MHz，ULPCLK 40 MHz |
| 左 PWM | TIMG12 CCP0，PB13，20 kHz |
| 右 PWM | TIMG12 CCP1，PA31，20 kHz |
| TB6612 方向/STBY | PB0、PB1、PB12、PB20、PA28，均初始低 |
| 左编码器 | TIMG8 QEI，PB6/PB7，LOAD=65535 |
| 右编码器 | GPIOB PB8/PB9，双边沿，Group 1 共享中断 |
| 理论计数 | `13 PPR × 30 × 4 = 1560 count/输出轴一圈` |

TIMG12 实际使用 80 MHz，不是 ULPCLK 40 MHz，所以 Period=4000 才是 20 kHz。
TIMG0 和两个 UART 使用 40 MHz，SysConfig 已分别重算为 1 ms 和约 115190.78 baud。

### 18.3 证据等级与仍未完成内容

当前 SysConfig CLI 已生成并确认真实宏、PinMux、80 MHz 时钟树和 GROUP1 IIDX。
2026-07-25 已把本轮 10 个预期文件同步至正式工程，并在正式 `Debug` 目录执行：

```text
gmake clean
gmake -j4 all
```

构建退出码为 0，没有编译器或链接器 Warning/Error，成功生成新的 `.out`、`.hex` 和
`.map`。Clean 后的正式 `ti_msp_dl_config.h` 已确认：

```text
CPUCLK_FREQ                     = 80000000
MOTOR_PWM_INST_CLK_FREQ         = 80000000
TICK_TIMER_INST_LOAD_VALUE      = 39999
IMU_UART_INST_FREQUENCY         = 40000000
HC05_UART_INST_FREQUENCY        = 40000000
```

map 中已确认 `GROUP1_IRQHandler`、`ADC0_IRQHandler`、`ADC1_IRQHandler`、
`TIMG0_IRQHandler`、`UART0_IRQHandler`、`UART1_IRQHandler`，以及全部 TB6612/
Encoder BSP 函数均由真实对象文件提供。源码调用扫描未发现 `__WFI`、Sleep、STOP、
STANDBY 或 SHUTDOWN 入口。本轮证据等级已达到“SysConfig 生成、目标编译、链接通过”；
尚不能替代下面的台架和实车验证。

上板仍待完成：

1. 不接电机主电源，测 STBY=低、四根方向线=低、两路 PWM 初始 0%；
2. 明确启动后测 PB13/PA31 均为 20 kHz，并核对占空比；
3. 确认编码器 A/B 高电平不超过 3.3 V；
4. 手转左轮和右轮，检查 OLED 计数连续、方向正确、ERR 不增长；
5. 每只输出轴完整转一圈，确认是否约 1560 count；
6. 车轮悬空后再接电机主电源，以低占空比逐轮开环测试；
7. 最后才进入速度 PID 和循迹联调。

任何实物方向相反，优先修改 `CAR_MOTOR_LEFT/RIGHT_POLARITY` 或
`CAR_ENCODER_LEFT/RIGHT_POLARITY`，不要在控制算法各处散落负号。

## 19. 主逻辑功能启用方式

现有程序并不是只显示 OLED：`App_Car_Init()` 与 `App_Scheduler_Init()` 已经默认启用
所有不造成上电误跑的后台链路，包括 IMU、八路 ADC、循迹位置解算、编码器、HC-05、
OLED 和状态保护。编码器在 IDLE 也持续更新，循迹 ADC 在 IDLE 也持续采样；只有
`LineControl → VehicleMixer → SpeedControl → Motor` 的闭环输出要求状态为 RUNNING。

正常使用时通过 HC-05 单字符命令控制：

```text
C：进入八路传感器动态标定，电机禁止
E：结束标定并保留 RAM 极值
R：进入 RUNNING，启动完整循迹和速度闭环
S：正常停止并拉低 STBY
X：紧急停止并锁存 FAULT
G：当前 IMU 偏航角清零
I：开始 IMU 静止零偏标定
```

`empty.c` 顶部新增两个默认注释的上电选项：

```c
/* #define CAR_POWER_ON_START_CALIBRATION */
/* #define CAR_POWER_ON_START_TRACKING    */
```

取消第一行注释会在初始化后自动进入传感器标定，电机仍然禁止；取消第二行会在初始化
后自动执行 `App_Car_Start()`，立即释放 TB6612 STBY 并进入完整闭环。两项同时启用会
由预处理器报错，避免含糊的上电状态。默认保持两行注释，满足“上电绝不自动运行”。

不要把 `LineSensor_Sample()`、`Encoder_Update()`、`Motor_Update()` 或 OLED 刷新再次
手工塞进 `while(1)`；这些调用已经由调度器按固定周期执行，重复调用会改变采样率、
滤波和斜坡行为。

本功能选择区已经用 TI ARM Clang 和正式 `device.opt` 分别验证：默认 IDLE、自动标定、
自动循迹三个合法分支均编译通过；同时定义两项时按设计触发互斥 `#error`。同步正式
工程后执行 `gmake -j4 all`，最终编译、链接和 HEX 生成退出码为 0。

## 20. 实车循迹转向极性修正

实测灰度模块黑线约 100、白底约 4095，因此 `CAR_LINE_BLACK_IS_LOW_RAW=1` 正确：
归一化后黑线接近 1000、白底接近 0，不应反转 ADC 黑白逻辑。实测黑线在车辆左侧时，
控制层生成左/右目标速度约 87/309，说明位置计算和差速混合关系有效；左右编码器、
实体电机对应和编码方向也已由用户确认。

针对最终底盘/传感器安装产生的整车转向极性，新增：

```c
#define CAR_LINE_STEERING_POLARITY (-1.0f)
```

该宏在 `line_control.c` 中统一乘到正常循迹 PID 输出和丢线搜索输出。`-1.0f` 翻转当前
转向，`+1.0f` 恢复原软件约定；不要用 `CAR_LINE_BLACK_IS_LOW_RAW`、电机极性或编码器
极性代替它。宏乘在完整 PID 输出之后，因此 P/I/D 符号保持一致。

## 21. 2026-07-26 IMU 偏航角速度内环

### 21.1 为什么普通循迹不直接锁定绝对偏航角

此前 IMU 已经在 1 ms 任务中接收、校验并发布 `yawDegrees` 和 `gyroZDps`，但控制链为：

```text
八路灰度位置
  -> LineControl 灰度方向 PID
  -> VehicleMixer 左右差速
  -> 两轮速度 PID
  -> Motor/TB6612
```

因此 IMU 当时只参与诊断，没有反馈到转向。不能简单增加“目标角度=0°”的全局角度
PID：车辆进入弯道后，赛道方向本来就会改变，绝对角度环会试图把车拉回起跑方向，
与灰度循迹正面冲突。

本轮改为适合连续赛道的级联结构：

```text
八路灰度位置
  -> 灰度方向 PID，得到 lineSteeringMmS（已验证的基础转向）
  -> 按 CAR_YAW_RATE_TARGET_GAIN 换算 targetYawRateDps
  -> IMU gyroZDps 角速度内环，得到有限的 correctionMmS
  -> finalSteering = lineSteering + correction
  -> VehicleMixer
  -> 左右轮速度 PID
  -> Motor/TB6612
```

累计 `yawDegrees` 仍然保留，并应在明确知道目标角度的状态中使用，例如：直角转弯、
定角转向、环岛分段、短时丢线保持。届时可以再加“角度外环输出目标角速度”，复用
本轮已经完成的角速度内环。

### 21.2 修改的代码

- `config/car_config.h`：新增角速度环开关、目标映射、目标限幅、PID、修正限幅和
  100 ms 渐入时间；明确 `CAR_IMU_YAW_POLARITY` 必须调成实体右转时读数为正；
- `control/line_control.c/.h`：在灰度方向 PID 后加入 IMU Z 轴角速度反馈；新增独立
  PID 状态、状态复位、无效数据回退、最终共同限幅以及 `LineControl_Status`；
- `app/app_car.c`：5 ms 控制任务把 `gyroZDps` 和“有效且未标定”状态传入方向环；
- `app/app_debug.c`：OLED 增加第三个 IMU/角速度环页面；HC-05 CSV 末尾增加角速度
  环是否激活、目标角速度、修正量和最终转向量。

当前首版参数：

```c
#define CAR_YAW_RATE_CONTROL_ENABLE        (1U)
#define CAR_YAW_RATE_TARGET_GAIN           (0.40f)
#define CAR_YAW_RATE_TARGET_LIMIT_DPS      (120.0f)
#define CAR_YAW_RATE_KP                    (0.50f)
#define CAR_YAW_RATE_KI                    (0.0f)
#define CAR_YAW_RATE_KD                    (0.0f)
#define CAR_YAW_RATE_CORRECTION_LIMIT_MM_S (80.0f)
#define CAR_YAW_RATE_ENGAGE_TIME_S         (0.10f)
```

这些是低速、保守的联调起点，不是实车最终参数。IMU 超时、CRC 导致数据超过 20 ms
未更新或正在执行 `I` 静止零偏标定时，角速度 PID 会复位，修正归零，车辆自动退回
此前已经正常工作的纯灰度循迹。最终转向仍受原来的 ±300 mm/s 限幅约束。

### 21.3 OLED 第三页字段

OLED 现在按“ADC/循迹 → 电机/编码器 → IMU/角速度环”轮换。第三页字段为：

```text
V       IMU 数据有效
A       角速度闭环本周期实际激活
CAL     正在执行静止零偏标定
Y       累计偏航角 ×10
G       实测 Z 轴角速度 ×10
T10     目标偏航角速度 ×10
C       IMU 产生的附加转向 mm/s
LS      灰度方向环基础转向 mm/s
OUT     最终转向 mm/s
BIAS10  陀螺仪零偏 ×10
EN      角速度修正渐入百分比
FR/CRC  有效帧和 CRC 错误
DROP/OVF 丢帧和 UART RX 溢出
```

HC-05 CSV 原 12 个字段保持顺序不变，并在末尾依次追加：

```text
yaw_rate_active,target_yaw_rate_x10,correction_mm_s,final_steering_mm_s
```

### 21.4 必须按顺序完成的上板验证

1. 先让车停止，观察第三页 `V=1`、`FR` 连续增长、`CRC/DROP/OVF` 基本不增长；
2. 车身保持完全静止，经 HC-05 发送 `I`，等待约 1 秒直至 `CAL` 从 1 回到 0；
3. 架空车轮或断开电机主电源，手动把车身向右旋转，确认 `G` 为正；向左为负；
4. 若方向相反，只把 `CAR_IMU_YAW_POLARITY` 从 `+1.0f` 改为 `-1.0f`；
5. 放在线中央静止观察：`T10` 接近 0，短时转动车身时 `C` 应与 `G` 反号，表示
   它在抵抗扰动；若同号则形成正反馈，禁止落地运行；
6. 低速落地后确认 `A=1`、`EN` 最终到 100，且 `OUT` 等于 `LS+C` 限幅后的结果；
7. 对比 `CAR_YAW_RATE_CONTROL_ENABLE=0/1` 的直线扰动和弯道表现，再开始调参。

调参顺序固定为：先确认方向，再只调 `CAR_YAW_RATE_KP`，然后调
`CAR_YAW_RATE_TARGET_GAIN`，最后才考虑极小的 `Ki`。首轮不要开 `Kd`，500 Hz 串口
角速度已经是直接速度反馈，额外微分很容易放大量化噪声。

### 21.5 当前证据等级和剩余工作

工作副本中的 `line_control.c`、`app_car.c`、`app_debug.c` 已使用正式工程相同的 TI ARM
Clang 5.1.1 LTS、`device.opt` 和 MSPM0 SDK 头文件独立编译，三个文件均零错误。此证据
只到“目标编译通过”；正式工程同步、完整链接以及 IMU 物理方向/闭环效果仍需在下一步
完成，不能把首版参数描述成已经实车整定。

### 21.6 正式工程同步与完整构建结果

2026-07-26 已将角速度闭环相关配置、方向控制、应用编排、OLED/HC-05 诊断和本交接
记录同步至正式工程，并在正式 `Debug` 目录依次执行：

```text
gmake clean
gmake -j4 all
```

结果为：SysConfig 重新生成成功、全部源文件编译成功、链接成功、Intel HEX 转换成功，
两个命令退出码均为 0。最新主要产物：

```text
empty_LP_MSPM0G3507_nortos_ticlang.out  380672 bytes  2026-07-26 21:55:39
empty_LP_MSPM0G3507_nortos_ticlang.hex   70404 bytes  2026-07-26 21:55:39
empty_LP_MSPM0G3507_nortos_ticlang.map   71102 bytes  2026-07-26 21:55:39
```

生成文件再次确认：

```text
CPUCLK_FREQ             = 80000000
IMU_UART_INST_FREQUENCY = 40000000
```

map 中存在正式链接的 `LineControl_Update`、`LineControl_GetStatus`、`IMU_GetData` 和
`IMU_GetDiagnostics`；源码扫描未发现 `__WFI`、Sleep、STOP、STANDBY 或 SHUTDOWN
调用。因此软件证据已达到“正式 SysConfig 生成、目标编译、完整链接、HEX 生成通过”。
仍待完成的是 21.4 所列的 IMU 实物通信、右转正方向、负反馈符号和首版参数实车整定。

## 22. 2026-07-26 HC-05 江协小程序 PID 在线调参

### 22.1 参考协议与移植原则

参考工程：

```text
C:\Epan\蓝牙模块教程资料\程序源码\
9-4 串口收发文本数据包-已修改为蓝牙串口功能
```

确认其线上格式为：

```text
[display,x,y,text]
[plot,y1,y2]
[key,name,action]
[slider,name,value]
```

参考 STM32 程序在 USART ISR 中拼接全局包并逐字节等待 TXE，且接收数组没有长度
保护。本工程只保留兼容的文本格式，不移植阻塞行为：HC-05 ISR 仍只搬运 FIFO 到
环形队列；`[`/`]` 状态机、字段拆分、十进制转换、PID 修改和格式化全部在 NoRTOS
主循环执行；发送使用 `BSP_UART_TryWrite()` 完整入队，空间不足立即失败。

### 22.2 新增文件和构建接入

新增：

```text
app/pid_debug.c
app/pid_debug.h
makefile.targets
```

`pid_debug.c/.h` 是独立编译单元，负责文本协议、PID 选择/修改、display、plot 和诊断
计数。根目录 `makefile.targets` 是 CCS 生成 Makefile 已预留的用户扩展入口：在 CCS
尚未 Refresh 并把新源文件写入 `app/subdir_vars.mk` 之前，将 `app/pid_debug.o` 安全
加入构建；若 CCS 已经自动发现该对象，`filter` 守卫会避免重复链接。没有手工修改
`Debug` 目录下任何自动生成文件。

同时修改：

- `config/car_config.h`：增加 PID 调试、CSV、plot 周期、包超时和增益上限宏；
- `app/app_debug.c/.h`：初始化新模块；把每个 RX 字节先交给括号协议；低频调用
  `PIDDebug_Task()`；保留原单字符命令；
- 默认关闭裸 CSV，避免无方括号 CSV 干扰手机小程序；需要电脑记录时可重新打开。

### 22.3 发送 API

没有全局覆盖 libc 的 `printf`，而是提供同样使用方式的非阻塞接口：

```c
PIDDebug_Printf("[display,0,0,Hello World]");
PIDDebug_Printf("[plot,%f,%f]", y1, y2);

PIDDebug_DisplayText(0, 0, "Hello World");
PIDDebug_Plot2(y1, y2);
```

原因是标准 `printf` 的逐字符重定向无法保证文本包原子性：TX 中途满时可能只发出
`[plot,1.2`，手机会永久等待 `]`。`PIDDebug_Printf()` 先在 192 字节有界缓冲区中用
`vsnprintf` 完整格式化，再一次性尝试入队；格式化截断或 TX 空间不足时整包拒绝。
它允许 `%f`，但只能用于 20/40 ms 低频调试任务，禁止 ISR 和 5 ms 控制任务调用。

### 22.4 key 和 slider 映射

按键动作兼容 `up`、`down`、`click`、`press`；滑条标签同时兼容参考例程的
`slider` 和用户常用的 `slide` 拼写。

```text
[key,1,up] 或 [key,line,down]   选择灰度方向 PID
[key,2,up] 或 [key,yaw,down]    选择 IMU 角速度 PID
[key,3,up] 或 [key,left,down]   选择左轮速度 PID
[key,4,up] 或 [key,right,down]  选择右轮速度 PID
[key,5,down] / [key,status,...] 主动刷新参数 display
[key,6,down] / [key,plot,...]   开关周期 plot
[key,7,down] / [key,reset,...]  清所选 PID 的积分/微分历史，不改增益

[slider,1,260.0] / [slide,kp,260.0] 修改所选 PID Kp
[slider,2,0.0]   / [slide,ki,0.0]   修改所选 PID Ki
[slider,3,6.0]   / [slide,kd,6.0]   修改所选 PID Kd
```

滑条值只接受普通十进制 `[-]123.456` 语法；实际增益限制为 0 到
`CAR_PID_DEBUG_GAIN_MAX`，拒绝负数、NaN、Inf、指数、非法尾随字符和超范围值。
调参只修改 RAM 中的 `PID_Controller`，掉电后恢复 `car_config.h` 编译默认值，没有
写 Flash，也不会触发 80 MHz 下的 Flash 操作注意事项。

### 22.5 与原单字符命令不冲突

`App_Debug_PollCommands()` 的顺序是：

```text
HC-05 RX byte
  -> PIDDebug_PushRxByte()
       -> 位于 [ ... ] 内：完整消费，绝不再解释字母
       -> 位于包外：返回 false
  -> 旧 R/S/X/C/E/G/I switch
```

因此 `[slider,1,260]` 中出现的 `r`、`i` 等字符不会触发 RUN 或 IMU 标定。只有手机
直接在方括号之外发送单字节 `R` 才会起跑。未闭合包 250 ms 超时自动退出；超长包
持续丢弃到 `]`，其尾部也不会泄漏到旧命令解析器。

### 22.6 默认显示与曲线

默认选择 LINE PID，每 1 秒重发三条 display，使手机晚于 MCU 上电连接也能看到：

```text
PID 名称
Kp / Ki
Kd / Plot开关
```

plot 默认每 40 ms（25 Hz）发送两条曲线：

| 所选 PID | y1 | y2 |
|---|---|---|
| LINE | 目标位置 0 | 灰度实际 position |
| YAW | 目标角速度 deg/s | IMU 实测角速度 deg/s |
| SPEED-L | 左轮目标 mm/s | 左轮实测 mm/s |
| SPEED-R | 右轮目标 mm/s | 右轮实测 mm/s |

配置宏：

```c
#define CAR_PID_DEBUG_ENABLE               (1U)
#define CAR_DEBUG_CSV_ENABLE               (0U)
#define CAR_PID_DEBUG_PLOT_DEFAULT         (1U)
#define CAR_PID_DEBUG_PLOT_PERIOD_MS       (40U)
#define CAR_PID_DEBUG_DISPLAY_PERIOD_MS    (1000U)
#define CAR_PID_DEBUG_PACKET_TIMEOUT_MS    (250U)
#define CAR_PID_DEBUG_GAIN_MAX             (10000.0f)
```

### 22.7 诊断和调参安全

`PIDDebug_GetDiagnostics()` 提供：正确闭合包、key、slider、格式错误、包溢出、包超时、
未知命令、拒绝值、TX 拒绝、格式化截断、当前 PID 和 plot 开关计数/状态。

在线调参必须架空或低速开始。建议顺序：

1. 选择某个 PID；
2. 先保持 Ki=0、Kd=0，只调 Kp；
3. 观察 plot 中目标与实测；
4. 再逐步增加 Ki 消除稳态误差；
5. 最后才增加少量 Kd；
6. 参数异常时发 `[key,7,down]` 只清历史，或停车后重新下载恢复编译默认值。

模块不会自动把 key 映射到起跑/停车，防止调参界面误触。车辆状态仍由明确的包外
`R/S/X/C/E/G/I` 管理。

### 22.8 正式工程 Clean Build 验证

2026-07-26 已把本节所列代码同步到正式工程，并在正式工程 `Debug` 目录依次执行：

```text
gmake clean
gmake -j4 all
```

第一次从干净状态构建时发现一个 CCS 用户扩展入口的依赖展开细节：虽然
`makefile.targets` 已把 `./app/pid_debug.o` 加入链接对象列表，但自动生成 Makefile 中
较早定义的 `all: $(OBJS)` 已经完成变量展开，导致链接阶段能够看到对象名，构建阶段
却没有先生成该对象。没有修改 `Debug/makefile`、`Debug/app/subdir_vars.mk` 或
`Debug/app/subdir_rules.mk` 等自动生成文件，而是在用户维护的 `makefile.targets` 中为
`all` 和最终 `.out` 目标显式补充 `./app/pid_debug.o` 依赖，并保留 `filter` 守卫避免
CCS Refresh 后重复加入对象。

修正后再次从干净状态完整构建，日志明确证明：

```text
Clean 删除 app\pid_debug.o、app\pid_debug.d、app\pid_debug.d_raw
Build 重新执行 Arm Compiler - building file: "../app/pid_debug.c"
最终链接命令包含 "./app/pid_debug.o"
SysConfig 生成成功
全部源文件编译成功
链接成功
Intel HEX 生成成功
clean 和 build 退出码均为 0
```

最终主要产物为：

```text
empty_LP_MSPM0G3507_nortos_ticlang.out  402660 bytes  2026-07-26 22:39:38
empty_LP_MSPM0G3507_nortos_ticlang.hex   79535 bytes  2026-07-26 22:39:38
empty_LP_MSPM0G3507_nortos_ticlang.map   76234 bytes  2026-07-26 22:39:38
app/pid_debug.o                           37836 bytes  2026-07-26 22:39:37
```

map 文件已确认存在 `PIDDebug_PushRxByte`、`PIDDebug_Task`、`PIDDebug_Init`、
`PIDDebug_Printf`、`PIDDebug_CheckTimeout` 和浮点格式化所需的 `vsnprintf`。部分短小公开
函数可能被 TI ARM Clang 内联，因此不要求每个包装函数都保留为独立链接符号。

源代码低功耗扫描未发现 `__WFI`、`enterSleep`、`enterSTOP`、`enterSTANDBY` 或
`enterSHUTDOWN` 调用。本次在线调参模块没有加入休眠、低功耗等待或阻塞式毫秒延时；
UART ISR 仍然只搬运字节，文本解析、浮点格式化、PID 参数修改和协议发送都在主循环
低频任务完成。

## 23. 2026-07-27 构建故障修复与运行链路复核

### 23.1 表面现象与真实首错

CCS 并行构建日志末尾显示 `oled_data.c`、`tb6612.c`、`oled.c` 均为
`Finished building`，随后只出现：

```text
gmake: Target 'all' not remade because of errors.
```

这些驱动不是故障源。使用 `gmake -j1 all` 重新构建后，完整首错为
`pid_debug.c` 找不到 `CAR_PID_DEBUG_GAIN_MAX`、plot/display 周期等宏。

### 23.2 根因和保留的实车参数

正式工程 `config/car_config.h` 当时被替换成了只有 107 行的较早版本，除 HC-05
调试宏外，还同时丢失：

```text
CAR_LINE_STEERING_POLARITY
CAR_YAW_RATE_CONTROL_ENABLE 及完整角速度环配置
CAR_PID_DEBUG_ENABLE 及完整方括号协议配置
```

修复没有直接用旧工作副本覆盖正式文件，而是做了合并，并保留正式文件中已经修改的
左右轮速度 PID：

```c
#define CAR_SPEED_LEFT_KP   (2.0f)
#define CAR_SPEED_LEFT_KI   (0.0f)
#define CAR_SPEED_LEFT_KD   (0.02f)
#define CAR_SPEED_RIGHT_KP  (2.0f)
#define CAR_SPEED_RIGHT_KI  (0.0f)
#define CAR_SPEED_RIGHT_KD  (0.02f)
```

### 23.3 同时发现的运行级问题

第一次修复配置后固件已经能够编译，但 map 只保留 `PIDDebug_Init`，没有
`PIDDebug_Task`、`App_Debug_PollCommands` 或 `App_Scheduler_Run`。进一步核对发现正式
`empty.c` 中主循环调用被注释成：

```c
// App_Scheduler_Run();
```

这种状态能够通过编译和链接，却会让程序在空 `while (1)` 中循环，导致 ADC、IMU、
5 ms 控制环、HC-05 RX/plot 和 OLED 周期刷新全部停止。现已恢复
`App_Scheduler_Run()`；用户主动启用的裸 `App_Car_Start()` 保持不变。

### 23.4 最终 Clean Build 证据

恢复调度器后再次执行完整 `gmake clean` 和 `gmake -j4 all`。SysConfig、全部源文件、
链接和 HEX 生成均成功，退出码为 0。最终产物：

```text
empty_LP_MSPM0G3507_nortos_ticlang.out  402660 bytes  2026-07-27 11:44:43
empty_LP_MSPM0G3507_nortos_ticlang.hex   79535 bytes  2026-07-27 11:44:43
empty_LP_MSPM0G3507_nortos_ticlang.map   76234 bytes  2026-07-27 11:44:43
app/pid_debug.o                           37836 bytes  2026-07-27 11:44:39
```

map 已确认最终运行链路包含：

```text
App_Scheduler_Run
App_Debug_PollCommands
App_Debug_Task
PIDDebug_Init
PIDDebug_PushRxByte
PIDDebug_CheckTimeout
PIDDebug_Printf
PIDDebug_Task
LineControl_Update
```

正式工程与工作副本的 `config/car_config.h`、`empty.c` SHA-256 均一致。若 CCS 编辑器
仍打开修复前的旧文件并提示“磁盘内容已被外部修改”，必须选择从磁盘重新加载；不要
保留旧编辑器内容后再保存，否则会再次覆盖这些配置和调度器调用。

## 24. 2026-07-27 HC-05 手动页面与相对角度闭环调试模式

> 本节记录当前最终状态。若本节与第 21、22、23 节中的旧默认值发生冲突，以本节和
> 当前源码为准。旧章节保留是为了说明功能演进和已解决故障，不代表当前运行配置。

### 24.1 本轮目标和最终结果

这轮修改的目标不是继续自动循迹，而是先把 IMU、蓝牙协议、角度环和双轮速度环单独
联调清楚。当前固件已经实现：

1. 暂时旁路八路灰度循迹方向环，ADC 采样与诊断仍然保留；
2. 手机滑条下发 `-180 deg ~ +180 deg` 的相对目标角；
3. 角度 PID 输出原地转向速度，再经过原有左右轮速度 PID 和 TB6612；
4. 手机 display 周期显示目标角、实测角、误差、角速度和角度 PID；
5. 手机 plot 默认绘制“目标角度/实测角度”；
6. OLED 不再自动切换诊断画面，改由 HC-05 `key 8` 手动翻页；
7. 支持从手机选择 LINE、YAW、ANGLE、左轮速度、右轮速度五个 PID，并在线修改
   `Kp/Ki/Kd`；
8. HC-05 UART 已从 115200 改为参考程序实际使用的 9600，IMU UART 仍保持 115200；
9. 全部接收 ISR 仍只搬运字节，协议解析、浮点格式化和参数修改仍在主循环低频任务；
10. 没有加入 WFI、Sleep、STOP、Standby 或阻塞式毫秒延时。

### 24.2 手机没有 display/plot 的首要根因

用户提供的江协蓝牙参考工程中，`Hardware/Serial.c` 明确配置：

```c
USART_InitStructure.USART_BaudRate = 9600;
```

原 MSPM0 工程中的 HC-05 UART 一度配置为 115200。若 HC-05 没有先进入 AT 模式并改写
其串口波特率，MCU 与模块之间波特率不一致，会同时造成：

- 手机收不到 `[display,...]`；
- 手机收不到 `[plot,...]`；
- MCU 也无法正确解析手机下发的 `[key,...]` 和 `[slider,...]`；
- OLED 上 UART 硬件错误计数可能增长。

当前最终串口分配为：

```text
IMU_UART  = UART0，PA10 TX，PA11 RX，115200 bit/s
HC05_UART = UART1，PB4  TX，PB5  RX，  9600 bit/s
```

SysConfig 生成结果已核对为 HC-05 `IBRD=260`、`FBRD=27`。手机与 HC-05 之间仍是无线
蓝牙连接；这里的 9600 指 MCU 与 HC-05 模块之间的 TTL UART 波特率。接线必须交叉：

```text
MCU PB4 / TX -> HC-05 RX
MCU PB5 / RX <- HC-05 TX
MCU GND       -- HC-05 GND
```

### 24.3 为什么使用相对角度 -180 deg ~ +180 deg

当前角度定义为：

```text
每次启动或执行清零时的车头方向 = 0 deg
目标为正                         = 向右转
目标为负                         = 向左转
允许下发范围                     = -180 deg ~ +180 deg
```

没有使用 `0 deg ~ 360 deg`，因为有符号角更容易直接表达左右方向；角度误差会折算到
`[-180 deg, +180 deg]`，因此跨越边界时始终选择较短的旋转方向。例如当前位置
`+170 deg`、目标 `-170 deg` 时，控制器使用约 `+20 deg` 的误差，而不是反向旋转
`340 deg`。

### 24.4 角度闭环的实际控制链

新增模块：

```text
control/angle_control.c
control/angle_control.h
```

实际数据链是：

```text
[slider,4,target]
        |
        v
ANGLE 角度 PID
        |
        | steeringMmS
        v
VehicleMixer_Mix(0, steeringMmS)
        |
        +--> 左轮目标 +steeringMmS
        +--> 右轮目标 -steeringMmS
                  |
                  v
        左右轮编码器速度 PID
                  |
                  v
        Motor 斜坡、限幅、极性
                  |
                  v
               TB6612
```

角度环没有直接写 PWM，也没有绕过已验证的电机和编码器层。这样既保留两轮速度一致性，
也继续受到 PWM 限幅、电机斜坡、STBY 和急停保护。IMU 无效或处于陀螺仪标定状态时，
`AngleControl_Update()` 输出 0，车辆不会在没有可信角度反馈时盲目旋转。

当前编译默认值集中在 `config/car_config.h`：

```c
#define CAR_CONTROL_MODE                   CAR_CONTROL_MODE_ANGLE_DEBUG
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
```

只有同时满足“角度误差不超过 2 deg”和“角速度不超过 5 deg/s”，并连续保持 200 ms，
才判定 `settled=true` 并把角度环输出归零。若外力再次使车身离开容差，闭环会重新纠正。

### 24.5 临时停用循迹的方法

模式由一个集中宏控制：

```c
#define CAR_CONTROL_MODE_LINE              (0U)
#define CAR_CONTROL_MODE_ANGLE_DEBUG       (1U)
#define CAR_CONTROL_MODE                   CAR_CONTROL_MODE_ANGLE_DEBUG
```

当前为 `CAR_CONTROL_MODE_ANGLE_DEBUG`，因此 5 ms 控制任务将前进速度固定为 0，只执行
原地定角；灰度 ADC 仍持续采样并可在 OLED 第 0 页观察，但灰度方向 PID 和丢线停车看门
不会参与电机控制。角度调试完成后，只需把最后一个宏改回：

```c
#define CAR_CONTROL_MODE CAR_CONTROL_MODE_LINE
```

然后 Clean Build，即可恢复“灰度位置外环 + IMU 角速度内环 + 双轮速度环”的循迹链。
原有循迹参数、标定值和方向极性均未删除。

### 24.6 江协小程序 key 映射

当前 ANGLE 调试模式默认选中 ANGLE PID。按键映射为：

| 手机命令 | 作用 |
|---|---|
| `[key,1,down]` 或 `[key,line,down]` | 选择灰度位置 LINE PID |
| `[key,2,down]` 或 `[key,yaw,down]` | 选择循迹角速度 YAW PID |
| `[key,3,down]` 或 `[key,left,down]` | 选择左轮速度 PID |
| `[key,4,down]` 或 `[key,right,down]` | 选择右轮速度 PID |
| `[key,5,down]` 或 `[key,angle,down]` | 选择原地定角 ANGLE PID |
| `[key,6,down]` 或 `[key,plot,down]` | 开关 plot 周期发送 |
| `[key,7,down]` 或 `[key,reset,down]` | 清所选 PID 的积分/微分历史，不改增益 |
| `[key,8,down]` 或 `[key,page,down]` | OLED 下一诊断页 |
| `[key,9,down]` 或 `[key,zero,down]` | 当前车头清零、目标归零并选中 ANGLE PID |
| `[key,status,down]` | 立即请求刷新手机 display |

代码兼容 `down/up/click/press` 四种动作。但 `key 6` 和 `key 8` 都是“收到一次合法包就
切换一次”，所以江协小程序按钮应只配置一次 `down`，不要同一按键同时发送 down 和
up，否则会开关两次或连翻两页，看起来像没有动作。

### 24.7 slider 映射和在线修改 PID 的实际代码路径

选择某个 PID 后，使用：

```text
[slider,1,value] 或 [slide,kp,value] -> 修改所选 PID 的 Kp
[slider,2,value] 或 [slide,ki,value] -> 修改所选 PID 的 Ki
[slider,3,value] 或 [slide,kd,value] -> 修改所选 PID 的 Kd
```

实现不是占位：`pid_debug.c` 先通过 `selected_pid()` 取得实际 PID 对象，再调用：

```c
PID_SetTunings(pid, value, pid->ki, pid->kd);  /* Kp */
PID_SetTunings(pid, pid->kp, value, pid->kd);  /* Ki */
PID_SetTunings(pid, pid->kp, pid->ki, value);  /* Kd */
```

可被选择和修改的五个对象分别来自：

```text
LineControl_GetPID()          -> 灰度位置环
LineControl_GetYawRatePID()   -> 循迹角速度环
AngleControl_GetPID()         -> 原地定角环
SpeedControl_GetLeftPID()     -> 左轮速度环
SpeedControl_GetRightPID()    -> 右轮速度环
```

目标角度单独占用 slider 4：

```text
[slider,4,-90]
[slide,angle,90]
[slides,target,180]
```

协议标签同时兼容 `slider/slide/slides`。目标角允许负值并严格限制在 -180~+180；PID
增益只允许 `0~CAR_PID_DEBUG_GAIN_MAX`。在线修改只作用于 RAM，复位或重新下载后恢复
`car_config.h` 中的编译默认值。

### 24.8 手机 display、plot 和发送带宽

ANGLE 页面每 500 ms 周期发送以下五条 display：

```text
[display,0,0,MODE:ANGLE V:... S:...]
[display,0,20,T:... Y:...]
[display,0,40,E:... G:...]
[display,0,60,K:Kp/Ki/Kd]
[display,0,80,RX:... TXR:...]
```

字段含义：

```text
V   = IMU 数据有效
S   = 角度已经稳定到位
T   = 目标相对角度 deg
Y   = IMU 实测相对偏航角 deg
E   = 已折算到 [-180,+180] 的角度误差 deg
G   = IMU Z 轴角速度 deg/s
K   = 当前 ANGLE PID 的 Kp/Ki/Kd
RX  = MCU 已收到并解析的完整方括号包数
TXR = UART TX 队列空间不足导致的发送拒绝数
```

ANGLE 默认 plot 为：

```text
y1 = 目标角度 T
y2 = 实测角度 Y
```

HC-05 改为 9600 后，发送周期已降低为：

```c
#define CAR_PID_DEBUG_PLOT_PERIOD_MS       (100U)  /* 10 Hz */
#define CAR_PID_DEBUG_DISPLAY_PERIOD_MS    (500U)  /* 2 Hz  */
```

约 960 byte/s 是 9600 bit/s、8N1 UART 的理论有效上限。当前 display 与 plot 的平均
负载约 560 byte/s，留有必要余量。不要把 plot 恢复到 25 Hz 后仍同时高频发送五条
display，否则 TX 队列会拥塞，`TXR` 会持续增长。

### 24.9 OLED 手动页面和“分段刷新”的含义

OLED 逻辑页面已经取消自动轮换：

```text
第 0 页 = 八路 ADC / 循迹诊断
第 1 页 = 电机 / 编码器诊断
第 2 页 = IMU / ANGLE 诊断
```

ANGLE 模式上电默认显示第 2 页。只有收到 `[key,8,down]` 或 `[key,page,down]` 才切换
逻辑页面。

需要区分“逻辑页面切换”和“SSD1306 物理传输”。逻辑页面已经完全手动；但每次 OLED
任务仍只发送一条 8 像素高的 SSD1306 硬件页。原因是 128x64 软件 I2C 整屏需要发送
1024 个显存字节，实测量级会占用二十多毫秒，足以破坏 5 ms 角度控制节拍。当前每
40 ms 发送一条硬件页，八条约 0.32 s 完成一整屏，运动控制实时性显著更安全。

ANGLE OLED 页字段为：

```text
ANGLE S / V / SET       车辆状态、IMU 有效、是否到位
T10 / Y10               目标角、实测角，均放大 10 倍
E10 / G10               误差、角速度，均放大 10 倍
OUT / CNT               角度环转向输出、稳定累计周期
KP100 / KI              Kp 放大 100 倍、Ki 放大 100 倍
KD100 / TXR             Kd 放大 100 倍、UART TX 拒绝数
FR / CRC                IMU 有效帧数、CRC 错误数
PKT / UERR              完整蓝牙包数、UART 硬件错误数
```

手机每发一次完整 `[key,...]` 或 `[slider,...]`，`PKT` 应增加。可用它快速分段定位：

- `PKT` 不增加：先查 9600、HC-05 TX 到 PB5、交叉接线和共地；
- `PKT` 增加但手机没有 display：查 PB4 到 HC-05 RX、小程序 display 控件和 TXR；
- `UERR` 增加：通常是波特率不一致、接线接触或串口电平/信号质量问题；
- `TXR` 增加：发送流量过高或 TX 队列空间不足，应降低 plot/display 频率。

### 24.10 推荐的安全上板顺序

当前 `empty.c` 仍保留用户主动启用的裸 `App_Car_Start();`，所以固件下载并复位后会
自动进入 RUN。默认角度目标为 0，正常情况下不会主动转动，但任何方向符号错误、IMU
异常或旧机械运动都可能造成输出。第一次测试必须架空车轮，最好先断开电机主电源。

推荐顺序：

1. 断开电机主电源，只给 MCU、OLED、IMU、HC-05 供电并下载固件；
2. OLED 应默认进入 ANGLE 页，确认 `V=1`、`FR` 持续增加、`CRC` 基本不增长；
3. 连接手机，等待约 0.5 s，应看到 `MODE:ANGLE` 和 `T/Y/E/G/K`；
4. 发送 `[key,8,down]`，确认 OLED 页面改变且 `PKT` 增加 1；
5. 发送 `[key,9,down]`，或包外发送单字符 `G`，把当前车头定义为 0；
6. 手动向右转动车身，确认 OLED/手机上的 `Y` 和 `G` 按约定增大；
7. 架空车轮并接电机电源，先发 `[slider,4,30]`，观察车辆趋向右转；
8. 再发 `[slider,4,-30]`，观察车辆趋向左转；
9. 架空验证停止和急停后，才允许低速落地测试 `+/-30 deg`；
10. 参数稳定后再逐步测试 `+/-90 deg`，最后才考虑接近 `+/-180 deg`。

每次包外发送 `R` 调用 `App_Car_Start()` 时都会执行 `IMU_ResetYaw()` 和
`AngleControl_Reset()`，因此会把当前方向和目标都恢复为 0。正确操作顺序是先发送
`R`，再发送 slider 4 目标角；不要反过来。

方向判断只允许按下列规则修正：

```text
手动向右转，Y/G 反而减小  -> 只修改 CAR_IMU_YAW_POLARITY
正目标使整车实际左转      -> 立即停机，检查 VehicleMixer 与两轮物理正方向
```

不要为了补偿 IMU 安装方向去修改已经实车验证的 `CAR_LINE_STEERING_POLARITY`、电机
极性或编码器极性。

### 24.11 最终构建和链接证据

本轮已新增 `control/angle_control.c/.h`，并通过用户维护的 `makefile.targets` 加入构建。
正式工程已执行 `gmake clean` 和 `gmake -j4 all`，SysConfig、全部 C 文件、链接和 HEX
生成均成功，退出码为 0。最后一次记录的主要产物为：

```text
empty_LP_MSPM0G3507_nortos_ticlang.out  412488 bytes  2026-07-27 18:26:55
empty_LP_MSPM0G3507_nortos_ticlang.hex   83417 bytes  2026-07-27 18:26:55
empty_LP_MSPM0G3507_nortos_ticlang.map   78616 bytes
control/angle_control.o                    7940 bytes
app/pid_debug.o                           46408 bytes
```

map 已确认包含：

```text
AngleControl_Init
AngleControl_Reset
AngleControl_SetTargetDegrees
AngleControl_Update
AngleControl_GetPID
AngleControl_GetStatus
PIDDebug_PushRxByte
PIDDebug_Printf
PIDDebug_Task
App_Debug_NextOLEDView
App_Scheduler_Run
```

`app_car.o` 明确引用 `AngleControl_Update` 和 `VehicleMixer_Mix`，在当前条件编译模式下
不引用 `LineControl_Update`，这证明运行中的电机控制链确实是定角模式，而不是仅仅在
界面上显示角度。最终源代码扫描未发现 MCU 低功耗调用。
