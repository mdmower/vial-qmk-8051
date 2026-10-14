/********************************** (C) COPYRIGHT *******************************
* File Name          : Debug.H
* Author             : WCH
* Version            : V1.0
* Date               : 2020/07/03
* Description        : This file contains all the functions prototypes for UART
*                      Printf , Delay functions.									 
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
********************************************************************************/
#ifndef	__HAL_H__
#define __HAL_H__

#include <stdio.h>
#include <string.h>
#include "ch555.h"


#ifndef  SUCCESS
#define  SUCCESS    0
#endif
#ifndef  FAIL
#define  FAIL       0xFF
#endif


#define	 FREQ_SYS	                     F_CPU	          //System clock Fsys
#define  OSC_EN_XT                       0                    //External crystal oscillator enable control
#ifndef  UART0_BAUD
#define  UART0_BAUD                      115200
#endif
#ifndef  UART1_BAUD
#define  UART1_BAUD                      115200
#endif
#define UART1_PRINTF

#ifndef MIN
#define MIN(a,b)                (((a) <= (b)) ? (a) : (b))
#endif

void CfgFsys(void);                                           //CH555 clock selection and configuration

void mDelayuS(UINT16 n);                                      //delay in us
void mDelaymS(UINT16 n);                                      //delay in ms

#ifdef UART0_PRINTF
void mInitSTDIO(void);                                        //UART0 init, Timer1 as the baud rate generator
void CH555UART0Alter(void);                                   //UART0 pins mapped to P0.2/P0.3
int putchar(int c);
int getchar(void);
#elif defined(UART1_PRINTF)
void mInitSTDIO(void);                                        //UART1 init, baud rate from SBAUD1
int putchar(int c);
int getchar(void);
#endif

  
#endif
