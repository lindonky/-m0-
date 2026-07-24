/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)


#define CPUCLK_FREQ                                                     32000000



/* Defines for TICK_TIMER */
#define TICK_TIMER_INST                                                  (TIMG0)
#define TICK_TIMER_INST_IRQHandler                              TIMG0_IRQHandler
#define TICK_TIMER_INST_INT_IRQN                                (TIMG0_INT_IRQn)
#define TICK_TIMER_INST_LOAD_VALUE                                      (31999U)



/* Defines for IMU_UART */
#define IMU_UART_INST                                                      UART0
#define IMU_UART_INST_FREQUENCY                                         32000000
#define IMU_UART_INST_IRQHandler                                UART0_IRQHandler
#define IMU_UART_INST_INT_IRQN                                    UART0_INT_IRQn
#define GPIO_IMU_UART_RX_PORT                                              GPIOA
#define GPIO_IMU_UART_TX_PORT                                              GPIOA
#define GPIO_IMU_UART_RX_PIN                                      DL_GPIO_PIN_11
#define GPIO_IMU_UART_TX_PIN                                      DL_GPIO_PIN_10
#define GPIO_IMU_UART_IOMUX_RX                                   (IOMUX_PINCM22)
#define GPIO_IMU_UART_IOMUX_TX                                   (IOMUX_PINCM21)
#define GPIO_IMU_UART_IOMUX_RX_FUNC                    IOMUX_PINCM22_PF_UART0_RX
#define GPIO_IMU_UART_IOMUX_TX_FUNC                    IOMUX_PINCM21_PF_UART0_TX
#define IMU_UART_BAUD_RATE                                              (115200)
#define IMU_UART_IBRD_32_MHZ_115200_BAUD                                    (17)
#define IMU_UART_FBRD_32_MHZ_115200_BAUD                                    (23)
/* Defines for HC05_UART */
#define HC05_UART_INST                                                     UART1
#define HC05_UART_INST_FREQUENCY                                        32000000
#define HC05_UART_INST_IRQHandler                               UART1_IRQHandler
#define HC05_UART_INST_INT_IRQN                                   UART1_INT_IRQn
#define GPIO_HC05_UART_RX_PORT                                             GPIOB
#define GPIO_HC05_UART_TX_PORT                                             GPIOB
#define GPIO_HC05_UART_RX_PIN                                      DL_GPIO_PIN_5
#define GPIO_HC05_UART_TX_PIN                                      DL_GPIO_PIN_4
#define GPIO_HC05_UART_IOMUX_RX                                  (IOMUX_PINCM18)
#define GPIO_HC05_UART_IOMUX_TX                                  (IOMUX_PINCM17)
#define GPIO_HC05_UART_IOMUX_RX_FUNC                   IOMUX_PINCM18_PF_UART1_RX
#define GPIO_HC05_UART_IOMUX_TX_FUNC                   IOMUX_PINCM17_PF_UART1_TX
#define HC05_UART_BAUD_RATE                                             (115200)
#define HC05_UART_IBRD_32_MHZ_115200_BAUD                                   (17)
#define HC05_UART_FBRD_32_MHZ_115200_BAUD                                   (23)





/* Defines for LINE_ADC0 */
#define LINE_ADC0_INST                                                      ADC0
#define LINE_ADC0_INST_IRQHandler                                ADC0_IRQHandler
#define LINE_ADC0_INST_INT_IRQN                                  (ADC0_INT_IRQn)
#define LINE_ADC0_ADCMEM_0                                    DL_ADC12_MEM_IDX_0
#define LINE_ADC0_ADCMEM_0_REF                   DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define LINE_ADC0_ADCMEM_0_REF_VOLTAGE_V                                     3.3
#define LINE_ADC0_ADCMEM_1                                    DL_ADC12_MEM_IDX_1
#define LINE_ADC0_ADCMEM_1_REF                   DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define LINE_ADC0_ADCMEM_1_REF_VOLTAGE_V                                     3.3
#define LINE_ADC0_ADCMEM_2                                    DL_ADC12_MEM_IDX_2
#define LINE_ADC0_ADCMEM_2_REF                   DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define LINE_ADC0_ADCMEM_2_REF_VOLTAGE_V                                     3.3
#define GPIO_LINE_ADC0_C2_PORT                                             GPIOA
#define GPIO_LINE_ADC0_C2_PIN                                     DL_GPIO_PIN_25
#define GPIO_LINE_ADC0_IOMUX_C2                                  (IOMUX_PINCM55)
#define GPIO_LINE_ADC0_IOMUX_C2_FUNC              (IOMUX_PINCM55_PF_UNCONNECTED)
#define GPIO_LINE_ADC0_C3_PORT                                             GPIOA
#define GPIO_LINE_ADC0_C3_PIN                                     DL_GPIO_PIN_24
#define GPIO_LINE_ADC0_IOMUX_C3                                  (IOMUX_PINCM54)
#define GPIO_LINE_ADC0_IOMUX_C3_FUNC              (IOMUX_PINCM54_PF_UNCONNECTED)
#define GPIO_LINE_ADC0_C7_PORT                                             GPIOA
#define GPIO_LINE_ADC0_C7_PIN                                     DL_GPIO_PIN_22
#define GPIO_LINE_ADC0_IOMUX_C7                                  (IOMUX_PINCM47)
#define GPIO_LINE_ADC0_IOMUX_C7_FUNC              (IOMUX_PINCM47_PF_UNCONNECTED)

