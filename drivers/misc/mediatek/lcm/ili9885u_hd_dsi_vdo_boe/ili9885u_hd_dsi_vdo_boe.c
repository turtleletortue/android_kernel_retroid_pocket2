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

#ifdef BUILD_LK
#else
#include <linux/string.h>
#include <linux/kernel.h>
#endif
#include "lcm_drv.h"

	#include <mt-plat/mt_gpio.h>



// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1280)


#define REGFLAG_DELAY                                         0xFFE
#define REGFLAG_END_OF_TABLE                                  0xFFF   // END 
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)    


struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};

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

    // enable tearing-free
    params->dbi.te_mode                 = LCM_DBI_TE_MODE_VSYNC_ONLY;
    params->dbi.te_edge_polarity        = LCM_POLARITY_RISING;

    params->dsi.mode   = SYNC_PULSE_VDO_MODE; //SYNC_EVENT_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE;


    // DSI
    /* Command mode setting */
    //1 Three lane or Four lane
    params->dsi.LANE_NUM                = LCM_THREE_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
    params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    // Highly depends on LCD driver capability.
    // Not support in MT6573
    params->dsi.packet_size=256;

    // Video mode setting
    params->dsi.intermediat_buffer_num = 0;//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    params->dsi.word_count=720*3;


    params->dsi.vertical_sync_active                = 4;// 3    2
    params->dsi.vertical_backporch                    = 38;// 20   1
    params->dsi.vertical_frontporch                    = 40; // 1  12
    params->dsi.vertical_active_line                = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active                = 10;// 50  2
    params->dsi.horizontal_backporch                = 72;
    params->dsi.horizontal_frontporch                = 72 ;
    params->dsi.horizontal_active_pixel                = FRAME_WIDTH;

    //params->dsi.LPX=8;

    // Bit rate calculation
    //1 Every lane speed
    //params->dsi.pll_select=1;
    params->dsi.PLL_CLOCK = 233;//240 230
    params->dsi.compatibility_for_nvk = 1;
    
    //yan read device info
    set_global_lcd_name("nt35521_dsi_vdo_hd_auo");
    //yan end
}
static void push_table(struct LCM_setting_table *table, unsigned int count, 
        unsigned char force_update)

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
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, 
                        force_update);

        }
    }

}


