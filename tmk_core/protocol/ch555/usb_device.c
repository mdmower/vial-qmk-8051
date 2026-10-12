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

/******************************************************************************/
/* constant variable definition */
volatile UINT8	D0SetupReqCode = 0xFF;											/* USB Setup package request code */
volatile UINT16	D0SetupLen = 0x00;												/* USB Setup packet length */
volatile PUINT8C	pD0Descr;   /* points into __code Flash — MUST be PUINT8C on SDCC/8051 */

// Hub specific EP0 variables
volatile UINT8	HBSetupReqCode = 0xFF;
volatile UINT16	HBSetupLen = 0x00;
volatile UINT8  HBNewAddress = 0x00;
volatile PUINT8C pHBDescr;

// D1 specific EP0 variables
volatile UINT8	D1SetupReqCode = 0xFF;
volatile UINT16	D1SetupLen = 0x00;
volatile PUINT8C pD1Descr;
volatile UINT8  D1UsbConfig = 0x00;

// D2 specific EP0 variables
volatile UINT8	D2SetupReqCode = 0xFF;
volatile UINT16	D2SetupLen = 0x00;
volatile PUINT8C pD2Descr;
volatile UINT8  D2UsbConfig = 0x00;

#define pHB_SETUP_REQ ((PXUSB_SETUP_REQ)pHB_EP0_BUF)

/*
 * Hub port state machine.
 * hub_port_status[i]: wPortStatus bits — bit0=CONNECTION, bit1=ENABLE, bit8=POWER
 * hub_port_change[i]: wPortChange bits — bit0=C_CONNECTION, bit4=C_RESET
 * hub_ep1_pending:    bitmask of ports with pending status change (bit1=port1, etc.)
 *
 * Initial state: port 1 = POWER+CONNECTED (0x0101) with C_CONNECTION pending
 * (D0/keyboard is always physically present). Ports 2/3 = POWER only (0x0100).
 * Host sees C_CONNECTION on EP1 IN → issues PORT_RESET → we set ENABLED and
 * flag C_RESET, then notify via EP1 IN. On power-cycle (CLEAR/SET PORT_POWER),
 * C_CONNECTION is re-armed so the host re-enumerates the downstream device.
 */
#define HUB_PORT_POWER      0x0100u
#define HUB_PORT_CONNECTED  0x0001u
#define HUB_PORT_ENABLED    0x0002u
#define HUB_C_RESET         0x0010u
#define HUB_C_CONNECTION    0x0001u

volatile UINT16 hub_port_status[3];   /* indexed 0..2 = port 1..3 */
volatile UINT16 hub_port_change[3];
volatile UINT8  hub_ep1_pending = 0;  /* bit1=port1, bit2=port2, bit3=port3 */

volatile UINT8  D0UsbConfig = 0x00;												/* USB configuration flags - the Configuation Id selected */
volatile UINT8  Report_Value = 0x00;                                            /* hid interface related */
volatile UINT8  Idle_Value = 0x00;                                              /* host request hid interface go idle */
volatile UINT8  KB_USB_SetReport = 0x00;										/* */

volatile UINT8  USB_EnumStatus = 0x00;											/* USB enumeration status */	
volatile UINT8  USB_SleepStatus = 0x00;											/* USB sleep state */		
volatile UINT8  MCU_Sleep_Operate = 0x00;										/* Mcu sleep operation flag */
volatile UINT8  USB_WakeUpSent = 0x00;											/* remote wakeup signalled, no host transfer since */

/*
 * Raw byte-array USB string descriptors for CH555.
 * SDCC 8051 has 32-bit wchar_t AND mangles uint16_t flexible array members,
 * so we bypass all struct/type machinery and hand-place every byte as
 * correct USB UTF-16LE. Format: bLength, bDescriptorType(0x03), then
 * pairs of (char, 0x00) for each ASCII character.
 */
static const UINT8 __code RawManufacturerString[] = {
    18, 0x03,                   /* bLength=18, bDescriptorType=STRING */
    'R', 0x00,                  /* UTF-16LE 'R' */
    'e', 0x00,                  /* UTF-16LE 'e' */
    'd', 0x00,                  /* UTF-16LE 'd' */
    'r', 0x00,                  /* UTF-16LE 'r' */
    'a', 0x00,                  /* UTF-16LE 'a' */
    'g', 0x00,                  /* UTF-16LE 'g' */
    'o', 0x00,                  /* UTF-16LE 'o' */
    'n', 0x00                   /* UTF-16LE 'n' */
};

static const UINT8 __code RawProductString[] = {
    20, 0x03,                   /* bLength=20, bDescriptorType=STRING */
    'K', 0x00,                  /* UTF-16LE 'K' */
    '5', 0x00,                  /* UTF-16LE '5' */
    '8', 0x00,                  /* UTF-16LE '8' */
    '0', 0x00,                  /* UTF-16LE '0' */
    ' ', 0x00,                  /* UTF-16LE ' ' */
    'V', 0x00,                  /* UTF-16LE 'V' */
    'a', 0x00,                  /* UTF-16LE 'a' */
    't', 0x00,                  /* UTF-16LE 't' */
    'a', 0x00                   /* UTF-16LE 'a' */
};
   

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
#ifdef USE_D2_EP2_OUT
volatile UINT8  ep_d2ep2_data_wait = 0x00;										/* D2 endpoint 2 OUT data waiting flag */
#endif

