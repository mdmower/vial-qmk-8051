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

//#include <absacc.h>
#include <stdio.h>
#include <string.h>
//#include <intrins.h>
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
#ifndef  DE_PRINTF                                            //Debug print
#define  DE_PRINTF                      1
#endif
#define UART1_PRINTF

#ifndef MIN
#define MIN(a,b)                (((a) <= (b)) ? (a) : (b))
#endif

void CfgFsys(void);                                           //CH5555Ê±ÖÓÑ¡ÔñºÍÅäÖĂ

void mDelayuS(UINT16 n);                                 //̉ÔuSÎªµ¥Î»ÑÓÊ±
void mDelaymS(UINT16 n);                                 //̉ÔmSÎªµ¥Î»ÑÓÊ±

#ifdef UART0_PRINTF
void mInitSTDIO(void);                                        //T1×÷Îª²῭ØÂÊ·¢ÉúÆ÷
void CH555UART0Alter(void);                                    //CH555´®¿Ú0̉ư½ÅÓ³Éäµ½P0.2/P0.3
//void CH555UART0SendByte(UINT8 SendDat);                    //CH555UART0·¢ËÍ̉»¸ö×Ö½Ú
//UINT8 CH555UART0RcvByte(void);                                //CH555UART0²éÑ¯·½Ê½½ÓÊỞ»¸ö×Ö½Ú
int putchar(int c);
int getchar(void);
#elif defined(UART1_PRINTF)
void mInitSTDIO(void);                                        //T1×÷Îª²῭ØÂÊ·¢ÉúÆ÷
int putchar(int c);
int getchar(void);
#endif

void CH555WDTModeSelect(UINT8 mode);                       //CH555¿´ĂÅ¹·Ä£Ê½ÉèÖĂ 
void CH555WDTFeed(UINT8 tim);                              //CH555¿´ĂÅ¹·Î¹¹·
void CH555SoftReset(void);                                    //CH555Èí¸´Î»
  
#endif
