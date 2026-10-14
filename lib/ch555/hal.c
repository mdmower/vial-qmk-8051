/********************************** (C) COPYRIGHT *******************************
* File Name          : Debug.C
* Author             : WCH
* Version            : V1.4
* Date               : 2021/12/15
* Description        : This file contains all the functions prototypes for UART
*                      Printf , Delay functions.									 
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
********************************************************************************/
#include "config.h" // NO_PRINT
#include "hal.h"

/*******************************************************************************
*Function Name: CfgFsys
*Description  : CH555 clock selection and configuration function, reset uses internal crystal oscillator by default, Fsys=12MHz,
*               FREQ_SYS can be obtained through CLOCK_CFG configuration, the formula is as follows:
*               Fsys = (Fosc *4/(CLOCK_CFG & MASK_SYS_CK_SEL); The specific clock needs to be configured by yourself
*Input        : None
*Return       : None
*******************************************************************************/ 
void CfgFsys(void)  
{
#if OSC_EN_XT	
	P7 = P7 & 0xF0 | 0x06;                            //Before starting the external crystal oscillator, p7.0 is at low level and p7.1 is in pull-up state.
	SAFE_MOD = 0x55;
	SAFE_MOD = 0xAA;
	CLOCK_CFG |= bOSC_EN_XT;                          //Enable external crystal oscillator
	mDelaymS(10);
	SAFE_MOD = 0x55;
	SAFE_MOD = 0xAA;
	CLOCK_CFG &= ~bOSC_EN_INT;                        //Turn off the internal crystal oscillator 
	SAFE_MOD = 0x00;
#endif	
	
	SAFE_MOD = 0x55;
	SAFE_MOD = 0xAA;
#if FREQ_SYS == 48000000
#error "48 MHz is reserved for tests (datasheet table 8.2.2) and needs bLDO_CORE_VOL first"
#endif
	
#if FREQ_SYS == 32000000	
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x06;  // 32MHz	
	
#endif
	
#if FREQ_SYS == 24000000	
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x05;  // 24MHz	
	
#endif	

#if FREQ_SYS == 16000000		
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x04;  // 16MHz
	
#endif

#if FREQ_SYS == 12000000		
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x03;  // 12MHz
	
#endif	

#if FREQ_SYS == 3000000	
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x02;  // 3MHz
	
#endif

#if FREQ_SYS == 750000	
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x01;  // 750KHz
	
#endif

#if FREQ_SYS == 187500	
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x00;  // 187.5KHz	
	
#endif

	SAFE_MOD = 0x00;
}

/*******************************************************************************
* Function Name  : mDelayus
* Description    : microsecond delay function, delay in us
* Input          : n
* Return         : None
*******************************************************************************/ 
/* CH555 core cycles: NOP 1; DJNZ Rn 6 when it branches, 2 when it does not. One pass of the loop below, MDELAYUS_NOPS
 * NOPs and a DJNZ r6 that branches, takes FREQ_SYS / 1000000 cycles = 1 us. Every 256th pass takes 2 cycles more (DJNZ
 * r6 does not branch, DJNZ r7 does). Interrupts that run during the loop lengthen the delay. */
#define MDELAYUS_NOP2  "nop\n nop\n"
#define MDELAYUS_NOP8  MDELAYUS_NOP2 MDELAYUS_NOP2 MDELAYUS_NOP2 MDELAYUS_NOP2
#if FREQ_SYS == 32000000
#define MDELAYUS_NOPS  MDELAYUS_NOP8 MDELAYUS_NOP8 MDELAYUS_NOP8 MDELAYUS_NOP2                              /* 26 */
#elif FREQ_SYS == 24000000
#define MDELAYUS_NOPS  MDELAYUS_NOP8 MDELAYUS_NOP8 MDELAYUS_NOP2                                            /* 18 */
#elif FREQ_SYS == 16000000
#define MDELAYUS_NOPS  MDELAYUS_NOP8 MDELAYUS_NOP2                                                          /* 10 */
#elif FREQ_SYS == 12000000
#define MDELAYUS_NOPS  MDELAYUS_NOP2 MDELAYUS_NOP2 MDELAYUS_NOP2                                            /* 6 */
#else
#error "mDelayuS: no loop for this FREQ_SYS"
#endif

void mDelayuS(UINT16 n) __naked
{
	(void)n;                    /* in DPL (low byte) and DPH */
	/* SDCC saves no registers around a call of a naked function defined in the same file (mDelaymS) */
	__asm__("push ar6");
	__asm__("push ar7");
	__asm__("mov  a,dpl");
	__asm__("mov  r6,a");
	__asm__("orl  a,dph");
	__asm__("jz   00002$");     /* n = 0 */
	__asm__("mov  r7,dph");
	__asm__("mov  a,r6");
	__asm__("jz   00001$");
	__asm__("inc  r7");         /* low byte not 0: the first r6 round has that many passes, the others 256 */
	__asm__("00001$:");
	__asm__(MDELAYUS_NOPS);
	__asm__("djnz r6,00001$");
	__asm__("djnz r7,00001$");
	__asm__("00002$:");
	__asm__("pop  ar7");
	__asm__("pop  ar6");
	__asm__("ret");
}

