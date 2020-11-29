/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2008
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/
#include "lcm_drv.h"

#include <linux/string.h>
#include <linux/kernel.h>
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1280)


#define REGFLAG_DELAY             							0XFFE
#define REGFLAG_END_OF_TABLE      							0xFFF   // END OF REGISTERS MARKER
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util ;

#define SET_RESET_PIN(v)                                    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))
// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)									lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)				lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)										lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)      

struct LCM_setting_table 
{
    unsigned int cmd;
    unsigned char count;
    unsigned char para_list[3];
};

static struct LCM_setting_table lcm_initialization_setting[] =
{{0xFE, 1, {0x21}},
{0x00, 1, {0x90}},
{0x01, 1, {0xAB}},
{0x02, 1, {0xCF}},
{0x03, 1, {0xE9}},
{0x04, 1, {0x00}},
{0x05, 1, {0xFE}},
{0x06, 1, {0x10}},
{0x07, 1, {0x20}},
{0x08, 1, {0x2F}},
{0x09, 1, {0x15}},
{0x0A, 1, {0x3C}},
{0x0B, 1, {0x67}},
{0x0C, 1, {0x8B}},
{0x0D, 1, {0xC0}},
{0x0E, 1, {0x55}},
{0x0F, 1, {0xEA}},
{0x10, 1, {0x2A}},
{0x11, 1, {0x5D}},
{0x12, 1, {0x5E}},
{0x13, 1, {0x6A}},
{0x14, 1, {0x8C}},
{0x15, 1, {0xBB}},
{0x16, 1, {0xDD}},
{0x17, 1, {0x09}},
{0x18, 1, {0xAB}},
{0x19, 1, {0x25}},
{0x1A, 1, {0x47}},
{0x1B, 1, {0x50}},
{0x1C, 1, {0x59}},
{0x1D, 1, {0xFF}},
{0x1E, 1, {0x63}},
{0x1F, 1, {0x6B}},
{0x20, 1, {0x72}},
{0x21, 1, {0x77}},
{0x22, 1, {0xFF}},
{0x23, 1, {0x79}},
{0x24, 1, {0x7A}},
{0x25, 1, {0xF0}},
{0x26, 1, {0x90}},
{0x27, 1, {0xAB}},
{0x28, 1, {0xCF}},
{0x29, 1, {0xE9}},
{0x2A, 1, {0x00}},
{0x2B, 1, {0xFE}},
{0x2D, 1, {0x10}},
{0x2F, 1, {0x20}},
{0x30, 1, {0x2F}},
{0x31, 1, {0x15}},
{0x32, 1, {0x3C}},
{0x33, 1, {0x67}},
{0x34, 1, {0x8B}},
{0x35, 1, {0xC0}},
{0x36, 1, {0x55}},
{0x37, 1, {0xEA}},
{0x38, 1, {0x2A}},
{0x39, 1, {0x5D}},
{0x3A, 1, {0x5E}},
{0x3B, 1, {0x6A}},
{0x3D, 1, {0x8C}},
{0x3F, 1, {0xBB}},
{0x40, 1, {0xDD}},
{0x41, 1, {0x09}},
{0x42, 1, {0xAB}},
{0x43, 1, {0x25}},
{0x44, 1, {0x47}},
{0x45, 1, {0x50}},
{0x46, 1, {0x59}},
{0x47, 1, {0xFF}},
{0x48, 1, {0x63}},
{0x49, 1, {0x6B}},
{0x4A, 1, {0x72}},
{0x4B, 1, {0x77}},
{0x4C, 1, {0xFF}},
{0x4D, 1, {0x79}},
{0x4E, 1, {0x7A}},
{0x4F, 1, {0xF0}},
{0x50, 1, {0x90}},
{0x51, 1, {0xAB}},
{0x52, 1, {0xCF}},
{0x53, 1, {0xE9}},
{0x54, 1, {0x00}},
{0x55, 1, {0xFE}},
{0x56, 1, {0x10}},
{0x58, 1, {0x20}},
{0x59, 1, {0x2F}},
{0x5A, 1, {0x15}},
{0x5B, 1, {0x3C}},
{0x5C, 1, {0x67}},
{0x5D, 1, {0x8B}},
{0x5E, 1, {0xC0}},
{0x5F, 1, {0x55}},
{0x60, 1, {0xEA}},
{0x61, 1, {0x2A}},
{0x62, 1, {0x5D}},
{0x63, 1, {0x5E}},
{0x64, 1, {0x6A}},
{0x65, 1, {0x8C}},
{0x66, 1, {0xBB}},
{0x67, 1, {0xDD}},
{0x68, 1, {0x09}},
{0x69, 1, {0xAB}},
{0x6A, 1, {0x25}},
{0x6B, 1, {0x47}},
{0x6C, 1, {0x50}},
{0x6D, 1, {0x59}},
{0x6E, 1, {0xFF}},
{0x6F, 1, {0x63}},
{0x70, 1, {0x6B}},
{0x71, 1, {0x72}},
{0x72, 1, {0x77}},
{0x73, 1, {0xFF}},
{0x74, 1, {0x79}},
{0x75, 1, {0x7A}},
{0x76, 1, {0xF0}},
{0x77, 1, {0x90}},
{0x78, 1, {0xAB}},
{0x79, 1, {0xCF}},
{0x7A, 1, {0xE9}},
{0x7B, 1, {0x00}},
{0x7C, 1, {0xFE}},
{0x7D, 1, {0x10}},
{0x7E, 1, {0x20}},
{0x7F, 1, {0x2F}},
{0x80, 1, {0x15}},
{0x81, 1, {0x3C}},
{0x82, 1, {0x67}},
{0x83, 1, {0x8B}},
{0x84, 1, {0xC0}},
{0x85, 1, {0x55}},
{0x86, 1, {0xEA}},
{0x87, 1, {0x2A}},
{0x88, 1, {0x5D}},
{0x89, 1, {0x5E}},
{0x8A, 1, {0x6A}},
{0x8B, 1, {0x8C}},
{0x8C, 1, {0xBB}},
{0x8D, 1, {0xDD}},
{0x8E, 1, {0x09}},
{0x8F, 1, {0xAB}},
{0x90, 1, {0x25}},
{0x91, 1, {0x47}},
{0x92, 1, {0x50}},
{0x93, 1, {0x59}},
{0x94, 1, {0xFF}},
{0x95, 1, {0x63}},
{0x96, 1, {0x6B}},
{0x97, 1, {0x72}},
{0x98, 1, {0x77}},
{0x99, 1, {0xFF}},
{0x9A, 1, {0x79}},
{0x9B, 1, {0x7A}},
{0x9C, 1, {0xF0}},
{0x9D, 1, {0x90}},
{0x9E, 1, {0xAB}},
{0x9F, 1, {0xCF}},
{0xA0, 1, {0xE9}},
{0xA2, 1, {0x00}},
{0xA3, 1, {0xFE}},
{0xA4, 1, {0x10}},
{0xA5, 1, {0x20}},
{0xA6, 1, {0x2F}},
{0xA7, 1, {0x15}},
{0xA9, 1, {0x3C}},
{0xAA, 1, {0x67}},
{0xAB, 1, {0x8B}},
{0xAC, 1, {0xC0}},
{0xAD, 1, {0x55}},
{0xAE, 1, {0xEA}},
{0xAF, 1, {0x2A}},
{0xB0, 1, {0x5D}},
{0xB1, 1, {0x5E}},
{0xB2, 1, {0x6A}},
{0xB3, 1, {0x8C}},
{0xB4, 1, {0xBB}},
{0xB5, 1, {0xDD}},
{0xB6, 1, {0x09}},
{0xB7, 1, {0xAB}},
{0xB8, 1, {0x25}},
{0xB9, 1, {0x47}},
{0xBA, 1, {0x50}},
{0xBB, 1, {0x59}},
{0xBC, 1, {0xFF}},
{0xBD, 1, {0x63}},
{0xBE, 1, {0x6B}},
{0xBF, 1, {0x72}},
{0xC0, 1, {0x77}},
{0xC1, 1, {0xFF}},
{0xC2, 1, {0x79}},
{0xC3, 1, {0x7A}},
{0xC4, 1, {0xF0}},
{0xC5, 1, {0x90}},
{0xC6, 1, {0xAB}},
{0xC7, 1, {0xCF}},
{0xC8, 1, {0xE9}},
{0xC9, 1, {0x00}},
{0xCA, 1, {0xFE}},
{0xCB, 1, {0x10}},
{0xCC, 1, {0x20}},
{0xCD, 1, {0x2F}},
{0xCE, 1, {0x15}},
{0xCF, 1, {0x3C}},
{0xD0, 1, {0x67}},
{0xD1, 1, {0x8B}},
{0xD2, 1, {0xC0}},
{0xD3, 1, {0x55}},
{0xD4, 1, {0xEA}},
{0xD5, 1, {0x2A}},
{0xD6, 1, {0x5D}},
{0xD7, 1, {0x5E}},
{0xD8, 1, {0x6A}},
{0xD9, 1, {0x8C}},
{0xDA, 1, {0xBB}},
{0xDB, 1, {0xDD}},
{0xDC, 1, {0x09}},
{0xDD, 1, {0xAB}},
{0xDE, 1, {0x25}},
{0xDF, 1, {0x47}},
{0xE0, 1, {0x50}},
{0xE1, 1, {0x59}},
{0xE2, 1, {0xFF}},
{0xE3, 1, {0x63}},
{0xE4, 1, {0x6B}},
{0xE5, 1, {0x72}},
{0xE6, 1, {0x77}},
{0xE7, 1, {0xFF}},
{0xE8, 1, {0x79}},
{0xE9, 1, {0x7A}},
{0xEA, 1, {0xF0}},
{0xFE, 1, {0x3D}},
{0x00, 1, {0x05}},
{0xFE, 1, {0x23}},
{0x7D, 1, {0x20}},
{0x7E, 1, {0x2A}},
{0x7F, 1, {0x20}},
{0x08, 1, {0x82}},
{0x0A, 1, {0x00}},
{0x0B, 1, {0x3A}},
{0x0C, 1, {0x03}},
{0x16, 1, {0x2B}},
{0x17, 1, {0x15}},
{0x1B, 1, {0x04}},
{0x18, 1, {0x03}},
{0x19, 1, {0x04}},
{0x1C, 1, {0x81}},
{0x1E, 1, {0x2B}},
{0x1F, 1, {0x15}},
{0x23, 1, {0x04}},
{0x20, 1, {0x05}},
{0x21, 1, {0x06}},
{0x24, 1, {0xC1}},
{0x2B, 1, {0x01}},
{0x26, 1, {0xB0}},
{0x27, 1, {0x80}},
{0x53, 1, {0x36}},
{0x54, 1, {0x54}},
{0x6F, 1, {0x45}},
{0x6E, 1, {0x63}},
{0x56, 1, {0x77}},
{0x58, 1, {0xCB}},
{0x59, 1, {0xBC}},
{0x6C, 1, {0x77}},
{0x6B, 1, {0xCB}},
{0x6A, 1, {0xBC}},
{0x69, 1, {0x00}},
{0x5A, 1, {0x00}},
{0x5F, 1, {0x22}},
{0x60, 1, {0x11}},
{0x61, 1, {0x00}},
{0x62, 1, {0x00}},
{0x63, 1, {0x11}},
{0x64, 1, {0x22}},
{0xFE, 1, {0x00}},
{0xFE, 1, {0x3D}},
{0x55, 1, {0x78}},
{0xFE, 1, {0x20}},
{0x26, 1, {0x30}},
{0x5E, 1, {0x35}},
{0xFE, 1, {0x3D}},
{0x09, 1, {0x00}},
{0x20, 1, {0x71}},
{0xFE, 1, {0x00}},
{0x35, 1, {0x00}},
{0xBA, 1, {0x03}},

{0x36, 1,{0x03}},
{0x11,0,{0x00}},
{REGFLAG_DELAY, 200, {}},
{0x29,0,{0x00}},
{REGFLAG_DELAY, 120, {}},
    
{REGFLAG_END_OF_TABLE, 0x00, {}}
};


