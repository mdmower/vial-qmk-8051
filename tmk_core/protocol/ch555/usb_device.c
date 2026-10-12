/********************************** (C) COPYRIGHT *******************************
* File Name          : USB_DEVICE.C
* Author             : MJX
* Version            : V1.00
* Date               : 2021/04/29
* Description        : Usb device underlying operation related functions
*******************************************************************************/



/******************************************************************************/
#include <string.h>
#include "hal.h"
#include "debug.h"
#include "usb_device.h"
#include "suspend.h"
#include "report.h"

/******************************************************************************/
/* constant variable definition */
volatile UINT8	D0SetupReqCode = 0xFF;											/* USB Setup package request code */
volatile UINT16	D0SetupLen = 0x00;												/* USB Setup packet length */
volatile PUINT8C	pD0Descr;   /* points into __code Flash — MUST be PUINT8C on SDCC/8051 */

volatile UINT8  D0UsbConfig = 0x00;												/* USB configuration flags - the Configuation Id selected */
volatile UINT8  Report_Value = 0x01;                                            /* HID protocol of interface 0 (SET_PROTOCOL): 0 boot, 1 report (the default, HID 1.11 7.2.6) */
volatile UINT8  Idle_Value = 0x00;                                              /* host request hid interface go idle */
volatile UINT8  KB_USB_SetReport = 0x00;										/* */

volatile UINT8  USB_EnumStatus = 0x00;											/* USB enumeration status */	
volatile UINT8  USB_SleepStatus = 0x00;											/* USB sleep state */		
volatile UINT8  MCU_Sleep_Operate = 0x00;										/* Mcu sleep operation flag */
volatile UINT8  USB_WakeUpSent = 0x00;											/* remote wakeup signalled, no host transfer since */

volatile UINT8  KB_USB_UpStatus = 0x00;											/* endpoint 1 busy flag*/
#ifdef USE_D0_EP1_OUT
volatile UINT8  ep1_data_wait   = 0x00;											/* endpoint 1 data waiting flag*/
#endif
#ifdef USE_D0_EP2_OUT
volatile UINT8  ep2_data_wait   = 0x00;											/* endpoint 2 data waiting flag*/
#endif
#ifdef USE_D0_EP3_OUT
volatile UINT8  ep3_data_wait   = 0x00;											/* endpoint 3 data waiting flag*/
#endif
#ifdef USE_D0_EP4_OUT
volatile UINT8  ep4_data_wait   = 0x00;											/* endpoint 4 data waiting flag*/
#endif

//UINT8D  RGB_Mode = 0x00;
void USB_EP_init( void )  
{
    /* No bUEP_X_AUTO_TOG; the interrupt toggles every endpoint. IN: with the auto-toggle, a completed
     * transaction on one sub-device also flipped the IN toggle of the other sub-devices' auto-toggle
     * endpoints (seen on the VS11K33A, hub layout), and the next report repeated the last toggle and was
     * discarded by the host. OUT: the interrupt takes a packet only with bUXS_TOG_OK; a packet without it
     * repeats the previous one (its ACK was lost) and is dropped (USB 2.0 8.6.4, WCH's CompatibilityHID.C). */

    D0_EP0RES = UEP_R_RES_ACK | UEP_T_RES_NAK;  /* toggle bits cleared (DATA0) */
    D0_EP_MOD = bUX_DEV_EN;
    D1_EP_MOD = 0;              /* D1 and D2 are not used */
    D2_EP_MOD = 0;
#ifdef USE_D0_EP1_IN
    D0_EP1RES = UEP_X_RES_NAK;  /* DATA0, NAK until we have data */
#endif
#ifdef USE_D0_EP2_IN
    D0_EP2RES = UEP_X_RES_NAK;
#endif
#ifdef USE_D0_EP3_IN
    D0_EP3RES = UEP_X_RES_NAK; 
#endif
#ifdef USE_D0_EP5_IN
	D0_EP_MOD |= bUX_EP5I_EN;
    D0_EP5RES = UEP_X_RES_NAK; 
#endif
#ifdef USE_D0_EP6_IN
	D0_EP_MOD |= bUX_EP6I_EN;
    D0_EP6RES = UEP_X_RES_NAK; 
#endif

#ifdef USE_D0_EP1_OUT
    D0_EP_MOD |= bUX_EP1O_EN;
    D0_EP1RES = UEP_X_RES_ACK;  /* toggle bit cleared (DATA0), ACK ready to receive */
#endif
#ifdef USE_D0_EP2_OUT
    D0_EP_MOD |= bUX_EP2O_EN;
    D0_EP2RES = UEP_X_RES_ACK;
#endif
#ifdef USE_D0_EP3_OUT
    D0_EP_MOD |= bUX_EP3O_EN;
    D0_EP3RES = UEP_X_RES_ACK; 
#endif
#ifdef USE_D0_EP4_OUT
    D0_EP_MOD |= bUX_EP4O_EN;
    D0_EP4RES = UEP_X_RES_ACK; 
#endif


    /* Clear all endpoint transmit lengths */
    D0_EP0T_L = 0;
#ifdef USE_D0_EP1_IN
    D0_EP1T_L = 0;
#endif
#ifdef USE_D0_EP2_IN
    D0_EP2T_L = 0;
#endif
#ifdef USE_D0_EP3_IN
    D0_EP3T_L = 0;
#endif
#ifdef USE_D0_EP5_IN
    D0_EP5T_L = 0;
#endif
#ifdef USE_D0_EP6_IN
    D0_EP6T_L = 0;
#endif
}
/*******************************************************************************
* Function Name  : USB_Device_Init
* Description    : CH55X USB device initialization
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/ 
void USB_Device_Init( void )  
{
	/* Initialize related variables */
	D0SetupReqCode = 0xFF;														/* USB Setup package request code */
	D0SetupLen = 0x00;															/* USB Setup packet length */
	D0UsbConfig = 0x00;															/* USB configuration flags */
	USB_EnumStatus = 0x00;														/* USB enumeration status */	
	USB_SleepStatus = 0x00;														/* USB sleep state */		
	MCU_Sleep_Operate = 0x00;													/* Mcu sleep operation flag */
	USB_WakeUpSent = 0x00;														/* remote wakeup signalled, no host transfer since */
	KB_USB_UpStatus = 0x00;														/* Usb keyboard and mouse upload status */
