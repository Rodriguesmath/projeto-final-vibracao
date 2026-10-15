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
static float VibrationAnalyzer_ComputeAxisPeak(const float *pSamples, u16 count);
static float VibrationAnalyzer_ComputeAxisKurtosis(const float *pSamples, u16 count, float rms);

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

    // Calcula Pico e Fator de Crista no eixo dominante (normalmente Z ou eixo com maior RMS)
    float peakX = VibrationAnalyzer_ComputeAxisPeak(vibrationAnalyzer.samplesX, dVIB_WINDOW_SAMPLES);
    float peakY = VibrationAnalyzer_ComputeAxisPeak(vibrationAnalyzer.samplesY, dVIB_WINDOW_SAMPLES);
    float peakZ = VibrationAnalyzer_ComputeAxisPeak(vibrationAnalyzer.samplesZ, dVIB_WINDOW_SAMPLES);
    float maxPeak = fmaxf(peakX, fmaxf(peakY, peakZ));
    float dominantRms = fmaxf(rmsX, fmaxf(rmsY, rmsZ));

    float crestFactor = (dominantRms > 0.01f) ? (maxPeak / dominantRms) : 1.414f;
    vibrationAnalyzer.lastResult.crestFactor = crestFactor;

    // Calcula Curtose (4º momento estatistico padronizado)
    float kurtZ = VibrationAnalyzer_ComputeAxisKurtosis(vibrationAnalyzer.samplesZ, dVIB_WINDOW_SAMPLES, rmsZ);
    vibrationAnalyzer.lastResult.kurtosis = kurtZ;

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

    // Diagnostico Mecanico Avancado:
    // Kurtosis > 4.5 ou Crest Factor > 3.5 indica impactos pontuais repetitivos (defeito em pista de rolamento)
    if((kurtZ > 4.5f) || (crestFactor > 3.5f))
    {
        vibrationAnalyzer.lastResult.diagnosis = eDIAG_BEARING_FAULT;
        vibrationAnalyzer.lastResult.isAlarmActive = true;
    }
    else if(vibrationAnalyzer.lastResult.severity == eVIB_SEVERITY_ALARM)
    {
        vibrationAnalyzer.lastResult.diagnosis = eDIAG_UNBALANCE_SEVERITY;
    }
    else
    {
        vibrationAnalyzer.lastResult.diagnosis = eDIAG_NORMAL;
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

    float sum = 0.0f;
    for(u16 i = 0; i < count; i++)
    {
        sum += pSamples[i];
    }
    float mean = sum / (float)count;

    float sumSquaresAc = 0.0f;
    for(u16 i = 0; i < count; i++)
    {
        float delta = pSamples[i] - mean;
        sumSquaresAc += (delta * delta);
    }

    return sqrtf(sumSquaresAc / (float)count);
}

static float VibrationAnalyzer_ComputeAxisPeak(const float *pSamples, u16 count)
{
    if((pSamples == NULL) || (count == 0))
    {
        return 0.0f;
    }

    float sum = 0.0f;
    for(u16 i = 0; i < count; i++)
    {
        sum += pSamples[i];
    }
    float mean = sum / (float)count;

    float maxAbsDelta = 0.0f;
    for(u16 i = 0; i < count; i++)
    {
        float delta = fabsf(pSamples[i] - mean);
        if(delta > maxAbsDelta)
        {
            maxAbsDelta = delta;
        }
    }

    return maxAbsDelta;
}

static float VibrationAnalyzer_ComputeAxisKurtosis(const float *pSamples, u16 count, float rms)
{
    if((pSamples == NULL) || (count == 0) || (rms < 0.005f))
    {
        return 3.0f; // Valor padrao para distribuicao normal sem defeito
    }

    float sum = 0.0f;
    for(u16 i = 0; i < count; i++)
    {
        sum += pSamples[i];
    }
    float mean = sum / (float)count;

    float sumFourthPower = 0.0f;
    for(u16 i = 0; i < count; i++)
    {
        float delta = pSamples[i] - mean;
        float delta2 = delta * delta;
        sumFourthPower += (delta2 * delta2);
    }

    float variance2 = (rms * rms) * (rms * rms);
    if(variance2 <= 0.0000001f)
    {
        return 3.0f;
    }

    return (sumFourthPower / (float)count) / variance2;
}

/** @} DOXYGEN GROUP TAG END OF FILE */
