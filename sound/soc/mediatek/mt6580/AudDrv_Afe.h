/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program
 * If not, see <http://www.gnu.org/licenses/>.
 */
/*******************************************************************************
 *
 * Filename:
 * ---------
 *   AudioAfe.h
 *
 * Project:
 * --------
 *   MT6580  Audio Driver Afe Register setting
 *
 * Description:
 * ------------
 *   Audio register
 *
 * Author:
 * -------
 *   Ir Lian (mtk00976)
 *   Harvey Huang (mtk03996)
 *   Chipeng Chang (mtk02308)
 *
 *------------------------------------------------------------------------------
 *
 *
 *******************************************************************************/

#ifndef _AUDDRV_AFE_H_
#define _AUDDRV_AFE_H_

#include "AudDrv_Common.h"
#include "AudDrv_Def.h"
#include <linux/types.h>

/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/

/*****************************************************************************
 *                         D A T A   T Y P E S
 *****************************************************************************/





/*****************************************************************************
 *                          C O N S T A N T S
 *****************************************************************************/
#define AUDIO_HW_PHYSICAL_BASE  (0x11140000L)
#define AUDIO_HW_VIRTUAL_BASE   (0xF2070000L)

#ifdef AUDIO_MEM_IOREMAP
#define AFE_BASE                (0L)
#else
#define AFE_BASE                (AUDIO_HW_VIRTUAL_BASE)
#endif

/* Internal sram: 0x12004000~0x12007FFF (16K) */
#define AFE_INTERNAL_SRAM_PHY_BASE  (0x11141000L)
#define AFE_INTERNAL_SRAM_VIR_BASE  (AUDIO_HW_VIRTUAL_BASE - 0x70000+0x8000)
#define AFE_INTERNAL_SRAM_SIZE  (0x4000)

/* Dram */
#define AFE_EXTERNAL_DRAM_SIZE  (0x4000)
/*****************************************************************************
 *                         M A C R O
 *****************************************************************************/

/*****************************************************************************
 *                  R E G I S T E R       D E F I N I T I O N
 *****************************************************************************/

#ifdef AUDIO_TOP_CON0
#undef AUDIO_TOP_CON0
#define AUDIO_TOP_CON0  (AFE_BASE + 0x0000)
#endif
#define AUDIO_TOP_CON0  (AFE_BASE + 0x0000)

#define AUDIO_TOP_CON1  (AFE_BASE + 0x0004)
#define AUDIO_TOP_CON2  (AFE_BASE + 0x0008)
#define AUDIO_TOP_CON3  (AFE_BASE + 0x000C)
#define AFE_DAC_CON0    (AFE_BASE + 0x0010)
#define AFE_DAC_CON1    (AFE_BASE + 0x0014)
#define AFE_I2S_CON     (AFE_BASE + 0x0018)

#define AFE_CONN0       (AFE_BASE + 0x0020)
#define AFE_CONN1       (AFE_BASE + 0x0024)
#define AFE_CONN2       (AFE_BASE + 0x0028)
#define AFE_CONN3       (AFE_BASE + 0x002C)
#define AFE_CONN4       (AFE_BASE + 0x0030)

#define AFE_I2S_CON1    (AFE_BASE + 0x0034)
#define AFE_I2S_CON2    (AFE_BASE + 0x0038)

/* Memory interface */
#define AFE_DL1_BASE    (AFE_BASE + 0x0040)
#define AFE_DL1_CUR     (AFE_BASE + 0x0044)
#define AFE_DL1_END     (AFE_BASE + 0x0048)

#define AFE_I2S_CON3    (AFE_BASE + 0x004C)
#define AFE_DL2_BASE    (AFE_BASE + 0x0050)
#define AFE_DL2_CUR     (AFE_BASE + 0x0054)
#define AFE_DL2_END     (AFE_BASE + 0x0058)
#define AFE_AWB_BASE    (AFE_BASE + 0x0070)
#define AFE_AWB_END     (AFE_BASE + 0x0078)
#define AFE_AWB_CUR     (AFE_BASE + 0x007C)
#define AFE_VUL_BASE    (AFE_BASE + 0x0080)
#define AFE_VUL_END     (AFE_BASE + 0x0088)
#define AFE_VUL_CUR     (AFE_BASE + 0x008C)

