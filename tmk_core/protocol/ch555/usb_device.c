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
#include "usb_device.h"
#include "suspend.h"
#include "report.h"
#include "mcs51_isr.h"

/******************************************************************************/
/* constant variable definition */
volatile UINT8	D0SetupReqCode = 0xFF;											/* USB Setup package request code */
volatile UINT16	D0SetupLen = 0x00;												/* USB Setup packet length */
volatile PUINT8C	pD0Descr;   /* points into __code Flash — MUST be PUINT8C on SDCC/8051 */

volatile UINT8  D0UsbConfig = 0x00;												/* USB configuration flags - the Configuation Id selected */
volatile UINT8  Report_Value = 0x01;                                            /* HID protocol of interface 0 (SET_PROTOCOL): 0 boot, 1 report (the default, HID 1.11 7.2.6) */
/* HID 1.11 7.2.4: recommended idle rate of a keyboard at initialization, 500 ms (4 ms units) */
#define KEYBOARD_IDLE_DEFAULT 125
volatile UINT8  Idle_Value = KEYBOARD_IDLE_DEFAULT;                             /* SET_IDLE duration of interface 0 (4 ms units, 0: report only on a change) */
volatile UINT8  KB_USB_SetReport = 0x00;										/* LED output report (SET_REPORT) */

volatile UINT8  USB_EnumStatus = 0x00;											/* USB enumeration status */	
volatile UINT8  USB_SleepStatus = 0x00;											/* USB sleep state */		
volatile UINT8  USB_WakeUpSent = 0x00;											/* remote wakeup signalled, no host transfer since */

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

#if !defined(SERIAL_NUMBER) && defined(SERIAL_NUMBER_USE_HARDWARE_ID)
#include "hardware_id.h"

/* String descriptor 3 (serial number), in xdata: the 48-bit chip ID as 12 hex digits, bytes in get_hardware_id()
 * order, high nibble first. 26 bytes: always sent in one EP0 packet. */
static struct {
	USB_Descriptor_Header_t	Header;
	UINT16					UnicodeString[12];
} D0SerialString;

static void D0SerialStringInit( void )
{
	static const char __code hex[] = "0123456789ABCDEF";
	hardware_id_t id = get_hardware_id();
	UINT8X *p = (UINT8X *)&id;
	UINT8 i;

	D0SerialString.Header.Size = sizeof( D0SerialString );
	D0SerialString.Header.Type = DTYPE_String;
	for( i = 0; i < 6; i++ ) {
		D0SerialString.UnicodeString[2 * i]     = hex[p[i] >> 4];
		D0SerialString.UnicodeString[2 * i + 1] = hex[p[i] & 0x0F];
	}
}
#endif

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
	USB_WakeUpSent = 0x00;														/* remote wakeup signalled, no host transfer since */
#ifdef USE_D0_EP1_OUT
	ep1_data_wait   = 0x00;
#endif
#ifdef USE_D0_EP2_OUT
	ep2_data_wait   = 0x00;
#endif
#ifdef USE_D0_EP3_OUT
	ep3_data_wait   = 0x00;
#endif
#ifdef USE_D0_EP4_OUT
	ep4_data_wait   = 0x00;
#endif
	KB_USB_SetReport = 0x00;													/* Usb keyboard set report value */
#if !defined(SERIAL_NUMBER) && defined(SERIAL_NUMBER_USE_HARDWARE_ID)
	D0SerialStringInit();