/*
static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
	{0x11, 0, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
	{0x29, 0, {0x00}},
	{REGFLAG_DELAY, 10, {}},
	
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
*/

static struct LCM_setting_table lcm_sleep_in_setting[] = {
	// Display off sequence
	{0x28, 0, {0x00}},

    // Sleep Mode On
	{0x10, 0, {0x00}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};





static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

    for(i = 0; i < count; i++) {
		
        unsigned cmd;
        cmd = table[i].cmd;
		
        switch (cmd) {
			
            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;
				
            case REGFLAG_END_OF_TABLE :
                break;
				
            default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
       	}
    }
	
}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
		memset(params, 0, sizeof(LCM_PARAMS));
	
		params->type   = LCM_TYPE_DSI;

		params->width  = FRAME_WIDTH;
		params->height = FRAME_HEIGHT;


		params->dsi.mode   = BURST_VDO_MODE;
    	params->dsi.LANE_NUM				= LCM_THREE_LANE;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

		params->dsi.vertical_sync_active				= 2;//2;//2;
		params->dsi.vertical_backporch					= 6;//6;//5;//5;//5
		params->dsi.vertical_frontporch					= 14;//14;//10;//10;//13
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

        params->dsi.horizontal_sync_active		 	    = 8;//8;//8;//10;//8
		params->dsi.horizontal_backporch				= 16;//16;//16;//20;//16
		params->dsi.horizontal_frontporch				= 72;//72;//40;//40;//72
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

        params->dsi.PLL_CLOCK=250;
}

static void lcm_init(void)
{
	SET_RESET_PIN(1);
	MDELAY(20);
	SET_RESET_PIN(0);
	MDELAY(50);//Must > 5ms
	SET_RESET_PIN(1);
	MDELAY(150);//Must > 50ms

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
    push_table(lcm_sleep_in_setting, sizeof(lcm_sleep_in_setting) / sizeof(struct LCM_setting_table), 1);
    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(20); // 1ms

    SET_RESET_PIN(1);
    MDELAY(120);
}


static void lcm_resume(void)
{
	lcm_init();
}

LCM_DRIVER rm67200_hd720_dsi_vdo_lcm_drv = 
{
    .name			= "rm67200_hd720_dsi_vdo",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
};

