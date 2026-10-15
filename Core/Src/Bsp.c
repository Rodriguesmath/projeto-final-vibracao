/******************************************************************************/
/**
 * @file    Bsp.c
 * @brief   Implementacao do BSP para Monitor de Vibracao de Baixo Consumo.
 * @author  Rodrigues - Formacao em Sistemas Embarcados Virtus-CC
 * @note    Estilo de codificacao inspirado nas normas do Laboratorio ASSERT (IFPB).
 * @{
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "Bsp.h"
#include <string.h>

/*******************************************************************************
 * DEFINES LOCAIS
 ******************************************************************************/
#define dBSP_I2C_TIMEOUT_MS               100
#define dBSP_UART_TIMEOUT_MS              100

/*******************************************************************************
 * CONSTANTES
 ******************************************************************************/

/*******************************************************************************
 * ESTRUTURAS DE DADOS LOCAIS
 ******************************************************************************/
static struct
{
    I2C_HandleTypeDef   hi2c1;
    UART_HandleTypeDef  huart1;
    RTC_HandleTypeDef   hrtc;
    bool                isInitialized;
} bsp;

/*******************************************************************************
 * PROTOTIPOS LOCAIS
 ******************************************************************************/
static void Bsp_SystemClockConfig(void);
static void Bsp_GpioInit(void);
static void Bsp_I2c1Init(void);
static void Bsp_Uart1Init(void);
static void Bsp_RtcInit(void);

/*******************************************************************************
 * FUNCOES PUBLICAS
 ******************************************************************************/

void Bsp_Init(void)
{
    HAL_Init();
    Bsp_SystemClockConfig();
    Bsp_GpioInit();
    Bsp_I2c1Init();
    Bsp_Uart1Init();
    Bsp_RtcInit();

    bsp.isInitialized = true;
}

void Bsp_LedAlarmSet(bool state)
{
    // LED em PC13 (ativo em nivel BAIXO nas placas Black Pill / ativo ALTO em PB0 na Nucleo)
#if defined(STM32F767xx)
    GPIO_PinState pinState = (state == true) ? GPIO_PIN_SET : GPIO_PIN_RESET;
#else
    GPIO_PinState pinState = (state == true) ? GPIO_PIN_RESET : GPIO_PIN_SET;
#endif
    HAL_GPIO_WritePin(dBSP_LED_ALARM_PORT, dBSP_LED_ALARM_PIN, pinState);
}

void Bsp_LedAlarmToggle(void)
{
    HAL_GPIO_TogglePin(dBSP_LED_ALARM_PORT, dBSP_LED_ALARM_PIN);
}

void Bsp_DelayMs(u32 ms)
{
    HAL_Delay(ms);
}

void Bsp_UartTransmit(const char *pStr)
{
    if(pStr != NULL)
    {
        HAL_UART_Transmit(&bsp.huart1, (uint8_t *)pStr, (uint16_t)strlen(pStr), dBSP_UART_TIMEOUT_MS);
    }
}

bool Bsp_I2cWriteRegister(u8 devAddr, u8 regAddr, u8 val)
{
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&bsp.hi2c1,
                                                 (uint16_t)(devAddr << 1),
                                                 (uint16_t)regAddr,
                                                 I2C_MEMADD_SIZE_8BIT,
                                                 &val,
                                                 1,
                                                 dBSP_I2C_TIMEOUT_MS);
    return (status == HAL_OK);
}

bool Bsp_I2cReadRegisters(u8 devAddr, u8 regAddr, u8 *pBuffer, u16 length)
{
    if(pBuffer == NULL)
    {
        return false;
    }

    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&bsp.hi2c1,
                                                (uint16_t)(devAddr << 1),
                                                (uint16_t)regAddr,
                                                I2C_MEMADD_SIZE_8BIT,
                                                pBuffer,
                                                length,
                                                dBSP_I2C_TIMEOUT_MS);
    return (status == HAL_OK);
}

void Bsp_EnterStopMode(u32 sleepSeconds)
{
    // Configura o Wakeup Timer do RTC
    // Usando clock LSI (~32 kHz) dividido por 16 -> ~2 kHz (0.5 ms por tick)
    u32 wakeupCounter = sleepSeconds * 2000;
    if(wakeupCounter > 0xFFFF)
    {
        wakeupCounter = 0xFFFF;
    }

    HAL_RTCEx_DeactivateWakeUpTimer(&bsp.hrtc);
    __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&bsp.hrtc, RTC_FLAG_WUTF);

    HAL_RTCEx_SetWakeUpTimer_IT(&bsp.hrtc, (uint32_t)wakeupCounter, RTC_WAKEUPCLOCK_RTCCLK_DIV16);

    // Desativa SysTick para nao acordar a cada 1 ms
    HAL_SuspendTick();

    // Desliga perifericos para economia maxima no Stop Mode
    HAL_I2C_DeInit(&bsp.hi2c1);
    HAL_UART_DeInit(&bsp.huart1);

    // Entra no modo STOP com regulador de baixa potencia
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

    // --- PONTO DE RETOMADA APOS O DESPERTAR ---
    Bsp_ResumeFromStopMode();
}

