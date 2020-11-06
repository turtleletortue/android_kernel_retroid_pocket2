/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef AUDDRV_KERNEL_H
#define AUDDRV_KERNEL_H

#include "AudDrv_Common.h"
#include "AudDrv_Def.h"

/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/


/*****************************************************************************
 *                         D A T A   T Y P E S
 *****************************************************************************/

enum MEMIF_BUFFER_TYPE {
	MEM_DL1,
	MEM_DL2,
	MEM_VUL,
	MEM_DAI,
	MEM_I2S,		/* Cuurently not used. Add for sync with user space */
	MEM_AWB,
	MEM_MOD_DAI,
	NUM_OF_MEM_INTERFACE
};


enum IRQ_MCU_TYPE {
	INTERRUPT_IRQ1_MCU = 1,
	INTERRUPT_IRQ2_MCU = 2,
	INTERRUPT_IRQ3_MCU = 4,
	INTERRUPT_IRQ4_MCU = 8,
	INTERRUPT_IRQ5_MCU = 16,
};

enum {
	CLOCK_AUD_AFE = 0,
	CLOCK_AUD_I2S,
	CLOCK_AUD_ADC,
	CLOCK_AUD_DAC,
	CLOCK_AUD_LINEIN,
	CLOCK_AUD_HDMI,
	CLOCK_AUD_26M,		/* core clock */
	CLOCK_TYPE_MAX
};

#endif