#endif

	/* Initialize usb related registers */
	IE_USB = 0;                              // usb interrupt dissable, avoid acidental interrupt during init
	/* USB interrupt at high priority (datasheet 9: a bit set in IP/IP_EX = high), so that it does not wait for
	 * the LED and Timer0 handlers: after a STALL, EP0 answers STALL until the next SETUP has been decoded. Under
	 * the OEM bootloader this needs handlers that end with RET (mcs51_isr.h): with a RETI in the handler, the
	 * stub's own RETI would end the low-priority interrupt that this one preempted. */
	IP_EX |= bIP_USB;

	SAFE_MOD = 0x55;
	SAFE_MOD = 0xAA;
	GLOBAL_CFG &= ~bWDOG_EN;                 // disable watch-dog reset
	SAFE_MOD = 0x00;

	USB_CTRL = bUX_RESET_SIE | bUX_CLR_ALL; // pulse reset — flush SIE and FIFOs
	USB_CTRL = 0;                            // clear reset

    USB_EP_init();                           // setup endpoints

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
MCS51_ISR( USB_DeviceInterrupt, INT_NO_USB )
{
    UINT8  us,i;
	UINT16 len;

USB_DevIntNext:
	/* A SETUP ends any control transfer and a STALL of EP0 (USB 2.0 8.5.3.4), but EP0 keeps its previous response
	 * (STALL, or ACK with the previous length and data) until it is written. Answer NAK from here until the request
	 * is decoded and its reply loaded. First thing in the pass: the host's IN can follow its SETUP within about
	 * 2 us. */
	if( D0_STATUS & bUXS_SETUP_ACT )
	{
		D0_EP0RES = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_NAK | UEP_T_RES_NAK;
	}
	us = USB_IF; 
	if( us & ( bUX_IF_D0_TRANS | bUX_IF_BUS_RST ) )
	{
		/* The host is running the bus again (see USB_WakeUp_PC) */
		USB_WakeUpSent = 0x00;
	}
	if( us & bUX_IF_D0_TRANS )
	{
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
					/* next chunk of the descriptor; pD0Descr points into code memory */
					UINT8 i;
					len = D0SetupLen >= DEF_ENDP0_SIZE ? DEF_ENDP0_SIZE : D0SetupLen;
					for( i = 0; i < len; i++ ) {
						pD0_EP0_BUF[i] = pD0Descr[i];
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
					/* endpoint 0# SETUP (EP0 answers NAK since the start of this pass) */
					len = 0;  														/* Defaults to success and uploading 0 length */ 

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
											#elif defined(SERIAL_NUMBER_USE_HARDWARE_ID)
											case 3:
												pD0Descr = (PUINT8C)0;	/* D0SerialString (xdata), copied below */
												len = sizeof( D0SerialString );
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
												// no HID descriptor for this interface
												len = 0xFFFF;  
												break;
										}
										break;

									case USB_DESCR_TYP_REPORT:  
										// Hid report descriptor 
										switch( pD0_SETUP_REQ->wIndexL )
										{       
											// Select interface 
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
												// no report descriptor for this interface
												len = 0xFFFF;  
												break;
										}
										break;
										
									default:
										// unsupported descriptor type
										len = 0xFFFF; 
										break;
								} 
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
#if !defined(SERIAL_NUMBER) && defined(SERIAL_NUMBER_USE_HARDWARE_ID)
									if( !pD0Descr )
									{
										for( i = 0; i < len; i++ ) {
											pD0_EP0_BUF[i] = ((UINT8X *)&D0SerialString)[i];
										}
									}
									else
#endif
									{
										for( i = 0; i < len; i++ ) {
											pD0_EP0_BUF[i] = pD0Descr[i];
										}
										pD0Descr += len;
									}
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
								/* the host restarts every endpoint at DATA0; an IN endpoint goes to NAK, so that a report
								 * still waiting in it from the previous configuration is not read in the new one */
#ifdef USE_D0_EP1_IN
								D0_EP1RES = UEP_X_RES_NAK;
#endif
#ifdef USE_D0_EP2_IN
								D0_EP2RES = UEP_X_RES_NAK;
#endif
#ifdef USE_D0_EP3_IN
								D0_EP3RES = UEP_X_RES_NAK;
#endif
#ifdef USE_D0_EP3_OUT
								D0_EP3RES &= ~bUEP_X_TOG;
#endif
#ifdef USE_D0_EP5_IN
								D0_EP5RES = UEP_X_RES_NAK;
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
											D0_EP6RES = D0_EP6RES & ~( MASK_UEP_X_RES ) | UEP_X_RES_STALL;
											break;
#endif
											
#ifdef USE_D0_EP5_IN
										case 0x85:
											D0_EP5RES = D0_EP5RES & ~( MASK_UEP_X_RES ) | UEP_X_RES_STALL;
											break;
#endif
											
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
								/* HID 1.11 7.2.1: the input report (type 1) of the keyboard interface is its current report,
								 * kbd_report. The main loop writes kbd_report with this interrupt enabled (while the interrupt
								 * waits, EP0 still answers a previous STALL), so an answer during a key change can mix two
								 * consecutive reports; the new report follows on EP1. Other reports: 8 zero bytes. */
								{
									UINT8 k;
									if( pD0_SETUP_REQ->wIndexL == KEYBOARD_INTERFACE && pD0_SETUP_REQ->wValueH == 1 )
									{
										for(k = 0; k < 8; k++) {
											pD0_EP0_BUF[k] = kbd_report[k];
										}
									}
									else
									{
										for(k = 0; k < 8; k++) {
											pD0_EP0_BUF[k] = 0;
										}
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
								/* duration in the high byte of wValue; only the keyboard interface repeats its report
								 * (protocol_post_task()), the other interfaces report only on a change */
								if( pD0_SETUP_REQ->wIndexL == KEYBOARD_INTERFACE )
								{
									Idle_Value = pD0_SETUP_REQ->wValueH;
								}
								break;
								
							case DEF_USB_SET_PROTOCOL:							/* 0x0B: SET_PROTOCOL */
								/* HID 1.11 7.2.6: only a boot interface (the keyboard interface) has a protocol; STALL for
								 * the others. protocol_post_task() applies it. */
								if( pD0_SETUP_REQ->wIndexL == KEYBOARD_INTERFACE )
								{
									Report_Value = pD0_EP0_BUF[ 2 ];
								}
								else
								{
									len = 0xFFFF;
								}
								break;
								
							case DEF_USB_GET_IDLE:								/* 0x02: GET_IDLE */
								pD0_EP0_BUF[ 0 ] = ( pD0_SETUP_REQ->wIndexL == KEYBOARD_INTERFACE ) ? Idle_Value : 0;
								len = 1;
								break;

							case DEF_USB_GET_PROTOCOL:							/* 0x03: GET_PROTOCOL */
								/* HID 1.11 7.2.5: boot interfaces only, as SET_PROTOCOL */
								if( pD0_SETUP_REQ->wIndexL == KEYBOARD_INTERFACE )
								{
									pD0_EP0_BUF[ 0 ] = Report_Value;
									len = 1;
								}
								else
								{
									len = 0xFFFF;
								}
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
						/* Request not supported: STALL the data and status stages. The next SETUP ends the STALL
						 * (USB 2.0 8.5.3.4); the pass that handles it sets EP0 to NAK first (USB_DevIntNext). */
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
			/* Bus suspend: no bus activity for 4-6 ms (datasheet, Tusp). USB_EnumStatus is kept: the
			 * configuration lasts through a suspend (USB 2.0 9.1.1.6). */
			/* The main loop puts the MCU to sleep (USB_Sleep, if the keyboard defines CH555_SUSPEND_SLEEP) */
			USB_SleepStatus |= 0x02;
		}
		else
		{
			/* Bus activity resumed */
			USB_SleepStatus &= ~0x02;
			/* USB_EnumStatus already equals ( D0UsbConfig != 0 ): SET_CONFIGURATION and the bus reset set both */
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
		USB_EnumStatus = 0x00;
		D0UsbConfig = 0x00;								 /* Not yet configured */
		D0SetupReqCode = 0xFF;							 /* No pending request */
		D0SetupLen = 0x00;
		Report_Value = 0x01;							 /* report protocol */
		Idle_Value = KEYBOARD_IDLE_DEFAULT;
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

#ifdef CH555_SUSPEND_SLEEP
/*******************************************************************************
* Function Name  : USB_Sleep
* Description    : MCU sleep while the bus is suspended (USB 2.0 7.1.7.6, 7.2.3)
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USB_Sleep( void )
{
	EA = 0;																		/* datasheet 7.2: interrupts off before PD */
	if( ( USB_IF & bUX_SUSPEND ) || USB_WakeUpSent )							/* the bus may have resumed since the caller's test */
	{
		/* Bus activity wakes the MCU; the keys only if the host enabled remote wakeup */
		ch555_sleep_enter_kb( USB_SleepStatus & 0x01 );
		SAFE_MOD = 0x55;
		SAFE_MOD = 0xAA;
		WAKE_CTRL = bWAK_BY_USB;
		SAFE_MOD = 0x00;
		/* Deep sleep (POWER_CFG keeps bPWR_DN_MODE 0) turns the 3.3 V regulator off, and the 1.5K D+ pull-up goes to
		 * that 3.3 V. With VDD above 4 V, P6.1's 7.5K pull-up to VDD replaces it during the sleep: 7.5K on before
		 * 1.5K off, and after the wake 1.5K on before 7.5K off (datasheet 10.2.5). */
		P6_OUT_PU |= 0x02;
		USB_CTRL &= ~bUX_DP_PU_EN;
		PCON |= PD;
		/* Woken by bus activity or a key. Stock 1.04 waits 1 ms before the 1.5K pull-up goes back on. */
		mDelaymS( 1 );
		USB_CTRL |= bUX_DP_PU_EN;
		P6_OUT_PU &= ~0x02;
		SAFE_MOD = 0x55;
		SAFE_MOD = 0xAA;
		WAKE_CTRL = 0;
		SAFE_MOD = 0x00;
		ch555_sleep_exit_kb();
	}
	EA = 1;
}
#endif

