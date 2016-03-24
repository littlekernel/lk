/*
 * @brief Virtual Comm port USB descriptors
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2013
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#include "app_usbd_cfg.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/**
 * USB Standard Device Descriptor
 */
ALIGNED(4) const uint8_t USB_DeviceDescriptor[] = {
	USB_DEVICE_DESC_SIZE,				/* bLength */
	USB_DEVICE_DESCRIPTOR_TYPE,			/* bDescriptorType */
	WBVAL(0x0200),						/* bcdUSB */
	0xEF,								/* bDeviceClass */
	0x02,								/* bDeviceSubClass */
	0x01,								/* bDeviceProtocol */
	USB_MAX_PACKET0,					/* bMaxPacketSize0 */
	WBVAL(0x1FC9),						/* idVendor */
	WBVAL(0x0083),						/* idProduct */
	WBVAL(0x0100),						/* bcdDevice */
	0x01,								/* iManufacturer */
	0x02,								/* iProduct */
	0x03,								/* iSerialNumber */
	0x01								/* bNumConfigurations */
};

/**
 * USB FSConfiguration Descriptor
 * All Descriptors (Configuration, Interface, Endpoint, Class, Vendor)
 */
ALIGNED(4) uint8_t USB_FsConfigDescriptor[] = {
	/* Configuration 1 */
	USB_CONFIGURATION_DESC_SIZE,			/* bLength */
	USB_CONFIGURATION_DESCRIPTOR_TYPE,		/* bDescriptorType */
	WBVAL(									/* wTotalLength */
		USB_CONFIGURATION_DESC_SIZE     +
		USB_INTERFACE_ASSOC_DESC_SIZE   +	/* interface association descriptor */
		USB_INTERFACE_DESC_SIZE         +	/* communication control interface */
		0x0013                          +	/* CDC functions */
		1 * USB_ENDPOINT_DESC_SIZE      +	/* interrupt endpoint */
		USB_INTERFACE_DESC_SIZE         +	/* communication data interface */
		2 * USB_ENDPOINT_DESC_SIZE      +	/* bulk endpoints */
		0
		),
	0x02,									/* bNumInterfaces */
	0x01,									/* bConfigurationValue */
	0x00,									/* iConfiguration */
	USB_CONFIG_SELF_POWERED,				/* bmAttributes  */
	USB_CONFIG_POWER_MA(500),				/* bMaxPower */

	/* Interface association descriptor IAD*/
	USB_INTERFACE_ASSOC_DESC_SIZE,		/* bLength */
	USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE,	/* bDescriptorType */
	USB_CDC_CIF_NUM,					/* bFirstInterface */
	0x02,								/* bInterfaceCount */
	CDC_COMMUNICATION_INTERFACE_CLASS,	/* bFunctionClass */
	CDC_ABSTRACT_CONTROL_MODEL,			/* bFunctionSubClass */
	0x00,								/* bFunctionProtocol */
	0x04,								/* iFunction */

	/* Interface 0, Alternate Setting 0, Communication class interface descriptor */
	USB_INTERFACE_DESC_SIZE,			/* bLength */
	USB_INTERFACE_DESCRIPTOR_TYPE,		/* bDescriptorType */
	USB_CDC_CIF_NUM,					/* bInterfaceNumber: Number of Interface */
	0x00,								/* bAlternateSetting: Alternate setting */
	0x01,								/* bNumEndpoints: One endpoint used */
	CDC_COMMUNICATION_INTERFACE_CLASS,	/* bInterfaceClass: Communication Interface Class */
	CDC_ABSTRACT_CONTROL_MODEL,			/* bInterfaceSubClass: Abstract Control Model */
	0x00,								/* bInterfaceProtocol: no protocol used */
	0x04,								/* iInterface: */
	/* Header Functional Descriptor*/
	0x05,								/* bLength: CDC header Descriptor size */
	CDC_CS_INTERFACE,					/* bDescriptorType: CS_INTERFACE */
	CDC_HEADER,							/* bDescriptorSubtype: Header Func Desc */
	WBVAL(CDC_V1_10),					/* bcdCDC 1.10 */
	/* Call Management Functional Descriptor*/
	0x05,								/* bFunctionLength */
	CDC_CS_INTERFACE,					/* bDescriptorType: CS_INTERFACE */
	CDC_CALL_MANAGEMENT,				/* bDescriptorSubtype: Call Management Func Desc */
	0x01,								/* bmCapabilities: device handles call management */
	USB_CDC_DIF_NUM,					/* bDataInterface: CDC data IF ID */
	/* Abstract Control Management Functional Descriptor*/
	0x04,								/* bFunctionLength */
	CDC_CS_INTERFACE,					/* bDescriptorType: CS_INTERFACE */
	CDC_ABSTRACT_CONTROL_MANAGEMENT,	/* bDescriptorSubtype: Abstract Control Management desc */
	0x02,								/* bmCapabilities: SET_LINE_CODING, GET_LINE_CODING, SET_CONTROL_LINE_STATE supported */
	/* Union Functional Descriptor*/
	0x05,								/* bFunctionLength */
	CDC_CS_INTERFACE,					/* bDescriptorType: CS_INTERFACE */
	CDC_UNION,							/* bDescriptorSubtype: Union func desc */
	USB_CDC_CIF_NUM,					/* bMasterInterface: Communication class interface is master */
	USB_CDC_DIF_NUM,					/* bSlaveInterface0: Data class interface is slave 0 */
	/* Endpoint 1 Descriptor*/
	USB_ENDPOINT_DESC_SIZE,				/* bLength */
	USB_ENDPOINT_DESCRIPTOR_TYPE,		/* bDescriptorType */
	USB_CDC_INT_EP,						/* bEndpointAddress */
	USB_ENDPOINT_TYPE_INTERRUPT,		/* bmAttributes */
	WBVAL(0x0010),						/* wMaxPacketSize */
	0x02,			/* 2ms */           /* bInterval */

	/* Interface 1, Alternate Setting 0, Data class interface descriptor*/
	USB_INTERFACE_DESC_SIZE,			/* bLength */
	USB_INTERFACE_DESCRIPTOR_TYPE,		/* bDescriptorType */
	USB_CDC_DIF_NUM,					/* bInterfaceNumber: Number of Interface */
	0x00,								/* bAlternateSetting: no alternate setting */
	0x02,								/* bNumEndpoints: two endpoints used */
	CDC_DATA_INTERFACE_CLASS,			/* bInterfaceClass: Data Interface Class */
	0x00,								/* bInterfaceSubClass: no subclass available */
	0x00,								/* bInterfaceProtocol: no protocol used */
	0x04,								/* iInterface: */
	/* Endpoint, EP Bulk Out */
	USB_ENDPOINT_DESC_SIZE,				/* bLength */
	USB_ENDPOINT_DESCRIPTOR_TYPE,		/* bDescriptorType */
	USB_CDC_OUT_EP,						/* bEndpointAddress */
	USB_ENDPOINT_TYPE_BULK,				/* bmAttributes */
	WBVAL(USB_FS_MAX_BULK_PACKET),		/* wMaxPacketSize */
	0x00,								/* bInterval: ignore for Bulk transfer */
	/* Endpoint, EP Bulk In */
	USB_ENDPOINT_DESC_SIZE,				/* bLength */
	USB_ENDPOINT_DESCRIPTOR_TYPE,		/* bDescriptorType */
	USB_CDC_IN_EP,						/* bEndpointAddress */
	USB_ENDPOINT_TYPE_BULK,				/* bmAttributes */
	WBVAL(64),							/* wMaxPacketSize */
	0x00,								/* bInterval: ignore for Bulk transfer */
	/* Terminator */
	0									/* bLength */
};