#ifdef USE_D0_EP1_OUT
	ep1_data_wait   = 0x00;														/* Usb keyboard and mouse upload status */
#endif
#ifdef USE_D0_EP2_OUT
	ep2_data_wait   = 0x00;														/* Usb keyboard and mouse upload status */
#endif
#ifdef USE_D0_EP3_OUT
	ep3_data_wait   = 0x00;														/* Usb keyboard and mouse upload status */
#endif
#ifdef USE_D0_EP4_OUT
	ep4_data_wait   = 0x00;
#endif
	KB_USB_SetReport = 0x00;													/* Usb keyboard set report value */  

	/* Initialize usb related registers */
	IE_USB = 0;                              // usb interrupt dissable, avoid acidental interrupt during init
	IP_EX &= ~bIP_USB;                       // set usb interrupt priority to high
    
	//SAFE_MOD = 0x55;
	//SAFE_MOD = 0xAA;
	//GLOBAL_CFG |= bXIR_XSFR;               // use __pdata to access xSFR instead of pRAM        

	SAFE_MOD = 0x55;
	SAFE_MOD = 0xAA;
	GLOBAL_CFG &= ~bWDOG_EN;                 // disable watch-dog reset
	SAFE_MOD = 0x00;

	USB_CTRL = bUX_RESET_SIE | bUX_CLR_ALL; // pulse reset — flush SIE and FIFOs
	USB_CTRL = 0;                            // clear reset

    USB_EP_init();                           // setup endpoints
//	D0_EP0RES = UEP_R_RES_ACK | UEP_T_RES_NAK;
//	D0_EP1RES = bUEP_X_AUTO_TOG | UEP_X_RES_NAK;
//	D0_EP2RES = bUEP_X_AUTO_TOG | UEP_X_RES_ACK;
//	D0_EP3RES = bUEP_X_AUTO_TOG | UEP_X_RES_NAK;
	//D0_EP_MOD = bUX_EP3O_EN | bUX_DEV_EN; 
	//D0_EP_MOD |= bUX_EP5I_EN;

	D0_ADDR = 0;
	HB_ADDR = 0x7F;             /* hub off: the controller passes everything to D0 (datasheet 16.1, WCH's examples) */
	USB_IF = 0xFF;
	USB_IE = bUX_IE_SUSPEND | bUX_IE_TRANSFER | bUX_IE_BUS_RST;
   	USB_CTRL = bUX_DP_PU_EN | bUX_INT_BUSY; // pull-up + auto-NAK while ISR runs
	IE_USB = 1;                              // usb interrupt enable
}

