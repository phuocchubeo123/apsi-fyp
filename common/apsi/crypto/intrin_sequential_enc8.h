/*
 * intrin_sequential_enc8.h
 * Copied and modified from Shay Gueron's code from intrinsic.h
/********************************************************************/
/* Copyright(c) 2014, Intel Corp.                                   */
/* Developers and authors: Shay Gueron (1) (2)                      */
/* (1) University of Haifa, Israel                                  */
/* (2) Intel, Israel                                                */
/* IPG, Architecture, Israel Development Center, Haifa, Israel      */
/********************************************************************/

#ifndef INTRIN_SEQUENTIAL_ENC8_H_
#define INTRIN_SEQUENTIAL_ENC8_H_
#ifdef AES256_HASH

#include <stdint.h>
#include <stdio.h>
#include <wmmintrin.h>

#if !defined (ALIGN16)
#if defined (__GNUC__)
#  define ALIGN16  __attribute__  ( (aligned (16)))
# else
#  define ALIGN16 __declspec (align (16))
# endif
#endif

#if defined(__INTEL_COMPILER)
# include <ia32intrin.h>
#elif defined(__GNUC__)
# include <emmintrin.h>
# include <smmintrin.h>
#endif

typedef struct KEY_SCHEDULE
{
	ALIGN16 unsigned char KEY[16*15];
	unsigned int nr;
} ROUND_KEYS;

#endif

#endif /*INTRIN_SEQUENTIAL_ENC8_H_*/