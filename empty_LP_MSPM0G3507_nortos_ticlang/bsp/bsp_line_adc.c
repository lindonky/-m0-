/*
 * 八路地址复用数字循迹模块适配层。
 *
 * 模块用 AD2:AD0 选择 CH0~CH7，并从 OUT 输出所选通道的数字比较结果。资料示例
 * 每切换一路阻塞 100 us；本工程不阻塞控制环，而是利用既有 1 ms 传感器任务：
 * - 本次调用读取上一次已经选好的通道；
 * - 随后立刻切换到下一通道；
 * - 下一次 1 ms 调用再读取，因此地址建立时间远大于资料建议的 100 us；
 * - 扫描完 8 路才向上层提交一帧，帧率为 125 Hz。
 *
 * 模块只能提供 0/1，不是八路模拟 ADC。为了保持现有 LineSensor 标定、极性和
 * 加权质心接口，数字状态在这里映射成 0 或 4095 的伪原始值。
 */
#include "bsp/bsp_line_adc.h"

#include <stddef.h>

#include "config/board_config.h"

#if CAR_LINE_MUX_READY
#include "ti_msp_dl_config.h"
#endif

_Static_assert(CAR_LINE_SENSOR_COUNT == 8U,
               "The address-multiplexed line sensor requires exactly 8 channels");
_Static_assert(CAR_LINE_MUX_ACTIVE_LEVEL <= 1U,
               "CAR_LINE_MUX_ACTIVE_LEVEL must be 0 or 1");
_Static_assert(CAR_LINE_MUX_REVERSE_ORDER <= 1U,
               "CAR_LINE_MUX_REVERSE_ORDER must be 0 or 1");

static uint16_t g_workingFrame[CAR_LINE_SENSOR_COUNT];
static uint8_t g_selectedChannel;
static uint32_t g_frameCount;

#if CAR_LINE_MUX_READY
/** @brief 写单个推挽地址引脚；三个地址位可以位于不同 GPIO 端口。 */
static void write_address_pin(GPIO_Regs *port, uint32_t pin, bool high)
{
    if (high) {
        DL_GPIO_setPins(port, pin);
    } else {
        DL_GPIO_clearPins(port, pin);
    }
}

/** @brief 按 AD2=C、AD1=B、AD0=A 的二进制表选择 CH0~CH7。 */
static void select_channel(uint8_t channel)
{
    write_address_pin(CAR_LINE_MUX_AD0_PORT, CAR_LINE_MUX_AD0_PIN,
                      (channel & 0x01U) != 0U);
    write_address_pin(CAR_LINE_MUX_AD1_PORT, CAR_LINE_MUX_AD1_PIN,
                      (channel & 0x02U) != 0U);
    write_address_pin(CAR_LINE_MUX_AD2_PORT, CAR_LINE_MUX_AD2_PIN,
                      (channel & 0x04U) != 0U);
}

/** @brief 把“目标线是否被检测”转换成符合上层黑线极性约定的伪 ADC 值。 */
static uint16_t target_detected_to_raw(bool targetDetected)
{
    /*
     * normalize() 在“目标为黑且黑线原始值低”或“目标为白且黑线原始值高”时
     * 会反相。因此这两种配置要把目标状态映射为低值，其余配置映射为高值。
     */
    const bool normalizationWillInvert =
        ((CAR_LINE_ACTIVE_DARK != 0U) == (CAR_LINE_BLACK_IS_LOW_RAW != 0U));

    if (normalizationWillInvert) {
        return targetDetected ? 0U : CAR_LINE_MUX_PSEUDO_ADC_MAX;
    }
    return targetDetected ? CAR_LINE_MUX_PSEUDO_ADC_MAX : 0U;
}
#endif

void BSP_LineADC_Init(void)
{
    uint32_t index;

    g_selectedChannel = 0U;
    g_frameCount = 0U;
    for (index = 0U; index < CAR_LINE_SENSOR_COUNT; ++index) {
        g_workingFrame[index] = 0U;
    }

#if CAR_LINE_MUX_READY
    /* GPIO 方向和 PinMux 必须已经由 SYSCFG_DL_init() 完成；这里只选择首通道。 */
    select_channel(0U);
#endif
}

bool BSP_LineADC_Read(uint16_t values[CAR_LINE_SENSOR_COUNT])
{
#if CAR_LINE_MUX_READY
    uint32_t index;
    uint32_t outputLevel;
    uint8_t outputIndex;
    bool electricalHigh;
    bool targetDetected;

    if (values == NULL) return false;

    outputLevel = DL_GPIO_readPins(CAR_LINE_MUX_OUT_PORT,
                                   CAR_LINE_MUX_OUT_PIN);
    electricalHigh = ((outputLevel & CAR_LINE_MUX_OUT_PIN) != 0U);
    targetDetected = (electricalHigh == (CAR_LINE_MUX_ACTIVE_LEVEL != 0U));

    outputIndex = g_selectedChannel;
    if (CAR_LINE_MUX_REVERSE_ORDER) {
        outputIndex = (uint8_t) ((CAR_LINE_SENSOR_COUNT - 1U) - outputIndex);
    }
    g_workingFrame[outputIndex] = target_detected_to_raw(targetDetected);

    g_selectedChannel++;
    if (g_selectedChannel < CAR_LINE_SENSOR_COUNT) {
        /* 提前切换地址，让硬件在下一次 1 ms 调用前充分稳定。 */
        select_channel(g_selectedChannel);
        return false;
    }

    /* 八路都完成后一次复制，保证上层不会看到半帧新、半帧旧的数据。 */
    for (index = 0U; index < CAR_LINE_SENSOR_COUNT; ++index) {
        values[index] = g_workingFrame[index];
    }
    g_frameCount++;
    g_selectedChannel = 0U;
    select_channel(0U);
    return true;
#else
    uint32_t index;

    if (values == NULL) return false;
    for (index = 0U; index < CAR_LINE_SENSOR_COUNT; ++index) {
        values[index] = 0U;
    }
    return false;
#endif
}

bool BSP_LineADC_IsReady(void)
{
#if CAR_LINE_MUX_READY
    return true;
#else
    return false;
#endif
}

uint32_t BSP_LineADC_GetFrameCount(void)
{
    return g_frameCount;
}
