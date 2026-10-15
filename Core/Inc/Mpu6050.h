/******************************************************************************/
/**
 * @file    Mpu6050.h
 * @brief   Driver MPU-6050 otimizado para Baixo Consumo (Sleep/Wakeup).
 * @author  Rodrigues - Formacao em Sistemas Embarcados Virtus-CC
 * @note    Estilo de codificacao inspirado nas normas do Laboratorio ASSERT (IFPB).
 * @{
 ******************************************************************************/
#ifndef _MPU6050_H_
#define _MPU6050_H_

/*******************************************************************************
 * INCLUDES NECESSARIOS
 ******************************************************************************/
#include "AssertTypes.h"

/*******************************************************************************
 * CONFIGURACOES
 ******************************************************************************/
#define dMPU6050_I2C_ADDR                 0x68
#define dMPU6050_WHO_AM_I_VAL             0x68

/*******************************************************************************
 * PROTOTIPOS PUBLICOS
 ******************************************************************************/
bool Mpu6050_Init(void);
bool Mpu6050_SetSleepMode(bool enableSleep);
bool Mpu6050_ReadRawAccel(s16 *pAx, s16 *pAy, s16 *pAz);
bool Mpu6050_ReadScaledAccelG(float *pAx, float *pAy, float *pAz);

#endif /* _MPU6050_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
