/******************************************************************************/
/**
 * @file    VibrationAnalyzer.h
 * @brief   Processamento estatistico de vibracao industrial (ISO 10816 / RMS).
 * @author  Rodrigues - Formacao em Sistemas Embarcados Virtus-CC
 * @note    Estilo de codificacao inspirado nas normas do Laboratorio ASSERT (IFPB).
 * @{
 ******************************************************************************/
#ifndef _VIBRATION_ANALYZER_H_
#define _VIBRATION_ANALYZER_H_

/*******************************************************************************
 * INCLUDES NECESSARIOS
 ******************************************************************************/
#include "AssertTypes.h"

/*******************************************************************************
 * CONFIGURACOES
 ******************************************************************************/
#define dVIB_WINDOW_SAMPLES               512
#define dVIB_DEFAULT_THRESHOLD_G          0.45f // Limiar de alarme em g RMS

/*******************************************************************************
 * ENUMS E ESTRUTURAS PUBLICAS
 ******************************************************************************/
typedef enum
{
    eVIB_SEVERITY_GOOD = 0,
    eVIB_SEVERITY_ACCEPTABLE,
    eVIB_SEVERITY_ALARM
} eVibrationSeverity_t;

typedef enum
{
    eDIAG_NORMAL = 0,
    eDIAG_BEARING_FAULT,      // Falha incipiente de rolamento (Kurtosis elevada)
    eDIAG_UNBALANCE_SEVERITY  // Desbalanceamento mecanico severo (RMS elevado)
} eDiagnosisType_t;

typedef struct
{
    float rmsX;
    float rmsY;
    float rmsZ;
    float rmsTotal;
    float crestFactor;        // Fator de Crista (Pico / RMS)
    float kurtosis;           // Curtose (4º momento estatistico)
    eVibrationSeverity_t severity;
    eDiagnosisType_t     diagnosis;
    bool  isAlarmActive;
} sVibrationResult_t;

/*******************************************************************************
 * PROTOTIPOS PUBLICOS
 ******************************************************************************/
void VibrationAnalyzer_Init(float alarmThresholdG);
void VibrationAnalyzer_AcquireWindow(void);
void VibrationAnalyzer_CalculateRms(void);
sVibrationResult_t VibrationAnalyzer_GetResults(void);
bool VibrationAnalyzer_IsAlarmTriggered(void);

#endif /* _VIBRATION_ANALYZER_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
