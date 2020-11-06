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
 *  mt_sco_afe_control.h
 *
 * Project:
 * --------
 *   MT6583  Audio Driver Kernel Function
 *
 * Description:
 * ------------
 *   Audio register
 *
 * Author:
 * -------
 * Chipeng Chang
 *
 *------------------------------------------------------------------------------
 *
 *
 *******************************************************************************/


/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/

#ifndef _AUDIO_AFE_CONTROL_H
#define _AUDIO_AFE_CONTROL_H

#include "AudDrv_Type_Def.h"
#include "AudDrv_Common.h"
#include "mt_soc_digital_type.h"
#include "AudDrv_Def.h"
#include <sound/memalloc.h>


#define GIC_PRIVATE_SIGNALS          (32)
#define AUDDRV_DL1_MAX_BUFFER_LENGTH (0x4000)
#define MT6580_AFE_MCU_IRQ_LINE (GIC_PRIVATE_SIGNALS + 80)


#define MASK_ALL          (0xFFFFFFFF)
#define AFE_MASK_ALL  (0xffffffff)

bool InitAfeControl(void);
bool ResetAfeControl(void);
bool Register_Aud_Irq(void *dev, unsigned int afe_irq_number);
void Auddrv_Reg_map(void);

bool SetSampleRate(unsigned int Aud_block, unsigned int SampleRate);
bool SetChannels(unsigned int Memory_Interface, unsigned int channel);
/*
DO NOT USER DIRECTLY, use irq manager
bool SetIrqMcuCounter(unsigned int Irqmode, unsigned int Counter);
bool SetIrqEnable(unsigned int Irqmode, bool bEnable);
bool SetIrqMcuSampleRate(unsigned int Irqmode, unsigned int SampleRate);
*/
bool SetConnection(unsigned int ConnectionState, unsigned int Input, unsigned int Output);
bool SetMemoryPathEnable(unsigned int Aud_block, bool bEnable);
bool GetMemoryPathEnable(unsigned int Aud_block);
bool SetI2SDacEnable(bool bEnable);
bool GetI2SDacEnable(void);
void EnableAfe(bool bEnable);
bool Set2ndI2SOutAttribute(uint32_t sampleRate);
bool Set2ndI2SOut(struct AudioDigtalI2S *DigtalI2S);
bool Set2ndI2SOutEnable(bool benable);
bool SetI2SAdcIn(struct AudioDigtalI2S *DigtalI2S);
bool Set2ndI2SAdcIn(struct AudioDigtalI2S *DigtalI2S);
bool SetDLSrc2(unsigned int SampleRate);
void SetULSrcEnable(bool bEnable);
void SetADDAEnable(bool bEnable);

bool GetMrgI2SEnable(void);
bool SetMrgI2SEnable(bool bEnable, unsigned int sampleRate);
bool SetDaiBt(struct AudioDigitalDAIBT *mAudioDaiBt);
bool SetDaiBtEnable(bool bEanble);

bool SetI2SAdcEnable(bool bEnable);
bool Set2ndI2SAdcEnable(bool bEnable);
bool SetI2SDacOut(unsigned int SampleRate, bool Lowgitter, bool I2SWLen);
bool SetHwDigitalGainMode(unsigned int GainType, unsigned int SampleRate, unsigned int SamplePerStep);
bool SetHwDigitalGainEnable(int GainType, bool Enable);
bool SetHwDigitalGain(unsigned int Gain, int GainType);

bool SetMemDuplicateWrite(unsigned int InterfaceType, int dupwrite);
bool EnableSideGenHw(unsigned int connection, bool direction, bool Enable);
bool SetSideGenSampleRate(unsigned int SampleRate);
bool CleanPreDistortion(void);
bool EnableSideToneFilter(bool stf_on);
bool SetModemPcmEnable(int modem_index, bool modem_pcm_on);
bool SetModemPcmConfig(int modem_index, struct AudioDigitalPCM p_modem_pcm_attribute);

bool Set2ndI2SIn(struct AudioDigtalI2S *mDigitalI2S);
bool Set2ndI2SInConfig(unsigned int sampleRate, bool bIsSlaveMode);
bool Set2ndI2SInEnable(bool bEnable);

bool SetI2SASRCConfig(bool bIsUseASRC, unsigned int dToSampleRate);
bool SetI2SASRCEnable(bool bEnable);

bool checkDllinkMEMIfStatus(void);
bool checkUplinkMEMIfStatus(void);
bool SetMemIfFetchFormatPerSample(unsigned int InterfaceType, unsigned int eFetchFormat);
bool SetoutputConnectionFormat(unsigned int ConnectionFormat, unsigned int Output);

bool SetHDMIApLL(unsigned int ApllSource);
unsigned int GetHDMIApLLSource(void);
bool SetHDMIMCLK(void);
bool SetHDMIBCLK(void);
bool SetHDMIdatalength(unsigned int length);
bool SetHDMIsamplerate(unsigned int samplerate);
bool SetHDMIConnection(unsigned int ConnectionState, unsigned int Input, unsigned int Output);
bool SetHDMIChannels(unsigned int Channels);
bool SetHDMIEnable(bool bEnable);