/*******************************************************************************
* Function Name  : USB_DeviceInterrupt
* Description    : CH55X Usb device interrupt handler function
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/ 
#pragma save
#pragma nooverlay
void USB_DeviceInterrupt( void ) __interrupt(INT_NO_USB) //__using(1)
{
    UINT8  save = USB_IE;
    USB_IE = 0;
    UINT8  us,i;
	UINT16 len;

USB_DevIntNext:
	us = USB_IF; 
	if( us & ( bUX_IF_D0_TRANS | bUX_IF_BUS_RST ) )
	{
		/* The host is running the bus again (see USB_WakeUp_PC) */
		USB_WakeUpSent = 0x00;
	}
	if( us & bUX_IF_D0_TRANS )
	{
		//dprintf("%x\n",SP);
		//dprint("d0tr");
		/* D0 data transfer completed */
		
		/* A SETUP that arrives while another D0 completion is pending sets bUXS_SETUP_ACT and leaves MASK_UXS_TOKEN
		 * and MASK_UXS_ENDP at that completion (a SETUP never changes MASK_UXS_ENDP). The completion is handled
		 * first. After D0_STATUS = 0 the controller sets bUX_IF_D0_TRANS again with bUXS_SETUP_ACT and token FREE,
		 * and the next pass (USB_DevIntNext) handles the SETUP (datasheet, D0_STATUS; seen on the K580). */
		switch( D0_STATUS & ( MASK_UXS_TOKEN | MASK_UXS_ENDP ) )     
		{
			/* Analyze action tokens and endpoint numbers */

		case UXS_TOKEN_IN | 0:
            	/* endpoint 0# IN */
			if( D0_STATUS & bUXS_SETUP_ACT )
			{
				/* A SETUP followed: it ends this control transfer, and its packet is in the EP0 buffer */
				break;
			}
			switch( D0SetupReqCode ) 
			{
				case USB_GET_DESCRIPTOR:
				{
					/* SDCC/8051: pD0Descr is __code space. memcpy() with a generic
					 * pointer silently reads from the wrong address space after the
					 * first 64-byte chunk. Use an explicit byte loop instead. */
					UINT8 i;
					len = D0SetupLen >= DEF_ENDP0_SIZE ? DEF_ENDP0_SIZE : D0SetupLen;
					for( i = 0; i < len; i++ ) {
						pD0_EP0_BUF[i] = pD0Descr[i];   /* __code read — correct on SDCC */
					}
					D0SetupLen -= len;
					pD0Descr   += len;
					D0_EP0T_L   = len;
					D0_EP0RES  ^= bUEP_T_TOG;
					break;
				}
					
				case USB_SET_ADDRESS:
					D0_ADDR = D0SetupLen;
					D0_EP0RES = UEP_R_RES_ACK | UEP_T_RES_NAK;
					break;

				default:
					/* The status phase is completed and interrupted or the 0-length data packet is forced to be uploaded to end the control transmission. */
					D0_EP0T_L = 0;  
					D0_EP0RES = UEP_R_RES_ACK | UEP_T_RES_NAK;
					break;
			}
                break;

#ifdef USE_D0_EP4_OUT
			case UXS_TOKEN_OUT | 4:  
				/*  endpoint 4# download */
				if( D0_STATUS & bUXS_TOG_OK )
				{
					/* a new packet; without bUXS_TOG_OK it repeats the previous one and is dropped */
					D0_EP4RES ^= bUEP_X_TOG;
					D0_EP4RES = D0_EP4RES & ~MASK_UEP_X_RES | UEP_X_RES_NAK; 		/* Pause download */
					ep4_data_wait = 0x01;
				}
                break; 
#endif
#ifdef USE_D0_EP3_OUT
			case UXS_TOKEN_OUT | 3:  
				/*  endpoint 3# download */
				if( D0_STATUS & bUXS_TOG_OK )
				{
					/* a new packet; without bUXS_TOG_OK it repeats the previous one and is dropped */
					D0_EP3RES ^= bUEP_X_TOG;
					D0_EP3RES = D0_EP3RES & ~MASK_UEP_X_RES | UEP_X_RES_NAK; 		/* Pause download */
					ep3_data_wait = 0x01;
				}
                break; 
#endif
#ifdef USE_D0_EP2_OUT
			case UXS_TOKEN_OUT | 2:  
				/*  endpoint 2# download */
				if( D0_STATUS & bUXS_TOG_OK )
				{
					/* a new packet; without bUXS_TOG_OK it repeats the previous one and is dropped */
					D0_EP2RES ^= bUEP_X_TOG;
					D0_EP2RES = D0_EP2RES & ~MASK_UEP_X_RES | UEP_X_RES_NAK; 		/* Pause download */
					ep2_data_wait = 0x01;
				}
                break; 
#endif
#ifdef USE_D0_EP1_OUT
			case UXS_TOKEN_OUT | 1:  
				/*  endpoint 1# download */
				if( D0_STATUS & bUXS_TOG_OK )
				{
					/* a new packet; without bUXS_TOG_OK it repeats the previous one and is dropped */
					D0_EP1RES ^= bUEP_X_TOG;
					D0_EP1RES = D0_EP1RES & ~MASK_UEP_X_RES | UEP_X_RES_NAK; 		/* Pause download */
					ep1_data_wait = 0x01;
				}
                break; 
#endif
                          
                          
                          
                          
#ifdef USE_D0_EP6_IN
            case UXS_TOKEN_IN | 6:  
				/* endpoint 6# upload completed */            	
				D0_EP6RES ^= bUEP_X_TOG;
				D0_EP6RES = D0_EP6RES & ~MASK_UEP_X_RES | UEP_X_RES_NAK; 		/* Pause upload */
                break;
#endif 

#ifdef USE_D0_EP5_IN
            case UXS_TOKEN_IN | 5:  
				/* endpoint 5# upload completed */            	
				D0_EP5RES ^= bUEP_X_TOG;
				D0_EP5RES = D0_EP5RES & ~MASK_UEP_X_RES | UEP_X_RES_NAK; 		/* Pause upload */
                break;
#endif 

#ifdef USE_D0_EP3_IN
            case UXS_TOKEN_IN | 3:  
				/* endpoint 3# upload completed */            	
				D0_EP3RES ^= bUEP_X_TOG;
				D0_EP3RES = D0_EP3RES & ~MASK_UEP_X_RES | UEP_X_RES_NAK; 		/* Pause upload */
                break;
#endif
                
#ifdef USE_D0_EP2_IN
            case UXS_TOKEN_IN | 2:  
				/* endpoint 2# upload completed */            	
				D0_EP2RES ^= bUEP_X_TOG;
				D0_EP2RES = D0_EP2RES & ~MASK_UEP_X_RES | UEP_X_RES_NAK; 		/* Pause upload */
                break;
#endif
                
#ifdef USE_D0_EP1_IN
            case UXS_TOKEN_IN | 1:  
            	/* endpoint 1# upload completed */
                D0_EP1RES ^= bUEP_X_TOG;
                D0_EP1RES = D0_EP1RES & ~MASK_UEP_X_RES | UEP_X_RES_NAK; 		/* Pause upload */
				KB_USB_UpStatus = 0x01;
                break;
#endif
               
		case UXS_TOKEN_OUT | 0:  
 				/* endpoint 0# OUT */
				if( D0_STATUS & bUXS_SETUP_ACT )
				{
					/* A SETUP followed: it ends this control transfer, and its packet is in the EP0 buffer */
					break;
				}
				switch( D0SetupReqCode ) 
				{
					case HID_SET_REPORT:
						/* LED output report: 1 byte from the keyboard interface, or report ID and 1 byte from
						 * the shared interface (NKRO) */
						len = D0_RX_LEN;
						if( len == 1 )
						{
							KB_USB_SetReport = pD0_EP0_BUF[ 0 ];
						}
						else if( len == 2 && ( pD0_EP0_BUF[ 0 ] == REPORT_ID_KEYBOARD || pD0_EP0_BUF[ 0 ] == REPORT_ID_NKRO ) )
						{
							KB_USB_SetReport = pD0_EP0_BUF[ 1 ];
						}
						D0_EP0RES ^= bUEP_R_TOG;
                        break;
                        
					case USB_GET_DESCRIPTOR:
					
					default:
						D0_EP0RES = UEP_R_RES_ACK | UEP_T_RES_NAK;  
						break;
				}
                break;
			
			default:
					if( ( D0_STATUS & ( bUXS_SETUP_ACT | MASK_UXS_TOKEN ) ) != ( bUXS_SETUP_ACT | UXS_TOKEN_FREE ) )
					{
						/* not a SETUP: a completion without a case above */
						break;
					}
					/* endpoint 0# SETUP */
					len = 0;  														/* Defaults to success and uploading 0 length */ 
					D0_EP0RES = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;

					D0SetupLen = pD0_SETUP_REQ->wLengthL + ( (UINT16)pD0_SETUP_REQ->wLengthH << 8 );
					D0SetupReqCode = pD0_SETUP_REQ->bRequest;
					
					/* Analyze the current setup package */				                
					if( ( pD0_SETUP_REQ->bRequestType & USB_REQ_TYP_MASK ) == USB_REQ_TYP_STANDARD )
					{ 
						/* Standard request */                                   
						switch( D0SetupReqCode ) 
						{  
							case USB_GET_DESCRIPTOR:            
								// Get usb descriptor 
								
								switch( pD0_SETUP_REQ->wValueH )
								{
									case USB_DESCR_TYP_DEVICE:  
										// USB device descriptor 
										pD0Descr = (PUINT8C)( &DeviceDescriptor );
										len = sizeof( USB_Descriptor_Device_t );
										break;
										
									case USB_DESCR_TYP_CONFIG:  
										// USB configuration descriptor 
										pD0Descr = (PUINT8C)( &ConfigurationDescriptor );
										len = sizeof( USB_Descriptor_Configuration_t );
										break;
										
									case USB_DESCR_TYP_STRING:  
										// USB string descriptor 
										switch( pD0_SETUP_REQ->wValueL ) 
										{
											case 1:
												pD0Descr = (PUINT8C)( &ManufacturerString );
												len = ManufacturerString.Header.Size;
												break;
																			
											case 2:
												pD0Descr = (PUINT8C)( &ProductString );
												len = ProductString.Header.Size;
												break;
												
											case 0:
												pD0Descr = (PUINT8C)( &LanguageString );
												len = LanguageString.Header.Size;
												break;
											#if defined(SERIAL_NUMBER)
											case 3:
												pD0Descr = (PUINT8C)( &SerialNumberString );
												len = SerialNumberString.Header.Size;
												break;
											#endif
												
											default:
												// Unsupported string descriptor 
												len = 0xFFFF;  
											break;
										}
										break;

									case USB_DESCR_TYP_HID:
										switch( pD0_SETUP_REQ->wIndexL )
										{       
											// Select interface 
											//case 0:
#ifndef KEYBOARD_SHARED_EP
											case KEYBOARD_INTERFACE:
												pD0Descr = (PUINT8C)( &ConfigurationDescriptor.Keyboard_HID );        
												len = 9;
												break;										
#endif
											
#if defined(MOUSE_ENABLE) && !defined(MOUSE_SHARED_EP)
                							case MOUSE_INTERFACE:
                    							pD0Descr = &ConfigurationDescriptor.Mouse_HID;
                    							len    = sizeof(USB_HID_Descriptor_HID_t);
                    							break;
#endif

#ifdef SHARED_EP_ENABLE
											case SHARED_INTERFACE:
												pD0Descr = (PUINT8C)( &ConfigurationDescriptor.Shared_HID );
												len = 9;
												break;
#endif

											//case 1:
#ifdef RAW_ENABLE
											case RAW_INTERFACE:
												pD0Descr = (PUINT8C)( &ConfigurationDescriptor.Raw_HID );
												len = sizeof(USB_HID_Descriptor_HID_t);
												break;
#endif

#ifdef CONSOLE_ENABLE
                case CONSOLE_INTERFACE:
                    pD0Descr = &ConfigurationDescriptor.Console_HID;
                    len    = sizeof(USB_HID_Descriptor_HID_t);
                    break;
#endif
#if defined(JOYSTICK_ENABLE) && !defined(JOYSTICK_SHARED_EP)
                case JOYSTICK_INTERFACE:
                    pD0Descr = &ConfigurationDescriptor.Joystick_HID;
                    len    = sizeof(USB_HID_Descriptor_HID_t);
                    break;
#endif
#if defined(DIGITIZER_ENABLE) && !defined(DIGITIZER_SHARED_EP)
                case DIGITIZER_INTERFACE:
                    pD0Descr = &ConfigurationDescriptor.Digitizer_HID;
                    len    = sizeof(USB_HID_Descriptor_HID_t);
                    break;
#endif
											default:
												// Unsupported string descriptor 
												len = 0xFFFF;  
												break;
										}
										break;

									case USB_DESCR_TYP_REPORT:  
										// Hid report descriptor 
										switch( pD0_SETUP_REQ->wIndexL )
										{       
											// Select interface 
											//case 0:
#ifndef KEYBOARD_SHARED_EP
											case KEYBOARD_INTERFACE:
												pD0Descr = (PUINT8C)( KeyboardReport );        
												len = KeyboardReport_size;
												break;										
#endif
											
#if defined(MOUSE_ENABLE) && !defined(MOUSE_SHARED_EP)
                                            case MOUSE_INTERFACE:
                                                pD0Descr = (PUINT8C) MouseReport;
                                                len    = MouseReport_size;
                                                break;
#endif

#ifdef SHARED_EP_ENABLE
											case SHARED_INTERFACE:
												pD0Descr = (PUINT8C)( SharedReport );
												len = SharedReport_size;
												break;
#endif

											//case 1:
#ifdef RAW_ENABLE
											case RAW_INTERFACE:
												pD0Descr = (PUINT8C)( RawReport );
												len = RawReport_size;
												break;
#endif

#ifdef CONSOLE_ENABLE
                                            case CONSOLE_INTERFACE:
                                                pD0Descr = (PUINT8C) ConsoleReport;
                                                len    = ConsoleReport_size;
                                                break;
#endif
#if defined(JOYSTICK_ENABLE) && !defined(JOYSTICK_SHARED_EP)
                                            case JOYSTICK_INTERFACE:
                                                pD0Descr = (PUINT8C) JoystickReport;
                                                len    = JoystickReport_size;
                                                break;
#endif
#if defined(DIGITIZER_ENABLE) && !defined(DIGITIZER_SHARED_EP)
                                            case DIGITIZER_INTERFACE:
                                                pD0Descr = (PUINT8C) DigitizerReport;
                                                len    = DigitizerReport_size;
                                                break;
#endif
											default:
												// Unsupported string descriptor 
												len = 0xFFFF;  
												break;
										}
										break;
										
									//case 0x06:
									//	pD0Descr = (PUINT8)( &My_QueDescr[ 0 ] );    								   
									//	len = sizeof( My_QueDescr );
									//	break;
									//		
									//case 0x07:	
									//	memcpy( &USB_FS_OSC_DESC[ 2 ], &MyDevice_CfgDesc[ 2 ], sizeof( MyDevice_CfgDesc ) - 2 );
									//	pD0Descr = (PUINT8)( &USB_FS_OSC_DESC[ 0 ] );    
									//	len = sizeof( USB_FS_OSC_DESC );
									//	break;
									//	
									default:
										// Unsupported string descriptor 
										len = 0xFFFF; 
										break;
								} 
								//USB_Request_Header_t *pSetupReq;
								//pSetupReq = (USB_Request_Header_t *)pD0_EP0_BUF;
								//#define pSetupReq ((USB_Request_Header_t __xdata *)XSFR_USBD0_BASE)
								//#define pSetupReq ((PXUSB_SETUP_REQ2)XSFR_USBD0_BASE)
								//PXUSB_SETUP_REQ2 pSetupReq = XSFR_USB_BASE;
								//USB_Request_Header_t *pSetupReq = XSFR_USB_BASE;
								//len = get_usb_descriptor(pSetupReq->wValue,pSetupReq->wIndex,pSetupReq->wLength,(void *)(&pD0Descr));
								//if (len==0) len = 0xffff;

								// Determine whether it can be processed normally						
								if( len != 0xFFFF )
								{
									UINT8 i;
									if( D0SetupLen > len ) 
									{	
										D0SetupLen = len;  							/* Limit total length */
									}
									len = D0SetupLen >= DEF_ENDP0_SIZE ? DEF_ENDP0_SIZE : D0SetupLen;  /* The length of this transmission */
									D0SetupLen -= len;
									for( i = 0; i < len; i++ ) {
										pD0_EP0_BUF[i] = pD0Descr[i];   /* __code read — completely safe & reentrant */
									}
									pD0Descr += len;
								}
								break;
								
							case USB_SET_ADDRESS:               
								/* USB device address setting */
								D0SetupLen = pD0_SETUP_REQ->wValueL;  				/* Temporarily store USB device address */
								break;
								
							case USB_GET_CONFIGURATION:
								pD0_EP0_BUF[ 0 ] = D0UsbConfig;
								if( D0SetupLen >= 1 ) 
								{
									len = 1;
								}
								break; 
														
							case USB_SET_CONFIGURATION:
								D0UsbConfig = pD0_SETUP_REQ->wValueL;
								/* configuration 0 returns the device to the Address state (USB 2.0 9.4.7): no reports until
								 * the host configures it again */
								USB_EnumStatus = D0UsbConfig ? 0x01 : 0x00;
								KB_USB_UpStatus = 0x01;
								/* the host restarts every endpoint at DATA0 */
#ifdef USE_D0_EP1_IN
								D0_EP1RES &= ~bUEP_X_TOG;
#endif
#ifdef USE_D0_EP2_IN
								D0_EP2RES &= ~bUEP_X_TOG;
#endif
#ifdef USE_D0_EP3_IN
								D0_EP3RES &= ~bUEP_X_TOG;
#endif
#ifdef USE_D0_EP3_OUT
								D0_EP3RES &= ~bUEP_X_TOG;
#endif
#ifdef USE_D0_EP5_IN
								D0_EP5RES &= ~bUEP_X_TOG;
#endif
								break;			
																
							case USB_SET_FEATURE:
								/* USB 2.0 9.4.9: ENDPOINT_HALT (0) for an endpoint, DEVICE_REMOTE_WAKEUP (1) for the device;
								 * other selectors are not supported (TEST_MODE is for high-speed devices, USB 2.0 7.1.20) */
								if( ( pD0_SETUP_REQ->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_ENDP && pD0_SETUP_REQ->wValueL == 0 ) 
								{  
									/* endpoint */
									switch( pD0_SETUP_REQ->wIndexL ) 
									{
#ifdef USE_D0_EP6_IN
										case 0x86:
										//case 0x06:
											D0_EP6RES = D0_EP6RES & ~( MASK_UEP_X_RES ) | UEP_X_RES_STALL;
											break;
#endif
											
#ifdef USE_D0_EP5_IN
										case 0x85:
										//case 0x05:
											D0_EP5RES = D0_EP5RES & ~( MASK_UEP_X_RES ) | UEP_X_RES_STALL;
											break;
#endif
											
										//case 0x84:
#ifdef USE_D0_EP4_OUT
										case 0x04:
											D0_EP4RES = D0_EP4RES & ~( MASK_UEP_X_RES ) | UEP_X_RES_STALL;
											break;
#endif
											
#ifdef USE_D0_EP3_IN
										case 0x83:
#endif
#ifdef USE_D0_EP3_OUT
										case 0x03:
#endif
#if defined(USE_D0_EP3_IN) || defined(USE_D0_EP3_OUT)
											D0_EP3RES = D0_EP3RES & ~( MASK_UEP_X_RES ) | UEP_X_RES_STALL;
											break;
#endif
											
#ifdef USE_D0_EP2_IN
										case 0x82:
#endif
#ifdef USE_D0_EP2_OUT
										case 0x02:
#endif
#if defined(USE_D0_EP2_IN) || defined(USE_D0_EP2_OUT)
											D0_EP2RES = D0_EP2RES & ~( MASK_UEP_X_RES ) | UEP_X_RES_STALL;
											break;
#endif
											
#ifdef USE_D0_EP1_IN
										case 0x81:
#endif
#ifdef USE_D0_EP1_OUT
										case 0x01:
#endif
#if defined(USE_D0_EP1_IN) || defined(USE_D0_EP1_OUT)
											D0_EP1RES = D0_EP1RES & ~( MASK_UEP_X_RES ) | UEP_X_RES_STALL;
											break;
#endif
											
										default:
											/* Unsupported endpoint */
											len = 0xFFFF;  
											break;
									}
								}
								else if( ( pD0_SETUP_REQ->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_DEVICE && pD0_SETUP_REQ->wValueL == 1 )
								{
									/* remote wakeup enabled */
									USB_SleepStatus |= 0x01;
								}
								else
								{
									len = 0xFFFF;
								}															
								break;
													
							case USB_SET_INTERFACE:
								break;
								
							case USB_CLEAR_FEATURE:
								/* the same selectors as SET_FEATURE */
								if( ( pD0_SETUP_REQ->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_ENDP && pD0_SETUP_REQ->wValueL == 0 ) 
								{  
									/* endpoint */
									switch( pD0_SETUP_REQ->wIndexL ) 
									{
#ifdef USE_D0_EP6_IN
										case 0x86:
											D0_EP6RES = D0_EP6RES & ~( bUEP_X_TOG | MASK_UEP_X_RES ) | UEP_X_RES_NAK;
											break;
#endif
											
#ifdef USE_D0_EP5_IN
										case 0x85:
											D0_EP5RES = D0_EP5RES & ~( bUEP_X_TOG | MASK_UEP_X_RES ) | UEP_X_RES_NAK;
											break;
#endif
											
#ifdef USE_D0_EP4_OUT
										case 0x04:
											D0_EP4RES = D0_EP4RES & ~( bUEP_X_TOG | MASK_UEP_X_RES ) | UEP_X_RES_ACK;
											break; 
#endif
																  
#ifdef USE_D0_EP3_IN
										case 0x83:
											D0_EP3RES = D0_EP3RES & ~( bUEP_X_TOG | MASK_UEP_X_RES ) | UEP_X_RES_NAK;
											break;
#endif
											
#ifdef USE_D0_EP3_OUT
										case 0x03:
											D0_EP3RES = D0_EP3RES & ~( bUEP_X_TOG | MASK_UEP_X_RES ) | UEP_X_RES_ACK;
											break; 
#endif
																  
#ifdef USE_D0_EP2_IN
										case 0x82:                       
											D0_EP2RES = D0_EP2RES & ~( bUEP_X_TOG | MASK_UEP_X_RES ) | UEP_X_RES_NAK;
											break;   
#endif
																
#ifdef USE_D0_EP2_OUT
										case 0x02:                       
											D0_EP2RES = D0_EP2RES & ~( bUEP_X_TOG | MASK_UEP_X_RES ) | UEP_X_RES_ACK;
											break; 
#endif
																  
#ifdef USE_D0_EP1_IN
										case 0x81:                       
											D0_EP1RES = D0_EP1RES & ~( bUEP_X_TOG | MASK_UEP_X_RES ) | UEP_X_RES_NAK;
											break;
#endif
																   
#ifdef USE_D0_EP1_OUT
										case 0x01:                       
											D0_EP1RES = D0_EP1RES & ~( bUEP_X_TOG | MASK_UEP_X_RES ) | UEP_X_RES_ACK;
											break;
#endif
											
										default:
											len = 0xFFFF;  
											break;
									}
								}
								else if( ( pD0_SETUP_REQ->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_DEVICE && pD0_SETUP_REQ->wValueL == 1 )
								{
									/* remote wakeup disabled */
									USB_SleepStatus &= ~0x01;
								}
								else
								{
									len = 0xFFFF;  
								}											
								break;
												
							case USB_GET_INTERFACE:
								pD0_EP0_BUF[ 0 ] = 0x00;
								if( D0SetupLen >= 1 ) 
								{
									len = 1;
								}
								break;
								
							case USB_GET_STATUS: 
								/* USB 2.0 9.4.5: an interface or endpoint that does not exist is a Request Error.
								 * pD0_EP0_BUF is also the SETUP packet: byte 0 (bRequestType) is written only after the recipient test. */
								if( ( pD0_SETUP_REQ->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_ENDP ) 
								{  
									/* endpoint */
									pD0_EP0_BUF[ 0 ] = 0x00;
									switch( pD0_SETUP_REQ->wIndexL ) 
									{
										case 0x00:
										case 0x80:
											break;

#ifdef USE_D0_EP6_IN
										case 0x86:                                       
											if( ( D0_EP6RES & MASK_UEP_X_RES ) == UEP_X_RES_STALL )
											{
												pD0_EP0_BUF[ 0 ] = 0x01;
											}                                      
											break;
#endif
											
#ifdef USE_D0_EP5_IN
										case 0x85:                                       
										//case 0x05:
											if( ( D0_EP5RES & MASK_UEP_X_RES ) == UEP_X_RES_STALL )
											{
												pD0_EP0_BUF[ 0 ] = 0x01;
											}                                      
											break;
#endif
											
#ifdef USE_D0_EP4_OUT
										case 0x04:
											if( ( D0_EP4RES & MASK_UEP_X_RES ) == UEP_X_RES_STALL )
											{
												pD0_EP0_BUF[ 0 ] = 0x01;
											}                                      
											break;
#endif

#ifdef USE_D0_EP3_IN
										case 0x83:                                       
#endif
#ifdef USE_D0_EP3_OUT
										case 0x03:
#endif
#if defined(USE_D0_EP3_IN) || defined(USE_D0_EP3_OUT)
											if( ( D0_EP3RES & MASK_UEP_X_RES ) == UEP_X_RES_STALL )
											{
												pD0_EP0_BUF[ 0 ] = 0x01;
											}                                      
											break;
#endif
											
#ifdef USE_D0_EP2_IN
										case 0x82:                                     
#endif
#ifdef USE_D0_EP2_OUT
										case 0x02:
#endif
#if defined(USE_D0_EP2_IN) || defined(USE_D0_EP2_OUT)
											if( ( D0_EP2RES & MASK_UEP_X_RES ) == UEP_X_RES_STALL )
											{
												pD0_EP0_BUF[ 0 ] = 0x01;																					
											}
											break;
#endif
											
#ifdef USE_D0_EP1_IN
										case 0x81:                                       
#endif
#ifdef USE_D0_EP1_OUT
										case 0x01:
#endif
#if defined(USE_D0_EP1_IN) || defined(USE_D0_EP1_OUT)
											if( ( D0_EP1RES &MASK_UEP_X_RES ) == UEP_X_RES_STALL )
											{
												pD0_EP0_BUF[0] = 0x01;
											}
											break;                                
#endif

										default:
											/* Unsupported endpoint */
											len = 0xFFFF;
											break;
										}
								}
								else if( ( pD0_SETUP_REQ->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_DEVICE )
								{
									/* bit 1: remote wakeup enabled (bit 0 of USB_SleepStatus; its bit 1 is the bus suspend) */
									pD0_EP0_BUF[ 0 ] = 0x00;
									if( USB_SleepStatus & 0x01 )
									{
										pD0_EP0_BUF[ 0 ] = 0x02;
									}
								}
								else if( ( pD0_SETUP_REQ->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_INTERF )
								{
									/* interface status: 0 */
									pD0_EP0_BUF[ 0 ] = 0x00;
									if( pD0_SETUP_REQ->wIndexL >= TOTAL_INTERFACES )
									{
										len = 0xFFFF;
									}
								}
								else
								{
									len = 0xFFFF;
								}
								if( len != 0xFFFF )
								{
									pD0_EP0_BUF[ 1 ] = 0;
									if( D0SetupLen >= 2 ) 
									{
										len = 2;
									}
									else 
									{
										len = D0SetupLen;
									}
								}
								break;
								
							default:
								/* operation failed */
								len = 0xFFFF;  
								break;
						}
					}
					else if( ( pD0_SETUP_REQ->bRequestType & USB_REQ_TYP_MASK ) == USB_REQ_TYP_CLASS )
					{
						/* class request */
						switch( D0SetupReqCode )
						{
							case DEF_USB_GET_REPORT:								/* 0x01: GET_REPORT */
								/* Windows sends GET_REPORT during HID initialization.
								 * Return an empty keyboard report (8 bytes of zeros).
								 * Without this, the device STALLs and Windows gives Code 43.
								 */
								{
									UINT8 k;
									for(k = 0; k < 8; k++) {
										pD0_EP0_BUF[k] = 0;
									}
									len = 8;
									if( D0SetupLen < len ) {
										len = D0SetupLen;
									}
								}
								break;

							case DEF_USB_SET_REPORT:			 				/* 0x09: SET_REPORT */
								break;
							
							case DEF_USB_SET_IDLE:								/* 0x0A: SET_IDLE */
								Idle_Value = pD0_EP0_BUF[ 3 ];
								break;
								
							case DEF_USB_SET_PROTOCOL:							/* 0x0B: SET_PROTOCOL */
								/* only the keyboard interface has a boot protocol; protocol_post_task() applies it */
								if( pD0_SETUP_REQ->wIndexL == KEYBOARD_INTERFACE )
								{
									Report_Value = pD0_EP0_BUF[ 2 ];
								}
								break;
								
							case DEF_USB_GET_IDLE:								/* 0x02: GET_IDLE */
								pD0_EP0_BUF[ 0 ] = Idle_Value;
								len = 1;
								break;

							case DEF_USB_GET_PROTOCOL:							/* 0x03: GET_PROTOCOL */
								pD0_EP0_BUF[ 0 ] = Report_Value;
								len = 1;
								break;
								
							default:
								len = 0xFFFF;  							
								break;
						}
					}
					else
					{
						/* Vendor or reserved request type: not supported, STALL (USB 2.0 9.2.7). Accepting one
						 * with an OUT data stage would leave its status stage at NAK (EP0 OUT handler) until
						 * the host times out. */
						len = 0xFFFF;
					}
					
					if( len == 0xFFFF ) 
					{  
						/* Operation failed — STALL endpoint 0.
						 * CH555 hardware may not auto-clear STALL when the next
						 * SETUP packet arrives (unlike standard USB controllers).
						 * We must manually clear it after a brief delay to ensure
						 * the host sees the STALL response but EP0 is ready for
						 * the next SETUP. Without this, Windows gives Code 43.
						 */
						D0SetupReqCode = 0xFF;
						D0_EP0RES = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;
					}
					else if( len <= DEF_ENDP0_SIZE ) 
					{  
						/* Upload data or status phase returns 0 length packet */               
						D0_EP0T_L = len;
						D0_EP0RES = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;  /* The default packet is data1 */  
					}
					else 
					{  
						/* Download data or other */
						/* Although it has not yet reached the status stage, it is preset to upload 0-length data packets in advance to prevent the host from entering the status stage early. */
						D0_EP0T_L = 0;  
						D0_EP0RES = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;  /* The default packet is data1 */
					}
				break;
		}     
        D0_STATUS = 0;    
	}
	else if( us & bUX_IF_SUSPEND )
    {
    	/* Suspend or wake up events */	
		if( us & bUX_SUSPEND )
		{       
			/* Bus suspend detected (no SOF for >3ms) */
			USB_SleepStatus |= 0x02;
			if( USB_SleepStatus == 0x03 )
			{
				/* Host explicitly requested suspend AND bus is suspended.
				 * DISABLED: CH55X_Sleep_Deal() drops the D+ pull-up from
				 * 1.5k to 7.5k, which Windows interprets as a physical
				 * disconnect, causing a connect/disconnect loop + Code 43.
				 * Safe to disable for a wired keyboard (no battery).
				 */
				// MCU_Sleep_Operate = 0x01;
			}
			/* NOTE: We no longer clear USB_EnumStatus here.
			 * The old code cleared it on every suspend that wasn't 0x03,
			 * which included the brief suspend during initial enumeration.
			 * This caused the device to think it wasn't enumerated even
			 * after SET_CONFIGURATION, leading to failed enumeration on
			 * Windows and intermittent failures on Linux.
			 */
		}
		else
		{                      
			/* Wake event (SOF resumed) */
			USB_SleepStatus &= ~0x02;
			/* Only restore USB_EnumStatus if we were previously configured.
			 * Don't unconditionally set it to 1 — that would allow sending
			 * reports before the host has finished enumeration.
			 */
			if( D0UsbConfig != 0 )
			{
				USB_EnumStatus = 0x01;
			}
        }
		/* No D0_STATUS write (WCH's example has one): it would clear a D0 completion that arrived during this
		 * pass, and that completion would not be handled. The check at the end starts another pass for it. */
		USB_IF = bUX_IF_SUSPEND;
    }
	else if(us & bUX_IF_BUS_RST) 
    {
    	/* Bus reset event — host is resetting the device.
		 * Must fully reinitialize all USB state per USB spec. */
        USB_EP_init();                           /* Re-init endpoints with clean toggle bits (DATA0) */
        D0_ADDR = 0;
        HB_ADDR = 0x7F;
        USB_IF = 0xFF;								 /* Clear all pending interrupt flags */

        USB_IE = bUX_IE_SUSPEND | bUX_IE_TRANSFER | bUX_IE_BUS_RST;

		/* Fully reset all state variables */
        KB_USB_UpStatus = 0x00;
		USB_EnumStatus = 0x00;
		D0UsbConfig = 0x00;								 /* Not yet configured */
		D0SetupReqCode = 0xFF;							 /* No pending request */
		D0SetupLen = 0x00;
		Report_Value = 0x01;							 /* report protocol */
		USB_SleepStatus = 0x00;							 /* Clear sleep state — fresh start */
#ifdef USE_D0_EP1_OUT
		ep1_data_wait = 0x00;
#endif
#ifdef USE_D0_EP2_OUT
		ep2_data_wait = 0x00;
#endif
#ifdef USE_D0_EP3_OUT
		ep3_data_wait = 0x00;
#endif
#ifdef USE_D0_EP4_OUT
		ep4_data_wait = 0x00;
#endif

        USB_IF = bUX_IF_BUS_RST;
	}
  	
  	/* Determine again whether there are still USB interrupts that need to be processed */
	if( USB_IF & ( bUX_IF_D0_TRANS | bUX_IF_SUSPEND | bUX_IF_BUS_RST ) )
	{
		goto USB_DevIntNext;
	}    
    USB_IE = save;
}
#pragma restore

/*******************************************************************************
* Function Name  : USB_WakeUp_PC
* Description    : Usb device wakes up computer
* Input          : None
* Output         : None
* Return         : None									   
*******************************************************************************/
void USB_WakeUp_PC( void )
{
	UINT8  save;

	/* USB 2.0 7.1.7.7: resume signalling only after 5 ms of idle bus. bUX_SUSPEND is set after 4-6 ms of idle
	 * bus (datasheet, Tusp); with the bit still set 2 ms later, the bus has been idle for at least 6 ms. If the
	 * host has resumed the bus meanwhile, no signal is needed. */
	mDelaymS( 2 );
	if( !( USB_IF & bUX_SUSPEND ) )
	{
		return;
	}

	/* The resume signal also clears bUX_SUSPEND, and if the host stays asleep the bit is not set
	 * again. USB_WakeUpSent marks the bus as still suspended until the next host transfer. */
	USB_WakeUpSent = 0x01;
	save = USB_CTRL;
    USB_CTRL ^= bUX_DP_PU_EN | bUX_DM_PU_EN;
    mDelaymS( 10 );  
    USB_CTRL = save;
#if USB_SUSPEND_WAKEUP_DELAY > 0
	/* QMK option: pause so that hubs, KVM switches and monitors can settle after the wakeup */
	mDelaymS( USB_SUSPEND_WAKEUP_DELAY );
#endif
}

/*******************************************************************************
* Function Name  : CH55X_Sleep_Deal
* Description    : Ch55x microcontroller sleep processing
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/ 
void CH55X_Sleep_Deal( void )	
{
	UINT8	savePortCfg;

	P4_LED_KEY = 0xFF;															/* enable key mode */
    P4_MOD_OC = 0xFF;															/* disable push-pull output */
    //LED_CTRL &= ~bLED_EN;
																				/* P4_DIR_PU = 0xFF; */																						
	P0 = P3 = 0;																/* COMMON all low to support any key wakeup */
	P7 &= ~0x03;																/* P7 &= ~ 0x03, if P7 used for COMMON  */ 
	savePortCfg = PORT_CFG;
	PORT_CFG |= bP4_IE_LEVEL;		/* enable P4 level change to wakeup, P4 input@pullup vs P4 output@0xFF, so low action */
																				/* USB_CTRL = 0; */	
	/* Before sleeping, switch the USB port pull-up resistor to 7 k5 */
	P6_OUT_PU = ( 1 << 1 );
	USB_CTRL &= ~bUX_DP_PU_EN;
	P6_DIR = 0;
	
	PCON |= PD;																	/* sleep */
		
	mDelaymS( 1 ); 
	USB_CTRL |= bUX_DP_PU_EN;
	P6_OUT_PU = 0;

	SAFE_MOD = 0x55;
	SAFE_MOD = 0xAA;
	WAKE_CTRL = 0x00;
	SAFE_MOD = 0x00;
    
	PORT_CFG = savePortCfg;
	if ( GPIO_IE & bIE_IO_EDGE ) 
	{	
		/* edge mode interrupt */
		GPIO_IE &= ~ bIE_IO_EDGE;												/* clear P4 level change interrupt flag */
		GPIO_IE |= bIE_IO_EDGE;	
	}
	mDelaymS( 5 );

	P0 = P3 = 0xFF;  																							
	//LED_CTRL = bLED_IE_INHIB | bLED_BLUE_EN | bLED_GREEN_EN | bLED_RED_EN | bLED_COM_AHEAD | bLED_PWM_INHIB | bLED_EN;
	//LED_STATUS |= bLED_IF_SET;  												/* force bLED_IF to enter interrupt service */
	//IE_PWM_I2C = 1;   
}

