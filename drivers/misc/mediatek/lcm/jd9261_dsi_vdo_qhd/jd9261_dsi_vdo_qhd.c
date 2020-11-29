
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
#include <linux/string.h>
#include <linux/kernel.h>
#include "lcm_drv.h"

#define REGFLAG_DELAY             								0XFE
#define REGFLAG_END_OF_TABLE      								0xFFF   // END OF REGISTERS MARKER
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define FRAME_WIDTH                                          	(540)
#define FRAME_HEIGHT                                         	(960)
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------
static LCM_UTIL_FUNCS lcm_util ;

#define SET_RESET_PIN(v)                                    	(lcm_util.set_reset_pin((v)))

#define UDELAY(n)                                             	(lcm_util.udelay(n))
#define MDELAY(n)                                             	(lcm_util.mdelay(n))
// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)        	lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                        	lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                    	lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)                                           lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)                   lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

struct LCM_setting_table 
{
    unsigned int cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] =
{
	{0xBF,3,{0x92,0x61,0xF3}},
	{0xB3,2,{0x00,0x78}},
	{0xB4,2,{0x00,0x78}},
	{0xB8,6,{0x00,0xCF,0x01,0x00,0xCF,0x01}},
	{0xC3,1,{0x04}},
	{0xC4,2,{0x00,0x78}},
	{0xC7,9,{0x00,0x02,0x32,0x08,0x68,0x2A,0x12,0xA5,0xA5}},
	{0xC8,38,{0x7F,0x62,0x4E,0x3F,0x38,0x26,0x27,0x0E,0x23,0x1F,
			  0x1D,0x39,0x27,0x35,0x2D,0x35,0x2D,0x21,0x02,0x7F,
			  0x62,0x4E,0x3F,0x38,0x26,0x27,0x0E,0x23,0x1F,0x1D,
			  0x39,0x27,0x35,0x2D,0x35,0x2D,0x21,0x02}},
	
	{0xD4,19,{0x1F,0x1F,0x1E,0x1F,0x00,0x10,0x1F,0x1F,0x04,0x08,
			  0x06,0x0A,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}},
	
	{0xD5,19,{0x1F,0x1F,0x1E,0x1F,0x01,0x11,0x1F,0x1F,0x05,0x09,
			  0x07,0x0B,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}},
	
	{0xD6,19,{0x1F,0x1F,0x1F,0x1E,0x11,0x01,0x1F,0x1F,0x0B,0x07,
			  0x09,0x05,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}},
	
	{0xD7,19,{0x1F,0x1F,0x1F,0x1E,0x10,0x00,0x1F,0x1F,0x0A,0x06,
			  0x08,0x04,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}},
	
	{0xD8,20,{0x20,0x00,0x00,0x10,0x02,0x30,0x00,0x00,0x30,0x00,
			  0x00,0x00,0x00,0x50,0x08,0x73,0x06,0x73,0x73,0x00}},
	
	{0xD9,21,{0x00,0x0A,0x0A,0x88,0x00,0x00,0x06,0x7b,0x00,0x00,
			  0x00,0x3B,0x2F,0x1F,0x00,0x00,0x00,0x03,0x7b,0x01,
			  0xE0}},
	
	{0xBE,1,{0x01}},
	{0xC1,1,{0x10}},
	{0xCC,10,{0x34,0x20,0x38,0x60,0x11,0x91,0x00,0x50,0x00,0x00}},
	{0xBE,1,{0x00}},
	
	//{0x35,1,{0x00}}, // TE ON
	{0x35,0,{}}, // TE ON
	{0x11,0,{}}, // Sleep Out
	{REGFLAG_DELAY,200,{}},
	{0x29,0,{}}, // Display On
	{REGFLAG_DELAY,50,{}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

    for(i = 0; i < count; i++)
    {
    	unsigned cmd;
        cmd = table[i].cmd;
		
        switch (cmd) 
        {
        case REGFLAG_DELAY :
            MDELAY(table[i].count);
            break;
			
        case REGFLAG_END_OF_TABLE :
            break;
			
        default:
			dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
			MDELAY(2);
            break;
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

	// enable tearing-free
    params->dbi.te_mode = LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;

	params->dsi.mode   = SYNC_PULSE_VDO_MODE;

	// DSI
	/* Command mode setting */
    params->dsi.LANE_NUM				= LCM_TWO_LANE;

	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	params->dsi.packet_size = 256;
	params->dsi.intermediat_buffer_num = 2;

	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.word_count=FRAME_WIDTH * 3;	//DSI CMD mode need set these two bellow params, different to 6577
    
	params->dsi.vertical_sync_active = 2;
	params->dsi.vertical_backporch = 12;
	params->dsi.vertical_frontporch = 5;
	params->dsi.vertical_active_line = FRAME_HEIGHT;
	 
	params->dsi.horizontal_sync_active = 10; 
	params->dsi.horizontal_backporch = 10; 
	params->dsi.horizontal_frontporch = 10; 
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;

	params->dsi.PLL_CLOCK = 203;//250;
	params->dsi.ssc_disable = 1;
	params->dsi.ssc_range = 4;
}

static void lcm_init(void)
{
    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(120);

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);

}

static struct LCM_setting_table lcm_sleep_in_setting[] = 
{
	// Display off sequence
	{0x28, 0, {0x00}},
    // Sleep Mode On
	{0x10, 0, {0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void lcm_suspend(void)
{
	push_table(lcm_sleep_in_setting, sizeof(lcm_sleep_in_setting) / sizeof(struct LCM_setting_table), 1);

    SET_RESET_PIN(0);
    MDELAY(20); // 1ms
}

static void lcm_resume(void)
{
    lcm_init();
}

LCM_DRIVER jd9261_dsi_vdo_qhd_lcm_drv =
{
    .name           = "jd9261_dsi_vdo_qhd",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
};