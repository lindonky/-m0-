/*
 * 编码器硬件适配层。
 *
 * 本层只返回“当前累计计数”，方向极性、轮径和速度滤波由 drivers/encoder.c 处理。
 * 左轮 TIMG8 是 16 位硬件 QEI，本层按相邻读数差扩展为 32 位；右轮 PB8/PB9
 * 使用双边沿 GPIO 中断和四状态查表完成 AB 四倍频解码。
 */
#include "bsp/bsp_encoder.h"

#include <stddef.h>
#include "config/board_config.h"
#include "ti_msp_dl_config.h"

#if CAR_ENCODER_READY
/*
 * 索引为 previousAB << 2 | currentAB。合法的单相变化返回 ±1；未变化返回 0；
 * 两相同时改变也是 0，但会单独计入 invalidTransitions，便于发现干扰或漏边沿。
 */
static const int8_t g_quadratureTable[16] = {
     0, -1, +1,  0,
    +1,  0,  0, -1,
    -1,  0,  0, +1,
     0, +1, -1,  0
};

static uint16_t g_leftPreviousRaw;
static int32_t g_leftExtendedCount;

static volatile int32_t g_rightCount;
static volatile uint32_t g_rightEdgeInterrupts;
static volatile uint32_t g_rightInvalidTransitions;
static volatile uint8_t g_rightPreviousState;

/** @brief 读取 PB8/PB9 并压缩为 bit1=A、bit0=B 的两位状态。 */
static uint8_t read_right_state(void)
{
    const uint32_t pins = DL_GPIO_readPins(
        CAR_ENCODER_RIGHT_GPIO_PORT,
        CAR_ENCODER_RIGHT_A_GPIO_PIN | CAR_ENCODER_RIGHT_B_GPIO_PIN);
    uint8_t state = 0U;

    if ((pins & CAR_ENCODER_RIGHT_A_GPIO_PIN) != 0U) state |= 0x02U;
    if ((pins & CAR_ENCODER_RIGHT_B_GPIO_PIN) != 0U) state |= 0x01U;
    return state;
}
#endif

void BSP_Encoder_Init(void)
{
#if CAR_ENCODER_READY
    const uint32_t rightMask = CAR_ENCODER_RIGHT_A_GPIO_PIN |
                               CAR_ENCODER_RIGHT_B_GPIO_PIN;

    /* 先准备右轮软件状态，再开放共享 GPIOB 中断，避免第一次边沿使用未知旧状态。 */
    NVIC_DisableIRQ(CAR_ENCODER_RIGHT_IRQN);
    DL_GPIO_clearInterruptStatus(CAR_ENCODER_RIGHT_GPIO_PORT, rightMask);
    g_rightCount = 0;
    g_rightEdgeInterrupts = 0U;
    g_rightInvalidTransitions = 0U;
    g_rightPreviousState = read_right_state();
    NVIC_ClearPendingIRQ(CAR_ENCODER_RIGHT_IRQN);
    NVIC_EnableIRQ(CAR_ENCODER_RIGHT_IRQN);

    /* SysConfig 只完成 QEI 静态配置且 Start Timer 未勾选，由 BSP 最后启动。 */
    DL_TimerG_stopCounter(CAR_ENCODER_LEFT_QEI_INST);
    DL_TimerG_setTimerCount(CAR_ENCODER_LEFT_QEI_INST, 0U);
    g_leftPreviousRaw = 0U;
    g_leftExtendedCount = 0;
    DL_TimerG_startCounter(CAR_ENCODER_LEFT_QEI_INST);
#endif
}

int32_t BSP_Encoder_GetLeftCount(void)
{
#if CAR_ENCODER_READY
    const uint16_t currentRaw =
        (uint16_t) DL_TimerG_getTimerCount(CAR_ENCODER_LEFT_QEI_INST);

    /*
     * 16 位差值自然处理 0↔65535 回绕。前提是两次读取间绝对增量小于 32768；
     * 本工程每 5 ms 读取一次，MG513 的实际转速远低于该上限。
     */
    g_leftExtendedCount += (int16_t) (currentRaw - g_leftPreviousRaw);
    g_leftPreviousRaw = currentRaw;
    return g_leftExtendedCount;
#else
    return 0;
#endif
}

int32_t BSP_Encoder_GetRightCount(void)
{
#if CAR_ENCODER_READY
    /* Cortex-M0+ 对自然对齐的 32 位读写是原子的。 */
    return g_rightCount;
#else
    return 0;
#endif
}

