
/*******************************************************************
*
* CAUTION: This file is automatically generated by libgen.
* Version: Xilinx EDK 8.2.01 EDK_Im_Sp1.3
* DO NOT EDIT.
*
* Copyright (c) 2005 Xilinx, Inc.  All rights reserved. 
* 
* Description: Driver configuration
*
*******************************************************************/

#include "xparameters.h"
#include "xintc.h"


extern void XNullHandler (void *);

/*
* The configuration table for devices
*/

XIntc_Config XIntc_ConfigTable[] =
{
	{
		XPAR_OPB_INTC_0_DEVICE_ID,
		XPAR_OPB_INTC_0_BASEADDR,
		XPAR_OPB_INTC_0_KIND_OF_INTR,
		XIN_SVC_SGL_ISR_OPTION,
		{
			{
				XNullHandler,
				(void *) XNULL
			},
			{
				XNullHandler,
				(void *) XNULL
			}
		}

	}
};