/*******************************************************************************
* Function Name  : mDelayms
* Description    : Ms delay function, delay in ms
* Input          : n
* Return         : None
*******************************************************************************/
void mDelaymS(UINT16 n)                                                    
{
	while(n){
		mDelayuS(1000);
		-- n;
	}
}                                         

#ifndef NO_PRINT /* the UART is the print output; ch555_usb_stack.c calls mInitSTDIO() only without NO_PRINT */
#ifdef UART0_PRINTF
/*******************************************************************************
* Function Name  : CH555UART0Alter
* Description    : CH555 serial port 0 pin mapping, mapped to p0.2 and p0.3
* Input          : None
* Return         : None
*******************************************************************************/
void CH555UART0Alter(void)
{
	PIN_FUNC |= bUART0_PIN_X;                                                 
}

/*******************************************************************************
* Function Name  : mInitSTDIO
* Description    : Ch555 serial port 0 is initialized, 
*                  t1 is used as the baud rate generator by default, t2 can also be used
*                  -T1 acts as baud rate generator for uart0: RCLK=0 and TCLK=0
*                  -T2 acts as baud rate generator for uart0: RCLK=1 or TCLK=1               
* Input          : None
* Return         : None
*******************************************************************************/
void mInitSTDIO(void)
{
	UINT32 x;
	UINT8 x2; 

/*Serial port 0 usage mode 1*/
	SM0 = 0;
	SM1 = 1;
	SM2 = 0;                                                                 
	
/*Use Timer1 as baud rate generator*/
	RCLK = 0;                                                                  //Uart0 receive clock
	TCLK = 0;                                                                  //Uart0 send clock
	
	PCON |= SMOD;
	x = 10 * FREQ_SYS / UART0_BAUD / 16;                                       //If you change the main frequency, be careful not to overflow the value of x                      
	x2 = x % 10;
	x /= 10;
	if(x2 >= 5) x++;                                                           //rounding

	TMOD = TMOD & ~ bT1_GATE & ~ bT1_CT & ~ MASK_T1_MOD | bT1_M1;              //0X20, Timer1 as 8-bit auto-reload timer
	T2MOD = T2MOD | bTMR_CLK | bT1_CLK;                                        //Timer1 clock selection
	TH1 = 0-x;                                                                 //12mhz crystal oscillator, buad/12 is the actual baud rate that needs to be set
	TR1 = 1;                                                                   //Start timer 1
	TI = 1;
	REN = 1;                                                                   //Serial port 0 receive enable
}

/*******************************************************************************
* Function Name  : getchar
* Description    : Ch555 uart0 receives a byte
* Input          : None
* Return         : SBUF
*******************************************************************************/
int getchar(void) {
	while (!RI);//Query reception, interrupt mode is not required
	RI = 0;
	return (int)SBUF;
}

/*******************************************************************************
* Function Name  : putchar
* Description    : Ch555 uart0 sends a byte
* Input          : c: data to send
* Return         : None
*******************************************************************************/
int putchar(int c) {
	while (!TI);//For query sending, the following two statements are not needed in the interrupt mode, but ti=0 is required before sending.
	TI = 0;
	SBUF = c & 0xff;
	return c;
}



#elif defined(UART1_PRINTF)
void CH555UART1Alter(void) {
    P1_MOD_OC |= (3<<6);
    P1_DIR_PU |= (3<<6);
	PIN_FUNC |= bUART1_PIN_X;
}
void mInitSTDIO(void) {
  SCON1 &= ~bU1SM0;
  SCON1 |= bU1REN | bU1SMOD;

	UINT32 x;
	UINT8 x2; 
	x = 10 * FREQ_SYS / UART1_BAUD / 16;                                       //If you change the main frequency, be careful not to overflow the value of x                      
	x2 = x % 10;
	x /= 10;
	if(x2 >= 5) x++;                                                           //rounding
    SBAUD1 = 0 - x;

  SIF1 = bU1TI | bU1RI; // clear interrupt flags
  CH555UART1Alter(); 
}

int getchar(void) {
	while (!(SIF1&bU1RI));//Query reception, interrupt mode is not required
	SIF1  = bU1RI;
	return (int)SBUF1;
}

int putchar(int c) {
	SBUF1 = c & 0xff;
	while (!(SIF1&bU1TI));//For query sending, the following two statements are not needed in the interrupt mode, but ti=0 is required before sending.
	SIF1  = bU1TI;
	return c;
}
#endif
#endif /* NO_PRINT */