static struct LCM_setting_table lcm_initialization_setting[] = {

{0xFF, 3,{0x98,0x85,0x01}},  // Change to Page 1 

// GIP Setting
{0x01, 1,{0x00}},
{0x02, 1,{0x00}},
{0x03, 1,{0x73}},
{0x04, 1,{0x00}},
{0x05, 1,{0x00}},
{0x06, 1,{0x08}},
{0x07, 1,{0x00}},
{0x08, 1,{0x00}},
{0x09, 1,{0x37}},
{0x0a, 1,{0x37}},
{0x0b, 1,{0x00}},
{0x0c, 1,{0x00}},
{0x0d, 1,{0x00}},
{0x0e, 1,{0x00}},
{0x0f, 1,{0x37}},

{0x10, 1,{0x37}}, 
{0x11, 1,{0x00}}, 
{0x12, 1,{0x00}}, 
{0x13, 1,{0x00}}, 
{0x14, 1,{0x00}}, 
{0x15, 1,{0x00}},
{0x16, 1,{0x00}}, 
{0x17, 1,{0x00}}, 
{0x18, 1,{0x00}}, 
{0x19, 1,{0x00}}, 
{0x1a, 1,{0x00}}, 
{0x1b, 1,{0x00}}, 
{0x1c, 1,{0x00}}, 
{0x1d, 1,{0x00}}, 
{0x1e, 1,{0x40}}, 
{0x1f, 1,{0x00}},

{0x20, 1,{0x06}}, 
{0x21, 1,{0x01}}, 
{0x22, 1,{0x00}}, 
{0x23, 1,{0x00}}, 
{0x24, 1,{0x00}}, 
{0x25, 1,{0x00}}, 
{0x26, 1,{0x00}}, 
{0x27, 1,{0x00}}, 
{0x28, 1,{0x33}}, 
{0x29, 1,{0x03}}, 
{0x2a, 1,{0x80}}, 
{0x2b, 1,{0x00}}, 
{0x2c, 1,{0x08}}, 
{0x2d, 1,{0x01}}, 
{0x2e, 1,{0x05}}, 
{0x2f, 1,{0x06}},

{0x30, 1,{0x00}}, 
{0x31, 1,{0x00}}, 
{0x32, 1,{0x30}}, 
{0x33, 1,{0x02}}, 
{0x34, 1,{0x00}}, 
{0x35, 1,{0x0b}}, 
{0x36, 1,{0x00}}, 
{0x37, 1,{0x08}}, 
{0x38, 1,{0x00}}, 
{0x39, 1,{0x0f}}, 
{0x3a, 1,{0x00}}, 
{0x3b, 1,{0x00}},
{0x3c, 1,{0x00}}, 
{0x3d, 1,{0x00}}, 
{0x3e, 1,{0x00}}, 
{0x3f, 1,{0x00}},

{0x40, 1,{0x00}}, 
{0x41, 1,{0x88}}, 
{0x42, 1,{0x00}}, 
{0x43, 1,{0x40}}, 
{0x44, 1,{0x05}}, 
{0x45, 1,{0x00}}, 
{0x46, 1,{0x00}}, 
{0x47, 1,{0x00}}, 
{0x4a, 1,{0x87}}, 
{0x4b, 1,{0x16}}, 
{0x4c, 1,{0x66}}, 
{0x4d, 1,{0x46}}, 
{0x4e, 1,{0xa4}}, 
{0x4f, 1,{0x8a}},
{0x50, 1,{0x08}}, 
{0x51, 1,{0xff}}, 
{0x52, 1,{0xff}}, 
{0x53, 1,{0xff}}, 
{0x54, 1,{0x97}}, 
{0x55, 1,{0x17}}, 
{0x56, 1,{0x76}}, 
{0x57, 1,{0x57}}, 
{0x58, 1,{0xb5}}, 
{0x59, 1,{0x9b}}, 
{0x5a, 1,{0x09}}, 
{0x5b, 1,{0xff}}, 
{0x5c, 1,{0xff}}, 
{0x5d, 1,{0xff}}, 
{0x5e, 1,{0x06}}, 
{0x5f, 1,{0xA7}},
{0x60, 1,{0x08}}, 
{0x61, 1,{0x06}}, 
{0x62, 1,{0x01}}, 
{0x63, 1,{0xA6}}, 
{0x64, 1,{0x56}}, 
{0x65, 1,{0x56}}, 
{0x66, 1,{0x54}}, 
{0x67, 1,{0x54}}, 
{0x68, 1,{0x5a}}, 
{0x69, 1,{0x5a}}, 
{0x6a, 1,{0x58}}, 
{0x6b, 1,{0x58}}, 
{0x6c, 1,{0x00}}, 
{0x6d, 1,{0xff}}, 
{0x6e, 1,{0xff}}, 
{0x6f, 1,{0xff}},
{0x70, 1,{0xff}}, 
{0x71, 1,{0xff}}, 
{0x72, 1,{0xff}}, 
{0x73, 1,{0xa7}}, 
{0x74, 1,{0x09}}, 
{0x75, 1,{0x07}}, 
{0x76, 1,{0x01}}, 
{0x77, 1,{0xa6}}, 
{0x78, 1,{0x57}}, 
{0x79, 1,{0x57}}, 
{0x7a, 1,{0x55}}, 
{0x7b, 1,{0x55}}, 
{0x7c, 1,{0x5b}}, 
{0x7d, 1,{0x5b}}, 
{0x7e, 1,{0x59}}, 
{0x7f, 1,{0x59}},
{0x80, 1,{0x00}}, 
{0x81, 1,{0xff}}, 
{0x82, 1,{0xff}}, 
{0x83, 1,{0xff}}, 
{0x84, 1,{0xff}}, 
{0x85, 1,{0xff}}, 
{0x86, 1,{0xff}},
{0xFF, 3,{0x98,0x85,0x02}},	// Change to Page 2 
{0x4D, 1,{0x80}},
//{0x33, 1,{0xE1}},

//============ Page5 ============//
{0xFF, 3,{0x98,0x85,0x05}},  // Change to Page 5 

{0x57, 1,{0xC0}},
{0x04, 1,{0x6F}},	// Vcom Setting 
{0x30, 1,{0x01}},	// VGL Pump Ratio: Single Mode 
{0x3D, 1,{0x22}},	// VGL Clamp Setting   -12V
{0x3B, 1,{0x18}},       // VGH Clamp Setting   12V
{0x39, 1,{0x3F}},	// VREG1OUT Setting   4.10V
{0x3A, 1,{0x3F}},	// VREG2OUT Setting   -4.10V

{0x63, 1,{0xF6}},

{0xFF, 3,{0x98,0x85,0x06}},	// Change to Page 6 
{0xA3, 1,{0x00}},

//{0xD2, 1,{0x1A}},               //2 LANE
{0xD2, 1,{0x3A}},               //3 LANE
{0xD3, 1,{0x05}},               

{0xFF, 3,{0x98,0x85,0x07}},
{0xE0, 1,{0x05}},  //HD scale

{0xFF, 3,{0x98,0x85,0x08}},  // Change to Page 8

// Positive Gamma Setting 
{0x80, 1,{0x00}},
{0x81, 1,{0x00}}, 
{0x82, 1,{0x00}}, 
{0x83, 1,{0x1C}}, 
{0x84, 1,{0x00}}, 
{0x85, 1,{0x41}}, 
{0x86, 1,{0x00}}, 
{0x87, 1,{0x5D}}, 
{0x88, 1,{0x00}}, 
{0x89, 1,{0x74}}, 
{0x8A, 1,{0x00}}, 
{0x8B, 1,{0x87}}, 
{0x8C, 1,{0x00}}, 
{0x8D, 1,{0x99}}, 
{0x8E, 1,{0x00}}, 
{0x8F, 1,{0xA8}}, 
{0x90, 1,{0x00}}, 
{0x91, 1,{0xB6}}, 
{0x92, 1,{0x00}}, 
{0x93, 1,{0xE6}}, 
{0x94, 1,{0x01}}, 
{0x95, 1,{0x0C}}, 
{0x96, 1,{0x01}}, 
{0x97, 1,{0x48}}, 
{0x98, 1,{0x01}}, 
{0x99, 1,{0x78}}, 
{0x9A, 1,{0x01}}, 
{0x9B, 1,{0xC2}}, 
{0x9C, 1,{0x01}}, 
{0x9D, 1,{0xFE}}, 
{0x9E, 1,{0x02}}, 
{0x9F, 1,{0x00}}, 
{0xA0, 1,{0x02}}, 
{0xA1, 1,{0x35}}, 
{0xA2, 1,{0x02}}, 
{0xA3, 1,{0x70}}, 
{0xA4, 1,{0x02}},
{0xA5, 1,{0x96}}, 
{0xA6, 1,{0x02}}, 
{0xA7, 1,{0xCC}}, 
{0xA8, 1,{0x02}}, 
{0xA9, 1,{0xF2}}, 
{0xAA, 1,{0x03}}, 
{0xAB, 1,{0x24}}, 
{0xAC, 1,{0x03}}, 
{0xAD, 1,{0x34}}, 
{0xAE, 1,{0x03}}, 
{0xAF, 1,{0x45}}, 
{0xB0, 1,{0x03}}, 
{0xB1, 1,{0x5A}}, 
{0xB2, 1,{0x03}}, 
{0xB3, 1,{0x71}}, 
{0xB4, 1,{0x03}}, 
{0xB5, 1,{0x8C}}, 
{0xB6, 1,{0x03}}, 
{0xB7, 1,{0xB0}}, 
{0xB8, 1,{0x03}}, 
{0xB9, 1,{0xDB}}, 
{0xBA, 1,{0x03}}, 
{0xBB, 1,{0xE6}},

{0xFF, 3,{0x98,0x85,0x09}},	// Change to Page 9 

// Positive Gamma Setting 
{0x80, 1,{0x00}},
{0x81, 1,{0x00}}, 
{0x82, 1,{0x00}}, 
{0x83, 1,{0x1C}}, 
{0x84, 1,{0x00}}, 
{0x85, 1,{0x41}}, 
{0x86, 1,{0x00}}, 
{0x87, 1,{0x5D}}, 
{0x88, 1,{0x00}}, 
{0x89, 1,{0x74}}, 
{0x8A, 1,{0x00}}, 
{0x8B, 1,{0x87}}, 
{0x8C, 1,{0x00}}, 
{0x8D, 1,{0x99}}, 
{0x8E, 1,{0x00}}, 
{0x8F, 1,{0xA8}}, 
{0x90, 1,{0x00}}, 
{0x91, 1,{0xB6}}, 
{0x92, 1,{0x00}}, 
{0x93, 1,{0xE6}}, 
{0x94, 1,{0x01}}, 
{0x95, 1,{0x0C}}, 
{0x96, 1,{0x01}}, 
{0x97, 1,{0x48}}, 
{0x98, 1,{0x01}}, 
{0x99, 1,{0x78}}, 
{0x9A, 1,{0x01}}, 
{0x9B, 1,{0xC2}}, 
{0x9C, 1,{0x01}}, 
{0x9D, 1,{0xFE}}, 
{0x9E, 1,{0x02}}, 
{0x9F, 1,{0x00}},
{0xA0, 1,{0x02}}, 
{0xA1, 1,{0x35}}, 
{0xA2, 1,{0x02}}, 
{0xA3, 1,{0x70}}, 
{0xA4, 1,{0x02}}, 
{0xA5, 1,{0x96}}, 
{0xA6, 1,{0x02}}, 
{0xA7, 1,{0xCC}}, 
{0xA8, 1,{0x02}}, 
{0xA9, 1,{0xF2}}, 
{0xAA, 1,{0x03}}, 
{0xAB, 1,{0x24}}, 
{0xAC, 1,{0x03}}, 
{0xAD, 1,{0x34}}, 
{0xAE, 1,{0x03}}, 
{0xAF, 1,{0x45}}, 
{0xB0, 1,{0x03}}, 
{0xB1, 1,{0x5A}}, 
{0xB2, 1,{0x03}}, 
{0xB3, 1,{0x71}}, 
{0xB4, 1,{0x03}}, 
{0xB5, 1,{0x8C}}, 
{0xB6, 1,{0x03}}, 
{0xB7, 1,{0xB0}}, 
{0xB8, 1,{0x03}}, 
{0xB9, 1,{0xDB}}, 
{0xBA, 1,{0x03}}, 
{0xBB, 1,{0xE6}},

//============Page0============//
{0xFF, 3,{0x98,0x85,0x00}},

{0x11, 1,{0x00}},
{REGFLAG_DELAY, 120, {}},
{0xFF, 3,{0x98,0x85,0x05}},
{0x3D, 1,{0x22}},	// VGL Clamp Setting   -12V
{0x3B, 1,{0x18}},       // VGH Clamp Setting   12V
{0x3F, 1,{0x18}},               //VGHO = 12V
{0x40, 1,{0x22}},               //VGLO = -12V

{0xFF, 3,{0x98,0x85,0x00}},
{0x29, 1,{0x00}},
{0x35, 1,{0x00}},
{REGFLAG_DELAY, 20, {}},
{REGFLAG_END_OF_TABLE, 0x00, {}}
};



