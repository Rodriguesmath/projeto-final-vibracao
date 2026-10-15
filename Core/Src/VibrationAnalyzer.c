/******************************************************************************/
/**
 * @file    VibrationAnalyzer.c
 * @brief   Implementacao da analise estatistica de vibracao RMS em tempo real.
 * @author  Rodrigues - Formacao em Sistemas Embarcados Virtus-CC
 * @note    Estilo de codificacao inspirado nas normas do Laboratorio ASSERT (IFPB).
 * @{
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "VibrationAnalyzer.h"
#include "Mpu6050.h"
#include "Bsp.h"
#include <math.h>

/*******************************************************************************
 * DEFINES LOCAIS
 ******************************************************************************/
#define dSEVERITY_GOOD_THRESHOLD_G        0.20f

/*******************************************************************************
 * CONSTANTES
 ******************************************************************************/

/*******************************************************************************
 * ESTRUTURAS DE DADOS LOCAIS
 ******************************************************************************/
static struct
{
    float              samplesX[dVIB_WINDOW_SAMPLES];
    float              samplesY[dVIB_WINDOW_SAMPLES];
    float              samplesZ[dVIB_WINDOW_SAMPLES];
    float              thresholdG;
    sVibrationResult_t lastResult;
    bool               isConfigured;
} vibrationAnalyzer;

/*******************************************************************************
 * PROTOTIPOS LOCAIS
 ******************************************************************************/
static float VibrationAnalyzer_ComputeAxisRms(const float *pSamples, u16 count);

/*******************************************************************************
 * FUNCOES PUBLICAS
 ******************************************************************************/

void VibrationAnalyzer_Init(float alarmThresholdG)
{
    vibrationAnalyzer.thresholdG = (alarmThresholdG > 0.05f) ? alarmThresholdG : dVIB_DEFAULT_THRESHOLD_G;
    vibrationAnalyzer.lastResult.rmsX = 0.0f;
    vibrationAnalyzer.lastResult.rmsY = 0.0f;
    vibrationAnalyzer.lastResult.rmsZ = 0.0f;
    vibrationAnalyzer.lastResult.rmsTotal = 0.0f;
    vibrationAnalyzer.lastResult.severity = eVIB_SEVERITY_GOOD;
    vibrationAnalyzer.lastResult.isAlarmActive = false;
    vibrationAnalyzer.isConfigured = true;
}

void VibrationAnalyzer_AcquireWindow(void)
{
    // Amostra a janela de 512 pontos a 250 Hz (intervalo de 4 ms por amostra)
    for(u16 i = 0; i < dVIB_WINDOW_SAMPLES; i++)
    {
        float ax = 0.0f;
        float ay = 0.0f;
        float az = 0.0f;

        Mpu6050_ReadScaledAccelG(&ax, &ay, &az);

        vibrationAnalyzer.samplesX[i] = ax;
        vibrationAnalyzer.samplesY[i] = ay;
        vibrationAnalyzer.samplesZ[i] = az;

        Bsp_DelayMs(4);
    }
}

void VibrationAnalyzer_CalculateRms(void)
{
    // Calcula o RMS dinamico (subtrai componente DC da gravidade) em cada eixo
    float rmsX = VibrationAnalyzer_ComputeAxisRms(vibrationAnalyzer.samplesX, dVIB_WINDOW_SAMPLES);
    float rmsY = VibrationAnalyzer_ComputeAxisRms(vibrationAnalyzer.samplesY, dVIB_WINDOW_SAMPLES);
    float rmsZ = VibrationAnalyzer_ComputeAxisRms(vibrationAnalyzer.samplesZ, dVIB_WINDOW_SAMPLES);

    // Magnitude resultante total do vetor de vibracao
    float rmsTotal = sqrtf((rmsX * rmsX) + (rmsY * rmsY) + (rmsZ * rmsZ));

    vibrationAnalyzer.lastResult.rmsX = rmsX;
    vibrationAnalyzer.lastResult.rmsY = rmsY;
    vibrationAnalyzer.lastResult.rmsZ = rmsZ;
    vibrationAnalyzer.lastResult.rmsTotal = rmsTotal;

    // Classificacao de severidade industrial (baseada na norma ISO 10816)
    if(rmsTotal < dSEVERITY_GOOD_THRESHOLD_G)
    {
        vibrationAnalyzer.lastResult.severity = eVIB_SEVERITY_GOOD;
        vibrationAnalyzer.lastResult.isAlarmActive = false;
    }
    else if(rmsTotal < vibrationAnalyzer.thresholdG)
    {
        vibrationAnalyzer.lastResult.severity = eVIB_SEVERITY_ACCEPTABLE;
        vibrationAnalyzer.lastResult.isAlarmActive = false;
    }
    else
    {
        vibrationAnalyzer.lastResult.severity = eVIB_SEVERITY_ALARM;
        vibrationAnalyzer.lastResult.isAlarmActive = true;
    }
}

sVibrationResult_t VibrationAnalyzer_GetResults(void)
{
    return vibrationAnalyzer.lastResult;
}

bool VibrationAnalyzer_IsAlarmTriggered(void)
{
    return vibrationAnalyzer.lastResult.isAlarmActive;
}

/*******************************************************************************
 * FUNCOES LOCAIS
 ******************************************************************************/

static float VibrationAnalyzer_ComputeAxisRms(const float *pSamples, u16 count)
{
    if((pSamples == NULL) || (count == 0))
    {
        return 0.0f;
    }

    // 1. Calcula a media (offset DC provocado pela inclinacao e gravidade estatica de 1g)
    float sum = 0.0f;
    for(u16 i = 0; i < count; i++)
    {
        sum += pSamples[i];
    }
    float mean = sum / (float)count;

    // 2. Calcula a soma dos quadrados das variacoes em torno da media (AC)
    float sumSquaresAc = 0.0f;
    for(u16 i = 0; i < count; i++)
    {
        float delta = pSamples[i] - mean;
        sumSquaresAc += (delta * delta);
    }

    // 3. Raiz da media dos quadrados
    return sqrtf(sumSquaresAc / (float)count);
}

/** @} DOXYGEN GROUP TAG END OF FILE */