void Bsp_ResumeFromStopMode(void)
{
    // Apos sair de STOP, o clock do sistema reverte para HSI (16 MHz)
    // Reconfigura o PLL para clock pleno
    Bsp_SystemClockConfig();

    // Reativa a interrupcao do SysTick
    HAL_ResumeTick();

    // Reinicializa os perifericos de comunicacao
    Bsp_I2c1Init();
    Bsp_Uart1Init();
}

bool Bsp_IsButtonPressed(void)
{
    GPIO_PinState state = HAL_GPIO_ReadPin(dBSP_BTN_USER_PORT, dBSP_BTN_USER_PIN);
    return (state == GPIO_PIN_SET);
}

/*******************************************************************************
 * FUNCOES LOCAIS
 ******************************************************************************/

static void Bsp_SystemClockConfig(void)
{
    RCC_OscInitTypeDef rccOscInit = {0};
    RCC_ClkInitTypeDef rccClkInit = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
#if defined(PWR_CR_VOS)
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
#endif

    rccOscInit.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI;
    rccOscInit.HSIState = RCC_HSI_ON;
    rccOscInit.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    rccOscInit.LSIState = RCC_LSI_ON;
    rccOscInit.PLL.PLLState = RCC_PLL_ON;
    rccOscInit.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    rccOscInit.PLL.PLLM = 16;
    rccOscInit.PLL.PLLN = 192;
    rccOscInit.PLL.PLLP = RCC_PLLP_DIV2;
    rccOscInit.PLL.PLLQ = 4;
    HAL_RCC_OscConfig(&rccOscInit);

    rccClkInit.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                           RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    rccClkInit.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    rccClkInit.AHBCLKDivider = RCC_SYSCLK_DIV1;
    rccClkInit.APB1CLKDivider = RCC_HCLK_DIV2;
    rccClkInit.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&rccClkInit, FLASH_LATENCY_3);
}

static void Bsp_GpioInit(void)
{
    GPIO_InitTypeDef gpioInit = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // LED de Alarme
    Bsp_LedAlarmSet(false);
    gpioInit.Pin = dBSP_LED_ALARM_PIN;
    gpioInit.Mode = GPIO_MODE_OUTPUT_PP;
    gpioInit.Pull = GPIO_NOPULL;
    gpioInit.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(dBSP_LED_ALARM_PORT, &gpioInit);

    // Botao de Teste Manual (com interrupcao EXTI para acordar do Stop Mode)
    gpioInit.Pin = dBSP_BTN_USER_PIN;
    gpioInit.Mode = GPIO_MODE_IT_RISING;
    gpioInit.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(dBSP_BTN_USER_PORT, &gpioInit);

    // Prioridade e habilitacao da EXTI do botao
    HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

static void Bsp_I2c1Init(void)
{
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef gpioInit = {0};
    gpioInit.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    gpioInit.Mode = GPIO_MODE_AF_OD;
    gpioInit.Pull = GPIO_PULLUP;
    gpioInit.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpioInit.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &gpioInit);

    bsp.hi2c1.Instance = I2C1;
    bsp.hi2c1.Init.ClockSpeed = 400000; // 400 kHz Fast Mode
    bsp.hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    bsp.hi2c1.Init.OwnAddress1 = 0;
    bsp.hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    bsp.hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    bsp.hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    bsp.hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&bsp.hi2c1);
}

static void Bsp_Uart1Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpioInit = {0};
    gpioInit.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    gpioInit.Mode = GPIO_MODE_AF_PP;
    gpioInit.Pull = GPIO_PULLUP;
    gpioInit.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpioInit.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &gpioInit);

    bsp.huart1.Instance = USART1;
    bsp.huart1.Init.BaudRate = 115200;
    bsp.huart1.Init.WordLength = UART_WORDLENGTH_8B;
    bsp.huart1.Init.StopBits = UART_STOPBITS_1;
    bsp.huart1.Init.Parity = UART_PARITY_NONE;
    bsp.huart1.Init.Mode = UART_MODE_TX_RX;
    bsp.huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    bsp.huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&bsp.huart1);
}

static void Bsp_RtcInit(void)
{
    __HAL_RCC_RTC_ENABLE();

    bsp.hrtc.Instance = RTC;
    bsp.hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    bsp.hrtc.Init.AsynchPrediv = 127;
    bsp.hrtc.Init.SynchPrediv = 255;
    bsp.hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    bsp.hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    bsp.hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    HAL_RTC_Init(&bsp.hrtc);

    // Habilita interrupcao do RTC WakeUp no NVIC
    HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
}

/*******************************************************************************
 * HANDLERS DE INTERRUPCAO
 ******************************************************************************/

void RTC_WKUP_IRQHandler(void)
{
    HAL_RTCEx_WakeUpTimerIRQHandler(&bsp.hrtc);
}

void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(dBSP_BTN_USER_PIN);
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    // Despertar acionado pelo RTC
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // Despertar acionado pelo botao manual de teste
}

/** @} DOXYGEN GROUP TAG END OF FILE */
