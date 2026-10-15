/******************************************************************************/
/**
 * @file    AssertTypes.h
 * @brief   Definicoes padronizadas de tipos primitivos (Normas ASSERT - IFPB).
 * @author  Rodrigues - Formacao em Sistemas Embarcados Virtus-CC
 * @note    Estilo de codificacao inspirado nas normas do Laboratorio ASSERT (IFPB).
 * @{
 ******************************************************************************/
#ifndef _ASSERT_TYPES_H_
#define _ASSERT_TYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*******************************************************************************
 * TIPOS INTEIROS PADRONIZADOS
 ******************************************************************************/
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;

typedef int8_t    s8;
typedef int16_t   s16;
typedef int32_t   s32;
typedef int64_t   s64;

/*******************************************************************************
 * TIPOS DE PONTO FLUTUANTE
 ******************************************************************************/
typedef float     f32;
typedef double    f64;

#endif /* _ASSERT_TYPES_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