//UINT8D  RGB_Mode = 0x00;
void USB_EP_init( void )  
{
    /* EP0: clear toggle bits to DATA0, ACK for OUT/SETUP, NAK for IN */
    HB_EP0RES = bUEP_X_AUTO_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
    /* IN endpoints: no bUEP_X_AUTO_TOG. With it, a completed transaction on one sub-device also flips the
     * IN toggle of the other sub-devices' auto-toggle endpoints (seen on the VS11K33A), and the next report
     * of such an endpoint repeats the last toggle and is discarded by the host. The interrupt toggles them
     * on each IN completion instead. OUT endpoints keep the auto-toggle (no flips seen). */
    HB_EP1RES = UEP_X_RES_NAK;

    D0_EP0RES = UEP_R_RES_ACK | UEP_T_RES_NAK;  /* toggle bits cleared (DATA0) */
    D0_EP_MOD = 0;              /* D0 disabled until PORT_RESET on port 1 */
    D1_EP0RES = UEP_R_RES_ACK | UEP_T_RES_NAK;
    D1_EP_MOD = 0;              /* D1 disabled until PORT_RESET on port 2 */
    D2_EP0RES = UEP_R_RES_ACK | UEP_T_RES_NAK;
    D2_EP_MOD = 0;              /* D2 disabled until PORT_RESET on port 3 */
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
    D0_EP1RES = bUEP_X_AUTO_TOG | UEP_X_RES_ACK;  /* toggle bit cleared (DATA0), ACK ready to receive */
#endif
#ifdef USE_D0_EP2_OUT
    D0_EP_MOD |= bUX_EP2O_EN;
    D0_EP2RES = bUEP_X_AUTO_TOG | UEP_X_RES_ACK;
#endif
#ifdef USE_D0_EP3_OUT
    D0_EP_MOD |= bUX_EP3O_EN;
    D0_EP3RES = bUEP_X_AUTO_TOG | UEP_X_RES_ACK; 
#endif
#ifdef USE_D0_EP4_OUT
    D0_EP_MOD |= bUX_EP4O_EN;
    D0_EP4RES = bUEP_X_AUTO_TOG | UEP_X_RES_ACK; 
#endif

#ifdef USE_D1_EP1_IN
    D1_EP1RES = UEP_X_RES_NAK;
#endif
#ifdef USE_D2_EP1_IN
    D2_EP1RES = UEP_X_RES_NAK;
#endif
#ifdef USE_D2_EP2_OUT
    D2_EP_MOD |= bUX_EP2O_EN;
    D2_EP2RES = bUEP_X_AUTO_TOG | UEP_X_RES_ACK;
#endif

    /* Clear all endpoint transmit lengths */
    HB_EP0T_L = 0;
    HB_EP1T_L = 0;
    D0_EP0T_L = 0;
    D1_EP0T_L = 0;
    D2_EP0T_L = 0;
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
#ifdef USE_D1_EP1_IN
    D1_EP1T_L = 0;
#endif
#ifdef USE_D2_EP1_IN
    D2_EP1T_L = 0;
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
	HBSetupReqCode = 0xFF;
	HBSetupLen = 0x00;
	/* Port 1 (D0/keyboard) is always physically present — report as connected so
	 * the host will issue PORT_RESET and enumerate D0.  Ports 2/3 start powered-only. */
	hub_port_status[0] = HUB_PORT_POWER | HUB_PORT_CONNECTED;
	hub_port_change[0] = HUB_C_CONNECTION;
	hub_ep1_pending    = 0x02;
#ifdef SHARED_EP_ENABLE
	hub_port_status[1] = HUB_PORT_POWER | HUB_PORT_CONNECTED;
	hub_port_change[1] = HUB_C_CONNECTION;
	hub_ep1_pending   |= 0x04;  /* bit 2 = port 2 has a status change pending */
#else
	hub_port_status[1] = HUB_PORT_POWER;
	hub_port_change[1] = 0;
#endif
#ifdef RAW_ENABLE
	hub_port_status[2] = HUB_PORT_POWER | HUB_PORT_CONNECTED;
	hub_port_change[2] = HUB_C_CONNECTION;
	hub_ep1_pending   |= 0x08;  /* bit 3 = port 3 has a status change pending */
#else
	hub_port_status[2] = HUB_PORT_POWER;
	hub_port_change[2] = 0;
#endif
	D1SetupReqCode = 0xFF;
	D1SetupLen = 0x00;
	D1UsbConfig = 0x00;
	D2SetupReqCode = 0xFF;
	D2SetupLen = 0x00;
	D2UsbConfig = 0x00;
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
#ifdef USE_D2_EP2_OUT
	ep_d2ep2_data_wait = 0x00;
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
	D1_ADDR = 0;
	D2_ADDR = 0;
	HB_ADDR = 0x00;             /* Hub enumerates as real hub at default address 0 */
	D0_ADDR = 0;
	D1_ADDR = 0;
	D2_ADDR = 0;
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
	if( us & ( bUX_IF_D2_TRANS | bUX_IF_D1_TRANS | bUX_IF_D0_TRANS | bUX_IF_HB_TRANS | bUX_IF_BUS_RST ) )
	{
		/* The host is running the bus again (see USB_WakeUp_PC) */
		USB_WakeUpSent = 0x00;
	}
	if ( us & bUX_IF_HB_TRANS )
	{
		// -------------------------------------------------------------------
		// USBHB (Hub) handling skeleton
		// -------------------------------------------------------------------
		if( HB_STATUS & bUXS_SETUP_ACT )
		{
			goto handle_hb_ep0_setup;
		}

		switch ( HB_STATUS & ( MASK_UXS_TOKEN | MASK_UXS_ENDP ) )
		{
			case UXS_TOKEN_IN | 0:
				switch( HBSetupReqCode )
				{
					case USB_GET_DESCRIPTOR:
					{
						UINT8 i;
						len = HBSetupLen >= DEF_ENDP0_SIZE ? DEF_ENDP0_SIZE : HBSetupLen;
						for( i = 0; i < len; i++ ) {
							pHB_EP0_BUF[i] = pHBDescr[i];
						}
						HBSetupLen -= len;
						pHBDescr   += len;
						HB_EP0T_L   = len;
						break;
					}
					case USB_SET_ADDRESS:
						HB_ADDR = HBNewAddress;
						HB_EP0T_L = 0;
						HB_EP0RES = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
						break;
					default:
						HB_EP0T_L = 0;  
						HB_EP0RES = UEP_R_RES_ACK | UEP_T_RES_NAK;
						break;
				}
				break;
				
			case UXS_TOKEN_IN | 1:
				/* Hub EP1 IN — port status change notification sent, go back to NAK */
				HB_EP1RES ^= bUEP_X_TOG;
				if( hub_ep1_pending ) {
					/* Send 1-byte bitmask: bit N = port N changed */
					pHB_EP1_BUF[0] = hub_ep1_pending;
					hub_ep1_pending = 0;
					HB_EP1T_L = 1;
					HB_EP1RES = HB_EP1RES & ~MASK_UEP_X_RES | UEP_X_RES_ACK;
				} else {
					HB_EP1RES = HB_EP1RES & ~MASK_UEP_X_RES | UEP_X_RES_NAK;
				}
				break;

			case UXS_TOKEN_OUT | 0:
				HB_EP0RES = UEP_R_RES_ACK | UEP_T_RES_NAK;
				break;

			default:
handle_hb_ep0_setup:
				len = 0;
				HB_EP0RES = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
				HBSetupLen = pHB_SETUP_REQ->wLengthL + ( (UINT16)pHB_SETUP_REQ->wLengthH << 8 );
				HBSetupReqCode = pHB_SETUP_REQ->bRequest;

				// CLASS Requests (Hub class: GetStatus, SetFeature, ClearFeature, GetDescriptor)
				if( ( pHB_SETUP_REQ->bRequestType & USB_REQ_TYP_MASK ) == USB_REQ_TYP_CLASS )
				{
					switch( HBSetupReqCode )
					{
						case USB_GET_DESCRIPTOR: // 0x06 — Hub Descriptor
							if( pHB_SETUP_REQ->wValueH == 0x29 )
							{
								pHBDescr = (PUINT8C)(&HubDescriptor);
								len = sizeof(USB_Descriptor_Hub_t);
								if( HBSetupLen > len ) HBSetupLen = len;
								len = HBSetupLen >= DEF_ENDP0_SIZE ? DEF_ENDP0_SIZE : HBSetupLen;
								HBSetupLen -= len;
								{ UINT8 i; for( i = 0; i < len; i++ ) pHB_EP0_BUF[i] = pHBDescr[i]; }
								pHBDescr += len;
							}
							else { len = 0xFFFF; }
							break;

						case 0x03: // SetFeature
						{
							/* wIndex = port number (1-based), wValue = feature selector */
							UINT8 port = pHB_SETUP_REQ->wIndexL; /* 1..3, or 0 for hub */
							UINT8 feat = pHB_SETUP_REQ->wValueL;
							if( port >= 1 && port <= 3 ) {
								if( feat == 8 ) { /* PORT_POWER */
									hub_port_status[port-1] |= HUB_PORT_POWER;
									/* If a device is physically present on this port, re-arm
									 * C_CONNECTION so the host re-enumerates after power cycle. */
									if( port == 1 ) { /* D0/keyboard always present */
										hub_port_status[port-1] |= HUB_PORT_CONNECTED;
										hub_port_change[port-1] |= HUB_C_CONNECTION;
										hub_ep1_pending |= (1 << port);
										HB_EP1RES = HB_EP1RES & ~MASK_UEP_X_RES | UEP_X_RES_ACK;
									}
								} else if( feat == 4 ) { /* PORT_RESET */
									/* Actually reset the sub-device SIE so it can enumerate
									 * at address 0 with clean state (clear any prior STALL,
									 * reset toggles, reset address). Enable bUX_DEV_EN now
									 * — sub-devices start disabled to avoid address-0 collision
									 * with the hub during initial enumeration. */
									if( port == 1 ) {
										D0_ADDR = 0;
										D0_EP0RES = UEP_R_RES_ACK | UEP_T_RES_NAK;
										D0_EP0T_L = 0;
										D0SetupReqCode = 0xFF;
										D0SetupLen = 0;
										D0UsbConfig = 0;
										D0_EP_MOD |= bUX_DEV_EN;
									} else if( port == 2 ) {
										D1_ADDR = 0;
										D1_EP0RES = UEP_R_RES_ACK | UEP_T_RES_NAK;
										D1_EP0T_L = 0;
										D1SetupReqCode = 0xFF;
										D1SetupLen = 0;
										D1UsbConfig = 0;
										D1_EP_MOD |= bUX_DEV_EN;
									} else if( port == 3 ) {
										D2_ADDR = 0;
										D2_EP0RES = UEP_R_RES_ACK | UEP_T_RES_NAK;
										D2_EP0T_L = 0;
										D2SetupReqCode = 0xFF;
										D2SetupLen = 0;
										D2UsbConfig = 0;
										D2_EP_MOD |= bUX_DEV_EN;
									}
									hub_port_status[port-1] |= HUB_PORT_CONNECTED | HUB_PORT_ENABLED;
									hub_port_change[port-1] |= HUB_C_RESET;
									hub_ep1_pending |= (1 << port); /* arm EP1 IN notification */
									HB_EP1RES = HB_EP1RES & ~MASK_UEP_X_RES | UEP_X_RES_ACK;
								}
							}
							len = 0; /* ACK with zero-length status */
							break;
						}

						case 0x01: // ClearFeature
						{
							UINT8 port = pHB_SETUP_REQ->wIndexL;
							UINT8 feat = pHB_SETUP_REQ->wValueL;
							if( port >= 1 && port <= 3 ) {
								if( feat == 20 ) { /* C_PORT_RESET */
									hub_port_change[port-1] &= ~HUB_C_RESET;
								} else if( feat == 16 ) { /* C_PORT_CONNECTION */
									hub_port_change[port-1] &= ~HUB_C_CONNECTION;
								} else if( feat == 1 ) { /* PORT_ENABLE — host disables port during retry */
									hub_port_status[port-1] &= ~HUB_PORT_ENABLED;
								} else if( feat == 8 ) { /* PORT_POWER — host power-cycling the port */
									hub_port_status[port-1] &= ~(HUB_PORT_POWER | HUB_PORT_ENABLED);
								}
							}
							len = 0;
							break;
						}

						case 0x00: // GetStatus
						{
							UINT8 port = pHB_SETUP_REQ->wIndexL;
							if( pHB_SETUP_REQ->bRequestType == 0xA3 && port >= 1 && port <= 3 ) {
								/* Port status: wPortStatus | wPortChange */
								UINT16 ps = hub_port_status[port-1];
								UINT16 pc = hub_port_change[port-1];
								pHB_EP0_BUF[0] = (UINT8)(ps & 0xFF);
								pHB_EP0_BUF[1] = (UINT8)(ps >> 8);
								pHB_EP0_BUF[2] = (UINT8)(pc & 0xFF);
								pHB_EP0_BUF[3] = (UINT8)(pc >> 8);
							} else {
								/* Hub status: no local power issue, no overcurrent */
								pHB_EP0_BUF[0] = 0x00;
								pHB_EP0_BUF[1] = 0x00;
								pHB_EP0_BUF[2] = 0x00;
								pHB_EP0_BUF[3] = 0x00;
							}
							len = 4;
							if( HBSetupLen > len ) HBSetupLen = len;
							len = HBSetupLen >= DEF_ENDP0_SIZE ? DEF_ENDP0_SIZE : HBSetupLen;
							HBSetupLen -= len;
							break;
						}

						default:
							len = 0xFFFF;
							break;
					}
				}
				// STANDARD Requests
				else if( ( pHB_SETUP_REQ->bRequestType & USB_REQ_TYP_MASK ) == USB_REQ_TYP_STANDARD )
				{
					switch( HBSetupReqCode )
					{
						case USB_GET_DESCRIPTOR:
							switch( pHB_SETUP_REQ->wValueH )
							{
								case 0x01: // Device
									pHBDescr = (PUINT8C)(&HubDeviceDescriptor);
									len = sizeof(USB_Descriptor_Device_t);
									break;
								case 0x02: // Configuration
									pHBDescr = (PUINT8C)(&HubConfigurationDescriptor);
									len = sizeof(USB_Descriptor_Hub_Configuration_t);
									break;
								case 0x29: // Hub Descriptor
									pHBDescr = (PUINT8C)(&HubDescriptor);
									len = sizeof(USB_Descriptor_Hub_t);
									break;
								default:
									len = 0xFFFF;
									break;
							}
							if( len != 0xFFFF )
							{
								UINT8 i;
								if( HBSetupLen > len ) HBSetupLen = len;
								len = HBSetupLen >= DEF_ENDP0_SIZE ? DEF_ENDP0_SIZE : HBSetupLen;
								HBSetupLen -= len;
								for( i = 0; i < len; i++ ) pHB_EP0_BUF[i] = pHBDescr[i];
								pHBDescr += len;
							}
							break;

						case USB_SET_ADDRESS:
							HBNewAddress = pHB_SETUP_REQ->wValueL;
							len = 0;
							break;

						case USB_SET_CONFIGURATION:
							/* Hub configured — arm EP1 IN; ACK immediately if a port change is pending */
							HB_EP1RES = hub_ep1_pending ? UEP_X_RES_ACK : UEP_X_RES_NAK;
							break;

						case USB_GET_STATUS:
							pHB_EP0_BUF[0] = 0x00;
							pHB_EP0_BUF[1] = 0x00;
							len = 2;
							if( HBSetupLen > len ) HBSetupLen = len;
							len = HBSetupLen >= DEF_ENDP0_SIZE ? DEF_ENDP0_SIZE : HBSetupLen;
							HBSetupLen -= len;
							HB_EP0T_L = len;
							break;

						default:
							len = 0xFFFF;
							break;
					}
				}
				else
				{
					len = 0xFFFF;
				}

				if (len == 0xFFFF) {
					HBSetupReqCode = 0xFF;
					HB_EP0RES = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;
				}
				else
					HB_EP0T_L = len;
				break;
		}
		HB_STATUS = 0;   /* clear bUX_IF_HB_TRANS — same as D0_STATUS=0 clears bUX_IF_D0_TRANS */
	}
	else if( us & bUX_IF_D0_TRANS )            
	{
		//dprintf("%x\n",SP);
		//dprint("d0tr");
		/* D0 data transfer completed */
		
		/* INCREDIBLY IMPORTANT: bUXS_SETUP_ACT indicates a SETUP packet arrived.
		 * The CH555 specification dictates that MASK_UXS_TOKEN retains its LAST
		 * transaction value when a SETUP packet arrives. Therefore, we MUST NOT
		 * include bUXS_SETUP_ACT in the switch statement below, or we will
		 * incorrectly jump to a random enpoint's handler and ignore the SETUP! */
		if( D0_STATUS & bUXS_SETUP_ACT )
		{
			goto handle_ep0_setup;
		}

		switch( D0_STATUS & ( MASK_UXS_TOKEN | MASK_UXS_ENDP ) )     
		{
			/* Analyze action tokens and endpoint numbers */

		case UXS_TOKEN_IN | 0:
            	/* endpoint 0# IN — normal continuation (no new SETUP) */
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

				case USB_SET_FEATURE:
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
				D0_EP4RES = D0_EP4RES & ~MASK_UEP_X_RES | UEP_X_RES_NAK; 		/* Pause download */
				ep4_data_wait = 0x01;
                break; 
#endif
#ifdef USE_D0_EP3_OUT
			case UXS_TOKEN_OUT | 3:  
				/*  endpoint 3# download */
				if( D0_STATUS & bUXS_TOG_OK ) 
				D0_EP3RES = D0_EP3RES & ~MASK_UEP_X_RES | UEP_X_RES_NAK; 		/* Pause download */
				ep3_data_wait = 0x01;
                break; 
#endif
#ifdef USE_D0_EP2_OUT
			case UXS_TOKEN_OUT | 2:  
				/*  endpoint 2# download */
				if( D0_STATUS & bUXS_TOG_OK ) 
				D0_EP2RES = D0_EP2RES & ~MASK_UEP_X_RES | UEP_X_RES_NAK; 		/* Pause download */
				ep2_data_wait = 0x01;
                break; 
#endif
#ifdef USE_D0_EP1_OUT
			case UXS_TOKEN_OUT | 1:  
				/*  endpoint 1# download */
				if( D0_STATUS & bUXS_TOG_OK ) 
				D0_EP1RES = D0_EP1RES & ~MASK_UEP_X_RES | UEP_X_RES_NAK; 		/* Pause download */
				ep1_data_wait = 0x01;
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
 				/* endpoint 0# OUT — normal data/status phase */
				switch( D0SetupReqCode ) 
				{
					case HID_SET_REPORT:
						KB_USB_SetReport = pD0_EP0_BUF[ 0 ];
						D0_EP0RES ^= bUEP_R_TOG;
                        break;
                        
					case USB_GET_DESCRIPTOR:
					
					default:
						D0_EP0RES = UEP_R_RES_ACK | UEP_T_RES_NAK;  
						break;
				}
                break;
			
			default:
handle_ep0_setup:
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
												/* Use raw byte array to bypass SDCC struct issues */
												pD0Descr = (PUINT8C)( RawManufacturerString );
												len = RawManufacturerString[0];
												break;
																			
											case 2:
												/* Use raw byte array to bypass SDCC struct issues */
												pD0Descr = (PUINT8C)( RawProductString );
												len = RawProductString[0];
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

	#ifndef QMK_MCU_CH555
										//case 2:
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
#endif /* !QMK_MCU_CH555 */
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

#ifndef QMK_MCU_CH555
											//case 2:
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
#endif /* !QMK_MCU_CH555 */
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
								USB_EnumStatus = 0x01;
								KB_USB_UpStatus = 0x01;
								break;			
																
							case USB_SET_FEATURE:
								if( ( pD0_SETUP_REQ->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_ENDP ) 
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
								else if( ( pD0_SETUP_REQ->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_DEVICE )
								{
									if( pD0_SETUP_REQ->wValueL == 1 )
									{ 
										/* Set sleep */
										USB_SleepStatus |= 0x01; 
									}		
								}
								else
								{
									len = 0xFFFF;
								}															
								break;
													
							case USB_SET_INTERFACE:
								break;
								
							case USB_CLEAR_FEATURE:
								if( ( pD0_SETUP_REQ->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_ENDP ) 
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
								else if( ( pD0_SETUP_REQ->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_DEVICE )
								{
									if( pD0_SETUP_REQ->wValueL == 1 )
									{ 
										USB_SleepStatus &= ~0x01;                      
									}
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
								if( ( pD0_SETUP_REQ->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_ENDP ) 
								{  
									/* endpoint */
									pD0_EP0_BUF[ 0 ] = 0x00;
									switch( pD0_SETUP_REQ->wIndexL ) 
									{
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
										}
								}
								else if( ( pD0_SETUP_REQ->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_DEVICE )
								{
									pD0_EP0_BUF[ 0 ] = 0x00;
									if( USB_SleepStatus )
									{
										pD0_EP0_BUF[ 0 ] = 0x02;
									}
									else
									{
										pD0_EP0_BUF[0] = 0x00;
									}
								}																	
								pD0_EP0_BUF[ 1 ] = 0;
								if( D0SetupLen >= 2 ) 
								{
									len = 2;
								}
								else 
								{
									len = D0SetupLen;
								}
								break;
								
							default:
								/* operation failed */
								len = 0xFFFF;  
								break;
						}
					}
					else if( ( pD0_SETUP_REQ->bRequestType & USB_REQ_TYP_MASK ) != USB_REQ_TYP_STANDARD ) 
					{  
						/* non-standard request */
						
						/*Other requests, such as class requests, manufacturer requests, etc.*/	
						if( pD0_SETUP_REQ->bRequestType & 0x40 )		 		
						{    
							/* Vendor request */
						}		
						else if( pD0_SETUP_REQ->bRequestType & 0x20 )	 		
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
									Report_Value = pD0_EP0_BUF[ 2 ];
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
					}
					else
					{  
						/* Only supports standard requests/class requests */
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
	else if( us & bUX_IF_D1_TRANS )
	{
		if( D1_STATUS & bUXS_SETUP_ACT )
		{
			goto handle_d1_ep0_setup;
		}

		switch( D1_STATUS & ( MASK_UXS_TOKEN | MASK_UXS_ENDP ) )     
		{
#ifdef USE_D1_EP1_IN
			case UXS_TOKEN_IN | 1:
				/* D1 EP1 IN: NKRO report sent, pause until next report */
				D1_EP1RES ^= bUEP_X_TOG;
				D1_EP1RES = D1_EP1RES & ~MASK_UEP_X_RES | UEP_X_RES_NAK;
				break;
#endif

			case UXS_TOKEN_IN | 0:
				switch( D1SetupReqCode )
				{
					case USB_GET_DESCRIPTOR:
					{
						UINT8 i;
						len = D1SetupLen >= DEF_ENDP0_SIZE ? DEF_ENDP0_SIZE : D1SetupLen;
						for( i = 0; i < len; i++ ) {
							pD1_EP0_BUF[i] = pD1Descr[i];
						}
						D1SetupLen -= len;
						pD1Descr   += len;
						D1_EP0T_L   = len;
						D1_EP0RES  ^= bUEP_T_TOG;
						break;
					}
					case USB_SET_ADDRESS:
						D1_ADDR = D1SetupLen;
						D1_EP0RES = UEP_R_RES_ACK | UEP_T_RES_NAK;
						break;
					default:
						D1_EP0T_L = 0;  
						D1_EP0RES = UEP_R_RES_ACK | UEP_T_RES_NAK;
						break;
				}
				break;

			case UXS_TOKEN_OUT | 0:  
				switch( D1SetupReqCode ) 
				{
					case HID_SET_REPORT: 
						D1_EP0RES ^= bUEP_R_TOG;                      
						break;
					case USB_GET_DESCRIPTOR:
					default:
						D1_EP0RES = UEP_R_RES_ACK | UEP_T_RES_NAK;  
						break;
				}
				break;
			
			default:
handle_d1_ep0_setup:
					len = 0;
					D1_EP0RES = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
					D1SetupLen = pD1_SETUP_REQ->wLengthL + ( (UINT16)pD1_SETUP_REQ->wLengthH << 8 );
					D1SetupReqCode = pD1_SETUP_REQ->bRequest;
					
					if( ( pD1_SETUP_REQ->bRequestType & USB_REQ_TYP_MASK ) == USB_REQ_TYP_STANDARD )
					{ 
						switch( D1SetupReqCode ) 
						{  
							case USB_GET_DESCRIPTOR:            
								switch( pD1_SETUP_REQ->wValueH )
								{
									case USB_DESCR_TYP_DEVICE:  
										pD1Descr = (PUINT8C)( &DeviceDescriptor );
										len = sizeof( USB_Descriptor_Device_t );
										break;
									case USB_DESCR_TYP_STRING:  
										switch( pD1_SETUP_REQ->wValueL ) 
										{
											case 1: pD1Descr = (PUINT8C)( RawManufacturerString ); len = RawManufacturerString[0]; break;
											case 2: pD1Descr = (PUINT8C)( RawProductString ); len = RawProductString[0]; break;
											case 0: pD1Descr = (PUINT8C)( &LanguageString ); len = LanguageString.Header.Size; break;
											default: len = 0xFFFF; break;
										}
										break;
									case USB_DESCR_TYP_CONFIG:
#ifdef SHARED_EP_ENABLE
										pD1Descr = (PUINT8C)( &ConfigurationDescriptor_D1 );
										len = sizeof( USB_Descriptor_Configuration_D1_t );
#else
										len = 0xFFFF;
#endif
										break;
									case USB_DESCR_TYP_HID:
										if ( pD1_SETUP_REQ->wIndexL == 0 ) {
#ifdef SHARED_EP_ENABLE
											pD1Descr = (PUINT8C)( &ConfigurationDescriptor_D1.Shared_HID );
											len = sizeof(USB_HID_Descriptor_HID_t);
#else
											len = 0xFFFF;
#endif
										} else {
											len = 0xFFFF;
										}
										break;
									case USB_DESCR_TYP_REPORT:
										if ( pD1_SETUP_REQ->wIndexL == 0 ) {
#ifdef SHARED_EP_ENABLE
											pD1Descr = (PUINT8C)( SharedReport );
											len = SharedReport_size;
#else
											len = 0xFFFF;
#endif
										} else {
											len = 0xFFFF;
										}
										break;
									default:
										len = 0xFFFF;
										break;
								} 
								if( len != 0xFFFF )
								{
									UINT8 i;
									if( D1SetupLen > len ) D1SetupLen = len;
									len = D1SetupLen >= DEF_ENDP0_SIZE ? DEF_ENDP0_SIZE : D1SetupLen;
									D1SetupLen -= len;
									for( i = 0; i < len; i++ ) pD1_EP0_BUF[i] = pD1Descr[i];
									pD1Descr += len;
								}
								break;
							case USB_SET_ADDRESS:               
								D1SetupLen = pD1_SETUP_REQ->wValueL;
								break;
							case USB_GET_CONFIGURATION:
								pD1_EP0_BUF[ 0 ] = D1UsbConfig;
								if( D1SetupLen >= 1 ) len = 1;
								break; 
							case USB_SET_CONFIGURATION:
								D1UsbConfig = pD1_SETUP_REQ->wValueL;
								D1_EP1RES &= ~bUEP_X_TOG;  /* the host restarts at DATA0 */
								break;			
							default:
								len = 0xFFFF;  
								break;
						}
					}
					else if( ( pD1_SETUP_REQ->bRequestType & USB_REQ_TYP_MASK ) == USB_REQ_TYP_CLASS )
					{
						switch( D1SetupReqCode ) 
						{
							case DEF_USB_GET_IDLE:
								if( D1SetupLen >= 1 ) len = 1;
								pD1_EP0_BUF[ 0 ] = 0; 
								break;
							case DEF_USB_SET_IDLE:                 
							case DEF_USB_SET_PROTOCOL:                 
								len = 0;
								break;
							default:
								len = 0xFFFF;  
								break;
						}
					}
					else
					{
						len = 0xFFFF;
					}															

					if( len == 0xFFFF )
					{
						D1SetupReqCode = 0xFF;
						D1_EP0RES = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;
					}
					else if( len <= DEF_ENDP0_SIZE )
					{
						D1_EP0T_L = len;
						D1_EP0RES = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
					}
					else
					{
						D1_EP0T_L = 0;
						D1_EP0RES = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
					}
					break;
		}
		D1_STATUS = 0;
	}
	else if( us & bUX_IF_D2_TRANS )
	{
		if( D2_STATUS & bUXS_SETUP_ACT )
		{
			goto handle_d2_ep0_setup;
		}

		switch( D2_STATUS & ( MASK_UXS_TOKEN | MASK_UXS_ENDP ) )     
		{
#ifdef USE_D2_EP1_IN
			case UXS_TOKEN_IN | 1:
				/* D2 EP1 IN: RAW HID report sent, pause until next */
				D2_EP1RES ^= bUEP_X_TOG;
				D2_EP1RES = D2_EP1RES & ~MASK_UEP_X_RES | UEP_X_RES_NAK;
				break;
#endif
#ifdef USE_D2_EP2_OUT
			case UXS_TOKEN_OUT | 2:
				/* D2 EP2 OUT: RAW HID data received from host */
				if( D2_STATUS & bUXS_TOG_OK )
					D2_EP2RES = D2_EP2RES & ~MASK_UEP_X_RES | UEP_X_RES_NAK;
				ep_d2ep2_data_wait = 0x01;
				break;
#endif

			case UXS_TOKEN_IN | 0:
				switch( D2SetupReqCode )
				{
					case USB_GET_DESCRIPTOR:
					{
						UINT8 i;
						len = D2SetupLen >= DEF_ENDP0_SIZE ? DEF_ENDP0_SIZE : D2SetupLen;
						for( i = 0; i < len; i++ ) {
							pD2_EP0_BUF[i] = pD2Descr[i];
						}
						D2SetupLen -= len;
						pD2Descr   += len;
						D2_EP0T_L   = len;
						D2_EP0RES  ^= bUEP_T_TOG;
						break;
					}
					case USB_SET_ADDRESS:
						D2_ADDR = D2SetupLen;
						D2_EP0RES = UEP_R_RES_ACK | UEP_T_RES_NAK;
						break;
					default:
						D2_EP0T_L = 0;  
						D2_EP0RES = UEP_R_RES_ACK | UEP_T_RES_NAK;
						break;
				}
				break;

			case UXS_TOKEN_OUT | 0:  
				switch( D2SetupReqCode ) 
				{
					case HID_SET_REPORT: 
						D2_EP0RES ^= bUEP_R_TOG;                      
						break;
					case USB_GET_DESCRIPTOR:
					default:
						D2_EP0RES = UEP_R_RES_ACK | UEP_T_RES_NAK;  
						break;
				}
				break;
			
			default:
handle_d2_ep0_setup:
					len = 0;
					D2_EP0RES = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
					D2SetupLen = pD2_SETUP_REQ->wLengthL + ( (UINT16)pD2_SETUP_REQ->wLengthH << 8 );
					D2SetupReqCode = pD2_SETUP_REQ->bRequest;
					
					if( ( pD2_SETUP_REQ->bRequestType & USB_REQ_TYP_MASK ) == USB_REQ_TYP_STANDARD )
					{ 
						switch( D2SetupReqCode ) 
						{  
							case USB_GET_DESCRIPTOR:            
								switch( pD2_SETUP_REQ->wValueH )
								{
									case USB_DESCR_TYP_DEVICE:  
										pD2Descr = (PUINT8C)( &DeviceDescriptor );
										len = sizeof( USB_Descriptor_Device_t );
										break;
									case USB_DESCR_TYP_STRING:  
										switch( pD2_SETUP_REQ->wValueL ) 
										{
											case 1: pD2Descr = (PUINT8C)( RawManufacturerString ); len = RawManufacturerString[0]; break;
											case 2: pD2Descr = (PUINT8C)( RawProductString ); len = RawProductString[0]; break;
											case 0: pD2Descr = (PUINT8C)( &LanguageString ); len = LanguageString.Header.Size; break;
											default: len = 0xFFFF; break;
										}
										break;
									case USB_DESCR_TYP_CONFIG:
#if defined(RAW_ENABLE) || defined(CONSOLE_ENABLE)
										pD2Descr = (PUINT8C)( &ConfigurationDescriptor_D2 );
										len = sizeof( USB_Descriptor_Configuration_D2_t );
#else
										len = 0xFFFF;
#endif
										break;
									case USB_DESCR_TYP_HID:
										switch ( pD2_SETUP_REQ->wIndexL ) {
#ifdef RAW_ENABLE
											case D2_RAW_INTERFACE_NUM:
												pD2Descr = (PUINT8C)( &ConfigurationDescriptor_D2.Raw_HID );
												len = sizeof(USB_HID_Descriptor_HID_t);
												break;
#endif
#ifdef CONSOLE_ENABLE
											case D2_CONSOLE_INTERFACE_NUM:
												pD2Descr = (PUINT8C)( &ConfigurationDescriptor_D2.Console_HID );
												len = sizeof(USB_HID_Descriptor_HID_t);
												break;
#endif
											default:
												len = 0xFFFF;
												break;
										}
										break;
									case USB_DESCR_TYP_REPORT:
										switch ( pD2_SETUP_REQ->wIndexL ) {
#ifdef RAW_ENABLE
											case D2_RAW_INTERFACE_NUM:
												pD2Descr = (PUINT8C)( RawReport );
												len = RawReport_size;
												break;
#endif
#ifdef CONSOLE_ENABLE
											case D2_CONSOLE_INTERFACE_NUM:
												pD2Descr = (PUINT8C)( ConsoleReport );
												len = ConsoleReport_size;
												break;
#endif
											default:
												len = 0xFFFF;
												break;
										}
										break;
									default:
										len = 0xFFFF;
										break;
								} 
								if( len != 0xFFFF )
								{
									UINT8 i;
									if( D2SetupLen > len ) D2SetupLen = len;
									len = D2SetupLen >= DEF_ENDP0_SIZE ? DEF_ENDP0_SIZE : D2SetupLen;
									D2SetupLen -= len;
									for( i = 0; i < len; i++ ) pD2_EP0_BUF[i] = pD2Descr[i];
									pD2Descr += len;
								}
								break;
							case USB_SET_ADDRESS:               
								D2SetupLen = pD2_SETUP_REQ->wValueL;
								break;
							case USB_GET_CONFIGURATION:
								pD2_EP0_BUF[ 0 ] = D2UsbConfig;
								if( D2SetupLen >= 1 ) len = 1;
								break; 
							case USB_SET_CONFIGURATION:
								D2UsbConfig = pD2_SETUP_REQ->wValueL;
								D2_EP1RES &= ~bUEP_X_TOG;  /* the host restarts at DATA0 */
								D2_EP2RES &= ~bUEP_X_TOG;
								break;			
							default:
								len = 0xFFFF;  
								break;
						}
					}
					else if( ( pD2_SETUP_REQ->bRequestType & USB_REQ_TYP_MASK ) == USB_REQ_TYP_CLASS )
					{
						switch( D2SetupReqCode ) 
						{
							case DEF_USB_GET_IDLE:
								if( D2SetupLen >= 1 ) len = 1;
								pD2_EP0_BUF[ 0 ] = 0;
								break;
							case DEF_USB_SET_IDLE:                 
							case DEF_USB_SET_PROTOCOL:                 
								len = 0;
								break;
							default:
								len = 0xFFFF;  
								break;
						}
					}
					else
					{
						len = 0xFFFF;
					}															

					if( len == 0xFFFF )
					{
						D2SetupReqCode = 0xFF;
						D2_EP0RES = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;
					}
					else if ( len <= DEF_ENDP0_SIZE )
					{
						D2_EP0T_L = len;
						D2_EP0RES = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
					}
					else
					{
						D2_EP0T_L = 0;
						D2_EP0RES = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
					}
					break;
		}
		D2_STATUS = 0;
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
		USB_IF = bUX_IF_SUSPEND;
		D0_STATUS = 0;
    }
	else if(us & bUX_IF_BUS_RST) 
    {
    	/* Bus reset event — host is resetting the device.
		 * Must fully reinitialize all USB state per USB spec. */
        USB_EP_init();                           /* Re-init endpoints with clean toggle bits (DATA0) */
        D0_ADDR = 0;
        D1_ADDR = 0;
        D2_ADDR = 0;
        HB_ADDR = 0x00;
        USB_IF = 0xFF;								 /* Clear all pending interrupt flags */

        USB_IE = bUX_IE_SUSPEND | bUX_IE_TRANSFER | bUX_IE_BUS_RST;

		/* Fully reset all state variables */
        KB_USB_UpStatus = 0x00;
		USB_EnumStatus = 0x00;
		D0UsbConfig = 0x00;								 /* Not yet configured */
		D0SetupReqCode = 0xFF;							 /* No pending request */
		D0SetupLen = 0x00;
		HBSetupReqCode = 0xFF;
		HBSetupLen = 0x00;
		D1SetupReqCode = 0xFF;
		D1SetupLen = 0x00;
		D1UsbConfig = 0x00;
		D2SetupReqCode = 0xFF;
		D2SetupLen = 0x00;
		D2UsbConfig = 0x00;
		hub_port_status[0] = HUB_PORT_POWER | HUB_PORT_CONNECTED;
		hub_port_change[0] = HUB_C_CONNECTION;
		hub_ep1_pending    = 0x02;
#ifdef SHARED_EP_ENABLE
		hub_port_status[1] = HUB_PORT_POWER | HUB_PORT_CONNECTED;
		hub_port_change[1] = HUB_C_CONNECTION;
		hub_ep1_pending   |= 0x04;
#else
		hub_port_status[1] = HUB_PORT_POWER;
		hub_port_change[1] = 0;
#endif
#ifdef RAW_ENABLE
		hub_port_status[2] = HUB_PORT_POWER | HUB_PORT_CONNECTED;
		hub_port_change[2] = HUB_C_CONNECTION;
		hub_ep1_pending   |= 0x08;
#else
		hub_port_status[2] = HUB_PORT_POWER;
		hub_port_change[2] = 0;
#endif
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
#ifdef USE_D2_EP2_OUT
		ep_d2ep2_data_wait = 0x00;
#endif

        USB_IF = bUX_IF_BUS_RST;
	}
	else
    {
    	/*It is an interrupt that does not need to be processed. Clear it directly.*/
        //print("@\n");
		//printf("%x\n",us);
		//printf("%x\n",D0_STATUS);
		//printf("%x\n",SP);
        D0_STATUS = 0;
	}
  	
  	/* Determine again whether there are still USB interrupts that need to be processed */
	if( USB_IF & ( bUX_IF_HB_TRANS | bUX_IF_D0_TRANS | bUX_IF_D1_TRANS | bUX_IF_D2_TRANS | bUX_IF_SUSPEND | bUX_IF_BUS_RST ) )
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
	
	dprint("sleepin...");
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