/* Memory interface monitor */
#define AFE_MEMIF_MON0 (AFE_BASE + 0x00D0)
#define AFE_MEMIF_MON1 (AFE_BASE + 0x00D4)
#define AFE_MEMIF_MON2 (AFE_BASE + 0x00D8)
#define AFE_MEMIF_MON4 (AFE_BASE + 0x00E0)

#define AFE_ADDA_DL_SRC2_CON0   (AFE_BASE+0x00108)
#define AFE_ADDA_DL_SRC2_CON1   (AFE_BASE+0x0010C)
#define AFE_ADDA_UL_SRC_CON0    (AFE_BASE+0x00114)
#define AFE_ADDA_UL_SRC_CON1    (AFE_BASE+0x00118)
#define AFE_ADDA_TOP_CON0       (AFE_BASE+0x00120)
#define AFE_ADDA_UL_DL_CON0     (AFE_BASE+0x00124)
#define AFE_ADDA_SRC_DEBUG      (AFE_BASE+0x0012C)
#define AFE_ADDA_SRC_DEBUG_MON0 (AFE_BASE+0x00130)
#define AFE_ADDA_SRC_DEBUG_MON1 (AFE_BASE+0x00134)
#define AFE_ADDA_NEWIF_CFG0     (AFE_BASE+0x00138)
#define AFE_ADDA_NEWIF_CFG1     (AFE_BASE+0x0013C)

#define AFE_SIDETONE_DEBUG  (AFE_BASE + 0x01D0)
#define AFE_SIDETONE_MON    (AFE_BASE + 0x01D4)
#define AFE_SIDETONE_CON0   (AFE_BASE + 0x01E0)
#define AFE_SIDETONE_COEFF  (AFE_BASE + 0x01E4)
#define AFE_SIDETONE_CON1   (AFE_BASE + 0x01E8)
#define AFE_SIDETONE_GAIN   (AFE_BASE + 0x01EC)

#define AFE_SGEN_CON0   (AFE_BASE + 0x01F0)
#define AFE_TOP_CON0    (AFE_BASE + 0x0200)

#define AFE_ADDA_PREDIS_CON0    (AFE_BASE+0x00260)
#define AFE_ADDA_PREDIS_CON1    (AFE_BASE+0x00264)

#define AFE_MOD_DAI_BASE (AFE_BASE + 0x0330)
#define AFE_MOD_DAI_END  (AFE_BASE + 0x0338)
#define AFE_MOD_DAI_CUR  (AFE_BASE + 0x033C)

#define AFE_IRQ_MCU_CON             (AFE_BASE + 0x03A0)
#define AFE_IRQ_MCU_STATUS          (AFE_BASE + 0x03A4)
#define AFE_IRQ_MCU_CLR             (AFE_BASE + 0x03A8)
#define AFE_IRQ_MCU_CNT1            (AFE_BASE + 0x03AC)
#define AFE_IRQ_MCU_CNT2            (AFE_BASE + 0x03B0)
#define AFE_IRQ_MCU_MON2            (AFE_BASE + 0x03B8)

#define AFE_IRQ1_MCU_CNT_MON        (AFE_BASE + 0x03C0)

#define AFE_IRQ2_MCU_CNT_MON        (AFE_BASE + 0x03C4)
#define AFE_IRQ1_MCU_EN_CNT_MON     (AFE_BASE + 0x03C8)

#define AFE_MEMIF_MINLEN	(AFE_BASE + 0x03D0)
#define AFE_MEMIF_MAXLEN	(AFE_BASE + 0x03D4)
#define AFE_MEMIF_PBUF_SIZE         (AFE_BASE + 0x03D8)

