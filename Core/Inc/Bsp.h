/******************************************************************************/
/**
 * @file    Bsp.h
 * @brief   Board Support Package para Monitor de Vibracao de Baixo Consumo.
 * @author  Rodrigues - Formacao em Sistemas Embarcados Virtus-CC
 * @note    Estilo de codificacao inspirado nas normas do Laboratorio ASSERT (IFPB).
 * @{
 ******************************************************************************/
#ifndef _BSP_H_
#define _BSP_H_

/*******************************************************************************
 * INCLUDES NECESSARIOS
 ******************************************************************************/
#include "AssertTypes.h"

#if defined(STM32F411xE) || defined(STM32F401xC)
  #include "stm32f4xx_hal.h"
#elif defined(STM32H723xx)
  #include "stm32h7xx_hal.h"
#elif defined(STM32F767xx)
  #include "stm32f7xx_hal.h"
#else
  #include "stm32f4xx_hal.h"
#endif

/*******************************************************************************
 * CONFIGURACOES
 ******************************************************************************/
// Intervalo de despertar do RTC: 8 horas em segundos (8 * 3600 = 28800 s)
// Para testes rápidos de bancada, pode ser ajustado para 10s ou 30s
#define dBSP_RTC_WAKEUP_INTERVAL_SEC      28800

/*******************************************************************************
 * DEFINES PUBLICOS
 ******************************************************************************/
#if defined(STM32F767xx)
  #define dBSP_LED_ALARM_PIN              GPIO_PIN_0
  #define dBSP_LED_ALARM_PORT             GPIOB
  #define dBSP_BTN_USER_PIN               GPIO_PIN_13
  #define dBSP_BTN_USER_PORT              GPIOC
#else
  #define dBSP_LED_ALARM_PIN              GPIO_PIN_13
  #define dBSP_LED_ALARM_PORT             GPIOC
  #define dBSP_BTN_USER_PIN               GPIO_PIN_0
  #define dBSP_BTN_USER_PORT              GPIOA
#endif

/*******************************************************************************
 * PROTOTIPOS PUBLICOS
 ******************************************************************************/
void Bsp_Init(void);
void Bsp_LedAlarmSet(bool state);
void Bsp_LedAlarmToggle(void);
void Bsp_DelayMs(u32 ms);
void Bsp_UartTransmit(const char *pStr);
bool Bsp_I2cWriteRegister(u8 devAddr, u8 regAddr, u8 val);
bool Bsp_I2cReadRegisters(u8 devAddr, u8 regAddr, u8 *pBuffer, u16 length);
void Bsp_EnterStopMode(u32 sleepSeconds);
void Bsp_ResumeFromStopMode(void);
bool Bsp_IsButtonPressed(void);

#endif /* _BSP_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
