/******************************************************************************/
/**
 * @file    main.c
 * @brief   Ponto de entrada do Monitor de Vibracao de Baixo Consumo (10 Anos).
 * @author  Rodrigues - Formacao em Sistemas Embarcados Virtus-CC
 * @note    Estilo de codificacao inspirado nas normas do Laboratorio ASSERT (IFPB).
 * @{
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "main.h"
#include "Bsp.h"
#include "Mpu6050.h"
#include "VibrationAnalyzer.h"
#include <stdio.h>
#include <string.h>

/*******************************************************************************
 * DEFINES LOCAIS
 ******************************************************************************/
#define dALARM_PULSE_COUNT               5
#define dALARM_PULSE_MS                  20

/*******************************************************************************
 * CONSTANTES
 ******************************************************************************/

/*******************************************************************************
 * ESTRUTURAS DE DADOS LOCAIS
 ******************************************************************************/
static struct
{
    u32                cycleCounter;
    sVibrationResult_t lastVibResult;
} mainApp;

/*******************************************************************************
 * PROTOTIPOS LOCAIS
 ******************************************************************************/
static void Main_PerformMeasurementCycle(void);
static void Main_PrintDiagnostics(const sVibrationResult_t *pResult);
static void Main_TriggerVisualAlarm(void);

/*******************************************************************************
 * FUNCOES PUBLICAS
 ******************************************************************************/

int main(void)
{
    Bsp_Init();
    Mpu6050_Init();
    VibrationAnalyzer_Init(dVIB_DEFAULT_THRESHOLD_G);

    mainApp.cycleCounter = 0;

    Bsp_UartTransmit("\r\n======================================================\r\n");
    Bsp_UartTransmit("  Projeto Final: Monitor de Vibracao Industrial (10 Anos)\r\n");
    Bsp_UartTransmit("  ISO 10816 Severity | Low-Power RTC Wakeup & MPU Sleep\r\n");
    Bsp_UartTransmit("  Assert Coding Standard - Virtus-CC / Rodrigues\r\n");
    Bsp_UartTransmit("======================================================\r\n");

    while(1)
    {
        // 1. Executa ciclo de medicao e diagnostico
        Main_PerformMeasurementCycle();

        // 2. Coloca o microcontrolador em Stop Mode de baixissimo consumo
        // O despertar ocorrera via alarme RTC (a cada 8 horas) ou botao do usuario (EXTI)
        Bsp_UartTransmit(">>> Entrando em Stop Mode (Standby de Ultra-Baixo Consumo)...\r\n");
        Bsp_EnterStopMode(dBSP_RTC_WAKEUP_INTERVAL_SEC);

        // 3. Ao acordar, retransmite aviso de despertar
        Bsp_UartTransmit("\r\n>>> Interrupcao de Despertar Detectada! Reiniciando ciclo... <<<\r\n");
    }
}

/*******************************************************************************
 * FUNCOES LOCAIS
 ******************************************************************************/

static void Main_PerformMeasurementCycle(void)
{
    mainApp.cycleCounter++;

    // Acorda o acelerometro MPU-6050 (sai do modo Sleep de 5 uA para 3.5 mA)
    Mpu6050_SetSleepMode(false);

    // Amostra janela de 512 pontos a 250 Hz (duracao de ~2,05 segundos)
    VibrationAnalyzer_AcquireWindow();

    // Coloca imediatamente o MPU-6050 de volta em Sleep (5 uA)
    Mpu6050_SetSleepMode(true);

    // Calcula os valores eficazes (RMS dinamico) de vibracao
    VibrationAnalyzer_CalculateRms();
    mainApp.lastVibResult = VibrationAnalyzer_GetResults();

    // Imprime diagnosticos via porta serial
    Main_PrintDiagnostics(&mainApp.lastVibResult);

    // Avalia acionamento de alarme visual
    if(mainApp.lastVibResult.isAlarmActive == true)
    {
        Main_TriggerVisualAlarm();
    }
    else
    {
        Bsp_LedAlarmSet(false);
    }
}

static void Main_PrintDiagnostics(const sVibrationResult_t *pResult)
{
    char msg[160];

    if(pResult == NULL)
    {
        return;
    }

    snprintf(msg, sizeof(msg),
             "\r\n[Ciclo #%lu] Vibracao RMS: X=%.3f g | Y=%.3f g | Z=%.3f g | Total=%.3f g\r\n",
             (unsigned long)mainApp.cycleCounter,
             pResult->rmsX,
             pResult->rmsY,
             pResult->rmsZ,
             pResult->rmsTotal);
    Bsp_UartTransmit(msg);

    if(pResult->severity == eVIB_SEVERITY_GOOD)
    {
        Bsp_UartTransmit("STATUS: BOM (Operacao Normal dentro dos limites ISO 10816)\r\n");
    }
    else if(pResult->severity == eVIB_SEVERITY_ACCEPTABLE)
    {
        Bsp_UartTransmit("STATUS: ACEITAVEL (Requer acompanhamento preventivo)\r\n");
    }
    else
    {
        Bsp_UartTransmit("STATUS: ALARME CRITICO! (Vibracao severa detectada - Manutencao Imediata!)\r\n");
    }
}

static void Main_TriggerVisualAlarm(void)
{
    // Pisca LED com pulsos curtos para alertar no painel sem drenar a bateria
    for(u8 i = 0; i < dALARM_PULSE_COUNT; i++)
    {
        Bsp_LedAlarmSet(true);
        Bsp_DelayMs(dALARM_PULSE_MS);
        Bsp_LedAlarmSet(false);
        Bsp_DelayMs(100);
    }
}

/** @} DOXYGEN GROUP TAG END OF FILE */