/**
 * USB String Descriptor (optional)
 */
ALIGNED(4) const uint8_t USB_StringDescriptor[] = {
	/* Index 0x00: LANGID Codes */
	0x04,								/* bLength */
	USB_STRING_DESCRIPTOR_TYPE,			/* bDescriptorType */
	WBVAL(0x0409),	/* US English */    /* wLANGID */
	/* Index 0x01: Manufacturer */
	(3 * 2 + 2),						/* bLength (13 Char + Type + lenght) */
	USB_STRING_DESCRIPTOR_TYPE,			/* bDescriptorType */
	'N', 0,
	'X', 0,
	'P', 0,
	/* Index 0x02: Product */
	(9 * 2 + 2),						/* bLength */
	USB_STRING_DESCRIPTOR_TYPE,			/* bDescriptorType */
	'V', 0,
	'C', 0,
	'O', 0,
	'M', 0,
	' ', 0,
	'P', 0,
	'o', 0,
	'r', 0,
	't', 0,
	/* Index 0x03: Serial Number */
	(6 * 2 + 2),						/* bLength (8 Char + Type + lenght) */
	USB_STRING_DESCRIPTOR_TYPE,			/* bDescriptorType */
	'N', 0,
	'X', 0,
	'P', 0,
	'-', 0,
	'7', 0,
	'7', 0,
	/* Index 0x04: Interface 1, Alternate Setting 0 */
	( 4 * 2 + 2),						/* bLength (4 Char + Type + lenght) */
	USB_STRING_DESCRIPTOR_TYPE,			/* bDescriptorType */
	'V', 0,
	'C', 0,
	'O', 0,
	'M', 0,
};