void BSP_Encoder_ResetCounts(void)
{
#if CAR_ENCODER_READY
    const uint32_t rightMask = CAR_ENCODER_RIGHT_A_GPIO_PIN |
                               CAR_ENCODER_RIGHT_B_GPIO_PIN;
    const uint32_t irqWasEnabled = NVIC_GetEnableIRQ(CAR_ENCODER_RIGHT_IRQN);

    /* DriverLib 明确不建议在计数器运行时直接写 CTR，所以先停、清零、再启动。 */
    DL_TimerG_stopCounter(CAR_ENCODER_LEFT_QEI_INST);
    DL_TimerG_setTimerCount(CAR_ENCODER_LEFT_QEI_INST, 0U);
    g_leftPreviousRaw = 0U;
    g_leftExtendedCount = 0;
    DL_TimerG_startCounter(CAR_ENCODER_LEFT_QEI_INST);

    /* 暂停 GPIOB NVIC，保证右累计值、上次 AB 状态和诊断计数作为一个整体复位。 */
    NVIC_DisableIRQ(CAR_ENCODER_RIGHT_IRQN);
    DL_GPIO_clearInterruptStatus(CAR_ENCODER_RIGHT_GPIO_PORT, rightMask);
    g_rightCount = 0;
    g_rightEdgeInterrupts = 0U;
    g_rightInvalidTransitions = 0U;
    g_rightPreviousState = read_right_state();
    NVIC_ClearPendingIRQ(CAR_ENCODER_RIGHT_IRQN);
    if (irqWasEnabled != 0U) NVIC_EnableIRQ(CAR_ENCODER_RIGHT_IRQN);
#endif
}

void BSP_Encoder_GetDiagnostics(BSP_Encoder_Diagnostics *diagnostics)
{
    if (diagnostics == NULL) return;

#if CAR_ENCODER_READY
    uint32_t sequenceBefore;
    uint32_t sequenceAfter;

    /*
     * edgeInterrupts 在 ISR 最后更新，相当于轻量序列号。若拷贝中途被 ISR 抢占，
     * 前后序列号不同就重读，不需要为 OLED 诊断暂时关闭高速编码器中断。
     */
    do {
        sequenceBefore = g_rightEdgeInterrupts;
        diagnostics->invalidTransitions = g_rightInvalidTransitions;
        diagnostics->state = g_rightPreviousState;
        sequenceAfter = g_rightEdgeInterrupts;
    } while (sequenceBefore != sequenceAfter);
    diagnostics->edgeInterrupts = sequenceAfter;
#else
    diagnostics->edgeInterrupts = 0U;
    diagnostics->invalidTransitions = 0U;
    diagnostics->state = 0U;
#endif
}

#if CAR_ENCODER_READY
/**
 * @brief MSPM0G3507 共享中断组 1 入口；当前只分派 GPIOB 的右编码器来源。
 *
 * ISR 只做确定时间的寄存器读取、清状态和查表加减，不执行 PID、OLED、串口、
 * 浮点运算或延时。以后若在 GROUP1 增加 GPIOA/COMP 等来源，必须在此扩充分派。
 */
void CAR_ENCODER_GROUP_IRQ_HANDLER(void)
{
    uint32_t pendingPins;
    uint8_t currentState;
    uint8_t tableIndex;
    int8_t step;

    if (DL_Interrupt_getPendingGroup(DL_INTERRUPT_GROUP_1) !=
        CAR_ENCODER_RIGHT_GROUP_IIDX) {
        return;
    }

    pendingPins = DL_GPIO_getEnabledInterruptStatus(
        CAR_ENCODER_RIGHT_GPIO_PORT,
        CAR_ENCODER_RIGHT_A_GPIO_PIN | CAR_ENCODER_RIGHT_B_GPIO_PIN);
    if (pendingPins == 0U) return;

    /* 先清已捕获边沿；清除之后到来的新边沿会再次置位并重新进入 ISR。 */
    DL_GPIO_clearInterruptStatus(CAR_ENCODER_RIGHT_GPIO_PORT, pendingPins);

    currentState = read_right_state();
    tableIndex = (uint8_t) ((g_rightPreviousState << 2) | currentState);
    step = g_quadratureTable[tableIndex];

    if ((currentState != g_rightPreviousState) && (step == 0)) {
        ++g_rightInvalidTransitions;
    }
    g_rightCount += step;
    g_rightPreviousState = currentState;

    /* 两根脚同时 pending 时按两次硬件边沿计数，便于估算实际中断负载。 */
    if ((pendingPins & CAR_ENCODER_RIGHT_A_GPIO_PIN) != 0U) {
        ++g_rightEdgeInterrupts;
    }
    if ((pendingPins & CAR_ENCODER_RIGHT_B_GPIO_PIN) != 0U) {
        ++g_rightEdgeInterrupts;
    }
}
#endif