bool SetTDMLrckWidth(unsigned int cycles);
bool SetTDMbckcycle(unsigned int cycles);
bool SetTDMChannelsSdata(unsigned int channels);

/* Soc_Aud_I2S_WLEN_WLEN_16BITS or Soc_Aud_I2S_WLEN_WLEN_32BITS */

bool SetTDMDatalength(unsigned int length);
bool SetTDMI2Smode(unsigned int mode);
bool SetTDMLrckInverse(bool enable);
bool SetTDMBckInverse(bool enable);
bool SetTDMEnable(bool enable);

unsigned int SampleRateTransform(unsigned int SampleRate);
unsigned int SGENSampleRateTransform(unsigned int SampleRate);

/* APLL , low jitter mode setting */
unsigned int SetCLkMclk(unsigned int I2snum, unsigned int SampleRate);
void EnableI2SDivPower(unsigned int Diveder_name, bool bEnable);
void EnableApll1(bool bEnable);
void EnableApll2(bool bEnable);
void SetCLkBclk(unsigned int MckDiv, unsigned int SampleRate, unsigned int Channels, unsigned int Wlength);

int AudDrv_Allocate_mem_Buffer(struct device *pDev,
			       enum Soc_Aud_Digital_Block MemBlock, unsigned int Buffer_length);
struct AFE_MEM_CONTROL_T *Get_Mem_ControlT(enum Soc_Aud_Digital_Block MemBlock);
bool SetMemifSubStream(enum Soc_Aud_Digital_Block MemBlock, struct snd_pcm_substream *substream);
bool RemoveMemifSubStream(enum Soc_Aud_Digital_Block MemBlock, struct snd_pcm_substream *substream);
bool ClearMemBlock(enum Soc_Aud_Digital_Block MemBlock);

/* interrupt handler */

void Auddrv_Dl1_Spinlock_lock(void);
void Auddrv_Dl1_Spinlock_unlock(void);
void Auddrv_Dl2_Spinlock_lock(void);
void Auddrv_Dl2_Spinlock_unlock(void);

void Auddrv_DL1_Interrupt_Handler(void);
void Auddrv_DL2_Interrupt_Handler(void);
void Auddrv_UL1_Interrupt_Handler(void);
void Auddrv_UL1_Spinlock_lock(void);
void Auddrv_UL1_Spinlock_unlock(void);
void Auddrv_AWB_Interrupt_Handler(void);
void Auddrv_DAI_Interrupt_Handler(void);
void Auddrv_HDMI_Interrupt_Handler(void);
void Auddrv_UL2_Interrupt_Handler(void);
void Clear_Mem_CopySize(enum Soc_Aud_Digital_Block MemBlock);
uint32_t Get_Mem_CopySizeByStream(enum Soc_Aud_Digital_Block MemBlock,
				    struct snd_pcm_substream *substream);
uint32_t Get_Mem_MaxCopySize(enum Soc_Aud_Digital_Block MemBlock);
void Set_Mem_CopySizeByStream(enum Soc_Aud_Digital_Block MemBlock,
			      struct snd_pcm_substream *substream, unsigned int size);

struct snd_dma_buffer *Get_Mem_Buffer(enum Soc_Aud_Digital_Block MemBlock);
int AudDrv_Allocate_DL1_Buffer(struct device *pDev, uint32_t Afe_Buf_Length);
int AudDrv_Allocate_DL2_Buffer(struct device *pDev, uint32_t Afe_Buf_Length);


bool BackUp_Audio_Register(void);
bool Restore_Audio_Register(void);

void AfeControlMutexLock(void);
void AfeControlMutexUnLock(void);

/* Sram management  function */
void AfeControlSramLock(void);
void AfeControlSramUnLock(void);
size_t GetCaptureSramSize(void);
unsigned int GetSramState(void);
void ClearSramState(unsigned int State);
void SetSramState(unsigned int State);
unsigned int GetPLaybackSramFullSize(void);
unsigned int GetPLaybackSramPartial(void);
unsigned int GetPLaybackDramSize(void);
size_t GetCaptureDramSize(void);

/* offsetTrimming */
void OpenAfeDigitaldl1(bool bEnable);
void SetExternalModemStatus(const bool bEnable);

/* set VOW status for AFE GPIO control */
void SetVOWStatus(bool bEnable);
bool ConditionEnterSuspend(void);
void SetFMEnableFlag(bool bEnable);
void SetOffloadEnableFlag(bool bEnable);

unsigned int Align64ByteSize(unsigned int insize);

bool AudDrv_checkDLISRStatus(void);

#ifdef CONFIG_OF
int GetGPIO_Info(int type, int *pin, int *pinmode);
#endif

/* IRQ Manager */
int init_irq_manager(void);
int irq_add_user(const void *_user,
		 enum Soc_Aud_IRQ_MCU_MODE _irq,
		 unsigned int _rate,
		 unsigned int _count);
int irq_remove_user(const void *_user,
		    enum Soc_Aud_IRQ_MCU_MODE _irq);
int irq_update_user(const void *_user,
		    enum Soc_Aud_IRQ_MCU_MODE _irq,
		    unsigned int _rate,
		    unsigned int _count);

/* IRQ Manager */

/* low latency debug */
int get_LowLatencyDebug(void);
void set_LowLatencyDebug(unsigned int bFlag);

#endif