/* AFE GAIN CONTROL REGISTER */
#define AFE_GAIN1_CON0         (AFE_BASE + 0x0410)
#define AFE_GAIN1_CON1         (AFE_BASE + 0x0414)
#define AFE_GAIN1_CON2         (AFE_BASE + 0x0418)
#define AFE_GAIN1_CON3         (AFE_BASE + 0x041C)
#define AFE_GAIN1_CONN         (AFE_BASE + 0x0420)
#define AFE_GAIN1_CUR          (AFE_BASE + 0x0424)
#define AFE_GAIN2_CON0         (AFE_BASE + 0x0428)
#define AFE_GAIN2_CON1         (AFE_BASE + 0x042C)
#define AFE_GAIN2_CON2         (AFE_BASE + 0x0430)
#define AFE_GAIN2_CON3         (AFE_BASE + 0x0434)
#define AFE_GAIN2_CONN         (AFE_BASE + 0x0438)
#define AFE_GAIN2_CUR          (AFE_BASE + 0x043C)
#define AFE_GAIN2_CONN2        (AFE_BASE + 0x0440)

/* here is only fpga needed */
#define FPGA_CFG2           (AFE_BASE + 0x4B8)
#define FPGA_CFG3           (AFE_BASE + 0x4BC)
#define FPGA_CFG0           (AFE_BASE + 0x4C0)
#define FPGA_CFG1           (AFE_BASE + 0x4C4)
/* #define FPGA_VER               (AFE_BASE + 0x4C8) */
#define FPGA_STC                (AFE_BASE + 0x4CC)

#define AFE_ASRC_CON0   (AFE_BASE + 0x500)
#define AFE_ASRC_CON1   (AFE_BASE + 0x504)
#define AFE_ASRC_CON2   (AFE_BASE + 0x508)
#define AFE_ASRC_CON3   (AFE_BASE + 0x50C)
#define AFE_ASRC_CON4   (AFE_BASE + 0x510)
#define AFE_ASRC_CON5   (AFE_BASE + 0x514)
#define AFE_ASRC_CON6   (AFE_BASE + 0x518)
#define AFE_ASRC_CON7   (AFE_BASE + 0x51C)
#define AFE_ASRC_CON8   (AFE_BASE + 0x520)
#define AFE_ASRC_CON9   (AFE_BASE + 0x524)
#define AFE_ASRC_CON10  (AFE_BASE + 0x528)
#define AFE_ASRC_CON11  (AFE_BASE + 0x52C)

#define PCM_INTF_CON    (AFE_BASE + 0x530)
#define PCM_INTF_CON2   (AFE_BASE + 0x538)
#define PCM2_INTF_CON   (AFE_BASE + 0x53C)

/* Add */
#define AFE_ASRC_CON13  (AFE_BASE+0x550)
#define AFE_ASRC_CON14  (AFE_BASE+0x554)
#define AFE_ASRC_CON15  (AFE_BASE+0x558)
#define AFE_ASRC_CON16  (AFE_BASE+0x55C)
#define AFE_ASRC_CON17  (AFE_BASE+0x560)
#define AFE_ASRC_CON18  (AFE_BASE+0x564)
#define AFE_ASRC_CON19  (AFE_BASE+0x568)
#define AFE_ASRC_CON20  (AFE_BASE+0x56C)
#define AFE_ASRC_CON21  (AFE_BASE+0x570)

#define AFE_REGISTER_OFFSET (0x574)

/* #define AFE_MASK_ALL  (0xffffffff) */

#define AFE_MAXLENGTH           (AFE_BASE + AFE_REGISTER_OFFSET)


/* do afe register ioremap */
void Auddrv_Reg_map(void);

void Afe_Set_Reg(unsigned int offset, unsigned int value, unsigned int mask);
unsigned int Afe_Get_Reg(unsigned int offset);

/* for debug usage */
void Afe_Log_Print(void);

/* function to get pointer */
dma_addr_t Get_Afe_Sram_Phys_Addr(void);
dma_addr_t Get_Afe_Sram_Capture_Phys_Addr(void);
void *Get_Afe_SramBase_Pointer(void);
void *Get_Afe_SramCaptureBase_Pointer(void);
#endif