static void lcd_ldo_init(void)
{
	   mt_set_gpio_mode(78, 0);
     mt_set_gpio_dir(78, GPIO_DIR_OUT);
	   mt_set_gpio_out(78,GPIO_OUT_ONE);
}


static void lcd_ldo_off(void)
{
   mt_set_gpio_out(78,GPIO_OUT_ZERO);
}

static void lcm_init(void)
{
	lcd_ldo_init();
	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);//Must > 5ms
	SET_RESET_PIN(1);
	MDELAY(120);//Must > 50ms

	push_table(lcm_initialization_setting,
		   sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{
		MDELAY(120);
	SET_RESET_PIN(0);
	lcd_ldo_off();
}


static void lcm_resume(void)
{
	lcm_init();
}
#if 0
static unsigned int lcm_ili9885u_id(void)
{
	int array[4];
	unsigned char buffer[5];
	char id_high=0;
	char id_midd=0;
	char id_low=0;
	int id=0;
	//Do reset here
	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(25);
	SET_RESET_PIN(1);
	MDELAY(50);      
   
	lcd_ldo_init();
	MDELAY(20);

	array[0]=0x00043902;
	array[1]=0x068598FF;
	dsi_set_cmdq(array, 3, 1);
	MDELAY(10);
	array[0]=0x00023700;
	dsi_set_cmdq(array, 1, 1);
	//read_reg_v2(0x04, buffer, 3);

	//read_reg_v2(0xF0, buffer,1);
	//id_high = buffer[0]; ///////////////////////0x98
	read_reg_v2(0xF1, buffer,1);
	id_midd = buffer[0]; ///////////////////////0x85
	read_reg_v2(0xF2, buffer,1);
	id_low = buffer[0]; ////////////////////////0x0C
   
	id = (id_midd << 8) | id_low;

#ifdef BUILD_LK
    printf("[erick-lk]%s,  9885U id = 0x%08x\n", __func__, id);
#endif

	if(id == 0x850C)
	{
		return 1;
	}
	
	return 0;
}
#endif 



LCM_DRIVER ili9885u_hd_dsi_vdo_boe_lcm_drv = 
{
    .name			= "ili9885u_hd_dsi_vdo_boe",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
//	.compare_id     = lcm_ili9885u_id,
};

