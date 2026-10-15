/******************************************************************************/
/**
 * @file    Mpu6050.c
 * @brief   Implementacao do Driver MPU-6050 com controle de baixo consumo.
 * @author  Rodrigues - Formacao em Sistemas Embarcados Virtus-CC
 * @note    Estilo de codificacao inspirado nas normas do Laboratorio ASSERT (IFPB).
 * @{
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "Mpu6050.h"
#include "Bsp.h"

/*******************************************************************************
 * DEFINES LOCAIS
 ******************************************************************************/
#define dREG_SMPLRT_DIV                   0x19
#define dREG_CONFIG                       0x1A
#define dREG_ACCEL_CONFIG                 0x1C
#define dREG_ACCEL_XOUT_H                 0x3B
#define dREG_PWR_MGMT_1                   0x6B
#define dREG_WHO_AM_I                     0x75

#define dACCEL_SENSITIVITY_2G             16384.0f

/*******************************************************************************
 * CONSTANTES
 ******************************************************************************/

/*******************************************************************************
 * ESTRUTURAS DE DADOS LOCAIS
 ******************************************************************************/
static struct
{
    bool isReady;
    bool isSleeping;
} mpu6050;

/*******************************************************************************
 * PROTOTIPOS LOCAIS
 ******************************************************************************/

/*******************************************************************************
 * FUNCOES PUBLICAS
 ******************************************************************************/

bool Mpu6050_Init(void)
{
    u8 whoAmI = 0;

    mpu6050.isReady = false;
    mpu6050.isSleeping = false;

    // Le registrador de identificacao WHO_AM_I
    if(Bsp_I2cReadRegisters(dMPU6050_I2C_ADDR, dREG_WHO_AM_I, &whoAmI, 1) == false)
    {
        return false;
    }

    if(whoAmI != dMPU6050_WHO_AM_I_VAL)
    {
        return false;
    }

    // Acorda o sensor para configuracao (seleciona PLL eixo X como clock de referencia)
    Bsp_I2cWriteRegister(dMPU6050_I2C_ADDR, dREG_PWR_MGMT_1, 0x01);
    Bsp_DelayMs(10);

    // Configura filtro passa-baixa digital (DLPF = 44 Hz, amostragem base = 1 kHz)
    Bsp_I2cWriteRegister(dMPU6050_I2C_ADDR, dREG_CONFIG, 0x03);

    // Divisor de taxa: 1 kHz / (1 + 3) = 250 Hz de taxa de saida do sensor
    Bsp_I2cWriteRegister(dMPU6050_I2C_ADDR, dREG_SMPLRT_DIV, 0x03);

    // Escala do acelerometro: +/- 2g
    Bsp_I2cWriteRegister(dMPU6050_I2C_ADDR, dREG_ACCEL_CONFIG, 0x00);

    // Coloca imediatamente o sensor em modo de baixo consumo (Sleep)
    Mpu6050_SetSleepMode(true);

    mpu6050.isReady = true;
    return true;
}

bool Mpu6050_SetSleepMode(bool enableSleep)
{
    bool success = false;

    if(enableSleep == true)
    {
        // Seta o bit 6 (SLEEP) em PWR_MGMT_1: sensor consome apenas ~5 uA
        success = Bsp_I2cWriteRegister(dMPU6050_I2C_ADDR, dREG_PWR_MGMT_1, 0x40);
        if(success == true)
        {
            mpu6050.isSleeping = true;
        }
    }
    else
    {
        // Limpa bit SLEEP e religa PLL (reconsome ~3.5 mA para medir)
        success = Bsp_I2cWriteRegister(dMPU6050_I2C_ADDR, dREG_PWR_MGMT_1, 0x01);
        if(success == true)
        {
            mpu6050.isSleeping = false;
            // Tempo para estabilizacao dos osciladores internos e DLPF
            Bsp_DelayMs(30);
        }
    }

    return success;
}

bool Mpu6050_ReadRawAccel(s16 *pAx, s16 *pAy, s16 *pAz)
{
    u8 buffer[6];

    if((pAx == NULL) || (pAy == NULL) || (pAz == NULL))
    {
        return false;
    }

    if(Bsp_I2cReadRegisters(dMPU6050_I2C_ADDR, dREG_ACCEL_XOUT_H, buffer, 6) == false)
    {
        return false;
    }

    *pAx = (s16)((buffer[0] << 8) | buffer[1]);
    *pAy = (s16)((buffer[2] << 8) | buffer[3]);
    *pAz = (s16)((buffer[4] << 8) | buffer[5]);

    return true;
}

bool Mpu6050_ReadScaledAccelG(float *pAx, float *pAy, float *pAz)
{
    s16 rawX = 0;
    s16 rawY = 0;
    s16 rawZ = 0;

    if(Mpu6050_ReadRawAccel(&rawX, &rawY, &rawZ) == false)
    {
        return false;
    }

    if((pAx != NULL) && (pAy != NULL) && (pAz != NULL))
    {
        *pAx = (float)rawX / dACCEL_SENSITIVITY_2G;
        *pAy = (float)rawY / dACCEL_SENSITIVITY_2G;
        *pAz = (float)rawZ / dACCEL_SENSITIVITY_2G;
        return true;
    }

    return false;
}

/*******************************************************************************
 * FUNCOES LOCAIS
 ******************************************************************************/

/** @} DOXYGEN GROUP TAG END OF FILE */
