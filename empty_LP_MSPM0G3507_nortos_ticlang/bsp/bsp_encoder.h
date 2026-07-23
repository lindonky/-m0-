#ifndef BSP_ENCODER_H
#define BSP_ENCODER_H

#include <stdint.h>

/** @file bsp_encoder.h @brief 左右硬件 QEI 计数器的最小抽象接口。 */

/** @brief 启动左右 QEI；具体引脚和计数方式由 SysConfig 决定。 */
void BSP_Encoder_Init(void);

/** @return 左编码器当前有符号累计计数。 */
int32_t BSP_Encoder_GetLeftCount(void);

/** @return 右编码器当前有符号累计计数。 */
int32_t BSP_Encoder_GetRightCount(void);

/** @brief 同时清零硬件计数器和可能存在的软件溢出扩展。 */
void BSP_Encoder_ResetCounts(void);

#endif /* BSP_ENCODER_H */
