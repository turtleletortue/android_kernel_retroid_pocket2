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

/******************************************************************************
*
 *
 * Filename:
 * ---------
 *   AudDrv_Common.h
 *
 * Project:
 * --------
 *   MT6583 FPGA LDVT Audio Driver
 *
 * Description:
 * ------------
 *   Audio register
 *
 * Author:
 * -------
 *   Chipeng Chang (MTK02308)
 *
 *---------------------------------------------------------------------------
---
 *
 *

*******************************************************************************/

#ifndef AUDIO_GLOBAL_H
#define AUDIO_GLOBAL_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/jiffies.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <mach/irqs.h>
#include <mt-plat/sync_write.h>
/*#include <linux/xlog.h>*/
/*#include <mach/mt_typedefs.h>*/
#include <linux/types.h>
#include "AudDrv_Def.h"
#include <mach/mt_clkmgr.h>

#define DL_ABNORMAL_CONTROL_MAX (5)

struct AFE_BLOCK_T {
	uint32_t pucPhysBufAddr;
	uint8_t *pucVirtBufAddr;
	int32_t u4BufferSize;
	int32_t u4DataRemained;
	uint32_t u4SampleNumMask;	/* sample number mask */
	uint32_t u4SamplesPerInt;	/* number of samples to play before interrupting */
	int32_t u4WriteIdx;	/* Previous Write Index. */
	int32_t u4DMAReadIdx;	/* Previous DMA Read Index. */
	uint32_t u4MaxCopySize;
	uint32_t u4fsyncflag;
	uint32_t uResetFlag;
};

struct substreamList {
	struct snd_pcm_substream *substream;
	uint32_t u4MaxCopySize;
	struct substreamList *next;
};


struct AFE_MEM_CONTROL_T {
	struct file *flip;
	struct substreamList *substreamL;
	struct AFE_BLOCK_T rBlock;
	uint32_t MemIfNum;
	bool interruptTrigger;
	spinlock_t substream_lock;
	bool mAssignDRAM;
};

struct pcm_afe_info {
	struct AFE_BLOCK_T *mAfeBlock;
	struct snd_pcm_substream *substream;
};


struct AFE_DL_ABNORMAL_CONTROL_T {
	int32_t u4BufferSize[DL_ABNORMAL_CONTROL_MAX];
	int32_t u4DataRemained[DL_ABNORMAL_CONTROL_MAX];
	int32_t u4WriteIdx[DL_ABNORMAL_CONTROL_MAX];          /* Previous Write Index. */
	int32_t u4DMAReadIdx[DL_ABNORMAL_CONTROL_MAX];        /* Previous DMA Read Index. */
	int32_t u4ConsumedBytes[DL_ABNORMAL_CONTROL_MAX];
	int32_t u4HwMemoryIndex[DL_ABNORMAL_CONTROL_MAX];
	int32_t pucPhysBufAddr[DL_ABNORMAL_CONTROL_MAX];
	int32_t u4UnderflowCnt;
	uint32_t MemIfNum[DL_ABNORMAL_CONTROL_MAX];
	unsigned long long IrqLastTimeNs[DL_ABNORMAL_CONTROL_MAX];
	unsigned long long IrqCurrentTimeNs[DL_ABNORMAL_CONTROL_MAX];
	unsigned long long IrqIntervalNs[DL_ABNORMAL_CONTROL_MAX];
	uint32_t IrqIntervalLimitMs[DL_ABNORMAL_CONTROL_MAX];
	bool IrqDelayCnt;
};

struct AFE_DL_ISR_COPY_T {
	int8_t *pBufferBase;
	int8_t *pBufferIndx;
	uint32_t u4BufferSize;
	uint32_t u4BufferSizeMax;

	int32_t u4IsrConsumeSize;
};

#endif