/* Defines for LINE_ADC1 */
#define LINE_ADC1_INST                                                      ADC1
#define LINE_ADC1_INST_IRQHandler                                ADC1_IRQHandler
#define LINE_ADC1_INST_INT_IRQN                                  (ADC1_INT_IRQn)
#define LINE_ADC1_ADCMEM_0                                    DL_ADC12_MEM_IDX_0
#define LINE_ADC1_ADCMEM_0_REF                   DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define LINE_ADC1_ADCMEM_0_REF_VOLTAGE_V                                     3.3
#define LINE_ADC1_ADCMEM_1                                    DL_ADC12_MEM_IDX_1
#define LINE_ADC1_ADCMEM_1_REF                   DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define LINE_ADC1_ADCMEM_1_REF_VOLTAGE_V                                     3.3
#define LINE_ADC1_ADCMEM_2                                    DL_ADC12_MEM_IDX_2
#define LINE_ADC1_ADCMEM_2_REF                   DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define LINE_ADC1_ADCMEM_2_REF_VOLTAGE_V                                     3.3
#define LINE_ADC1_ADCMEM_3                                    DL_ADC12_MEM_IDX_3
#define LINE_ADC1_ADCMEM_3_REF                   DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define LINE_ADC1_ADCMEM_3_REF_VOLTAGE_V                                     3.3
#define LINE_ADC1_ADCMEM_4                                    DL_ADC12_MEM_IDX_4
#define LINE_ADC1_ADCMEM_4_REF                   DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define LINE_ADC1_ADCMEM_4_REF_VOLTAGE_V                                     3.3
#define GPIO_LINE_ADC1_C0_PORT                                             GPIOA
#define GPIO_LINE_ADC1_C0_PIN                                     DL_GPIO_PIN_15
#define GPIO_LINE_ADC1_IOMUX_C0                                  (IOMUX_PINCM37)
#define GPIO_LINE_ADC1_IOMUX_C0_FUNC              (IOMUX_PINCM37_PF_UNCONNECTED)
#define GPIO_LINE_ADC1_C2_PORT                                             GPIOA
#define GPIO_LINE_ADC1_C2_PIN                                     DL_GPIO_PIN_17
#define GPIO_LINE_ADC1_IOMUX_C2                                  (IOMUX_PINCM39)
#define GPIO_LINE_ADC1_IOMUX_C2_FUNC              (IOMUX_PINCM39_PF_UNCONNECTED)
#define GPIO_LINE_ADC1_C4_PORT                                             GPIOB
#define GPIO_LINE_ADC1_C4_PIN                                     DL_GPIO_PIN_17
#define GPIO_LINE_ADC1_IOMUX_C4                                  (IOMUX_PINCM43)
#define GPIO_LINE_ADC1_IOMUX_C4_FUNC              (IOMUX_PINCM43_PF_UNCONNECTED)
#define GPIO_LINE_ADC1_C5_PORT                                             GPIOB
#define GPIO_LINE_ADC1_C5_PIN                                     DL_GPIO_PIN_18
#define GPIO_LINE_ADC1_IOMUX_C5                                  (IOMUX_PINCM44)
#define GPIO_LINE_ADC1_IOMUX_C5_FUNC              (IOMUX_PINCM44_PF_UNCONNECTED)
#define GPIO_LINE_ADC1_C6_PORT                                             GPIOB
#define GPIO_LINE_ADC1_C6_PIN                                     DL_GPIO_PIN_19
#define GPIO_LINE_ADC1_IOMUX_C6                                  (IOMUX_PINCM45)
#define GPIO_LINE_ADC1_IOMUX_C6_FUNC              (IOMUX_PINCM45_PF_UNCONNECTED)



/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_TICK_TIMER_init(void);
void SYSCFG_DL_IMU_UART_init(void);
void SYSCFG_DL_HC05_UART_init(void);
void SYSCFG_DL_LINE_ADC0_init(void);
void SYSCFG_DL_LINE_ADC1_init(void);

bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
