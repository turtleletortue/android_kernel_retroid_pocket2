
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

#ifndef BUILD_LK
#else
#include <linux/string.h>
#include <linux/kernel.h>
#endif

#include "lcm_drv.h"
//yufeng
/*
#ifdef BUILD_LK
    #include <platform/mt_gpio.h>
    #include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include <mach/mt_gpio.h>
#endif
*/

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH                                          (540)
#define FRAME_HEIGHT                                         (960)

#define LCM_ID_ILI9807                                   0x9817  //D3


#ifndef TRUE
    #define   TRUE     1
#endif

#ifndef FALSE
    #define   FALSE    0
#endif

 //unsigned static int lcm_esd_test = FALSE;      ///only for ESD test

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util ;

#define SET_RESET_PIN(v)                                    (lcm_util.set_reset_pin((v)))

#define UDELAY(n)                                             (lcm_util.udelay(n))
#define MDELAY(n)                                             (lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V3(para_tbl,size,force_update)        lcm_util.dsi_set_cmdq_V3(para_tbl,size,force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)            lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)        lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                        lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                    lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)                                            lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)                   lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define   LCM_DSI_CMD_MODE                            0



static LCM_setting_table_V3 lcm_initialization_setting[] = {
{0x39,0xFF,5,{0xFF,0x98,0x07,0x00,0x01}},
	{0x15,0x31,1,{0x00}},
	{0x15,0x40,1,{0x55}},
	{0x15,0x41,1,{0x55}},
	{0x15,0x42,1,{0x02}},
	{0x15,0x43,1,{0x89}},
	{0x15,0x44,1,{0x07}},
	{0x15,0x46,1,{0xEB}},
	{0x15,0x47,1,{0xBC}},
	{0x15,0x50,1,{0xA4}},
	{0x15,0x51,1,{0xA4}},
	{0x15,0x52,1,{0x00}},
	{0x15,0x53,1,{0x0D}},
	{0x15,0x60,1,{0x05}},
	{0x15,0x61,1,{0x08}},
	{0x15,0x62,1,{0x08}},
	{0x15,0x63,1,{0x08}},
	{0x15,0x64,1,{0x88}},
	
	//++++++++++++++++++ Gamma Setting ++++++++++++++++++//
	
	/*{0x39,0xFF,0xFF,0x98,0x06,0x04,0x01,*/
	
	{0x15,0xA0,1,{0x00}},
	{0x15,0xA1,1,{0x0B}},
	{0x15,0xA2,1,{0x25}},
	{0x15,0xA3,1,{0x0E}},
	{0x15,0xA4,1,{0x07}},
	{0x15,0xA5,1,{0x0F}},
	{0x15,0xA6,1,{0x08}},
	{0x15,0xA7,1,{0x05}},
	{0x15,0xA8,1,{0x06}},
	{0x15,0xA9,1,{0x0B}},
	{0x15,0xAA,1,{0x0F}},
	{0x15,0xAB,1,{0x07}},
	{0x15,0xAC,1,{0x0E}},
	{0x15,0xAD,1,{0x13}},
	{0x15,0xAE,1,{0x0E}},
	{0x15,0xAF,1,{0x00}},
	
	///==============Nagitive==============///
	
	{0x15,0xC0,1,{0x0F}},
	{0x15,0xC1,1,{0x1B}},
	{0x15,0xC2,1,{0x25}},
	{0x15,0xC3,1,{0x0E}},
	{0x15,0xC4,1,{0x07}},
	{0x15,0xC5,1,{0x10}},
	{0x15,0xC6,1,{0x08}},
	{0x15,0xC7,1,{0x05}},
	{0x15,0xC8,1,{0x06}},
	{0x15,0xC9,1,{0x0B}},
	{0x15,0xCA,1,{0x0E}},
	{0x15,0xCB,1,{0x06}},
	{0x15,0xCC,1,{0x0E}},
	{0x15,0xCD,1,{0x13}},
	{0x15,0xCE,1,{0x0E}},
	{0x15,0xCF,1,{0x0F}},
	
	//****************************** Page 6 Command 
	
	//******************************//
	
	{0x39,0xFF,5,{0xFF,0x98,0x07,0x00,0x06}},
	{0x15,0x00,1,{0x21}},
	{0x15,0x01,1,{0x06}},
	{0x15,0x02,1,{0x20}},
	{0x15,0x03,1,{0x02}},
	{0x15,0x04,1,{0x00}},
	{0x15,0x05,1,{0x00}},
	{0x15,0x06,1,{0x80}},
	{0x15,0x07,1,{0x03}},
	{0x15,0x08,1,{0x04}},
	{0x15,0x09,1,{0x00}},
	{0x15,0x0A,1,{0x00}},
	{0x15,0x0B,1,{0x00}},
	{0x15,0x0C,1,{0x26}},
	{0x15,0x0D,1,{0x26}},
	{0x15,0x0E,1,{0x00}},
	{0x15,0x0F,1,{0x00}},
	{0x15,0x10,1,{0xFF}},
	{0x15,0x11,1,{0xE0}},
	{0x15,0x12,1,{0x00}},
	{0x15,0x13,1,{0x00}},
	{0x15,0x14,1,{0x00}},
	{0x15,0x15,1,{0xC0}},
	{0x15,0x16,1,{0x08}},
	{0x15,0x17,1,{0x00}},
	{0x15,0x18,1,{0x00}},
	{0x15,0x19,1,{0x00}},
	{0x15,0x1A,1,{0x00}},
	{0x15,0x1B,1,{0x00}},
	{0x15,0x1C,1,{0x00}},
	{0x15,0x1D,1,{0x00}},
	{0x15,0x20,1,{0x01}},
	{0x15,0x21,1,{0x23}},
	{0x15,0x22,1,{0x45}},
	{0x15,0x23,1,{0x67}},
	{0x15,0x24,1,{0x01}},
	{0x15,0x25,1,{0x23}},
	{0x15,0x26,1,{0x45}},
	{0x15,0x27,1,{0x67}},
	{0x15,0x30,1,{0x10}},
	{0x15,0x31,1,{0x87}},
	{0x15,0x32,1,{0x96}},
	{0x15,0x33,1,{0x22}},
	{0x15,0x34,1,{0x22}},
	{0x15,0x35,1,{0xBA}},
	{0x15,0x36,1,{0xAB}},
	{0x15,0x37,1,{0xBA}},
	{0x15,0x38,1,{0xAB}},
	{0x15,0x39,1,{0xDC}},
	{0x15,0x3A,1,{0xCD}},
	{0x15,0x3B,1,{0xDC}},
	{0x15,0x3C,1,{0xCD}},
	{0x15,0x3D,1,{0x78}},
	{0x15,0x3E,1,{0x69}},
	{0x15,0x3F,1,{0x78}},
	{0x15,0x40,1,{0x69}},
	{0x15,0x41,1,{0x22}},
	{0x15,0x42,1,{0x22}},
	{0x15,0x43,1,{0x22}},
	{0x39,0xFF,5,{0xFF,0x98,0x07,0x00,0x00}},
	{0x15,0x35,1,{0x00}},


    {0x05,0x11,0,{}},
    {REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 120, {}},


    {0x05, 0x29,0,{}},
    {REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 10, {}},

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

        #if (LCM_DSI_CMD_MODE)
        params->dsi.mode   = CMD_MODE;
        #else
        params->dsi.mode   = SYNC_PULSE_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE;
        #endif

        // DSI
        /* Command mode setting */
        //1 Three lane or Four lane
        params->dsi.LANE_NUM                = LCM_TWO_LANE;
        //The following defined the fomat for data coming from LCD engine.
        params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

        // Video mode setting
        params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

    params->dsi.vertical_sync_active                =4;
    params->dsi.vertical_backporch                    = 16;
    params->dsi.vertical_frontporch                    = 10;
    params->dsi.vertical_active_line                = FRAME_HEIGHT;


    params->dsi.horizontal_sync_active                = 10;///////////////20 20 4  20  14  6
     params->dsi.horizontal_backporch                = 80;
    params->dsi.horizontal_frontporch                = 80;
    params->dsi.horizontal_active_pixel                = FRAME_WIDTH;

        //params->dsi.LPX=8;

        // Bit rate calculation
        //1 Every lane speed
        //params->dsi.pll_select=1;
        //params->dsi.PLL_CLOCK  = LCM_DSI_6589_PLL_CLOCK_377;
        params->dsi.PLL_CLOCK=234;
        params->dsi.pll_div1=0;        // div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
        params->dsi.pll_div2=0;        // div2=0,1,2,3;div1_real=1,2,4,4
#if (LCM_DSI_CMD_MODE)
        params->dsi.fbk_div =7;
#else
        params->dsi.fbk_div =7;    // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)
#endif
        //params->dsi.compatibility_for_nvk = 1;        // this parameter would be set to 1 if DriverIC is NTK's and when force match DSI clock for NTK's

}


static void lcm_init(void)
{
    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(120);

        dsi_set_cmdq_V3(lcm_initialization_setting,sizeof(lcm_initialization_setting)/sizeof(lcm_initialization_setting[0]),1);

}


static LCM_setting_table_V3  lcm_deep_sleep_mode_in_setting[] = {
    // Display off sequence
    {0x05, 0x28, 0, {}},
    {REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 10, {}},

    // Sleep Mode On
    {0x05, 0x10, 0, {}},
    {REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 120, {}},
};
static void lcm_suspend(void)
{

    dsi_set_cmdq_V3(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting)/sizeof(lcm_deep_sleep_mode_in_setting[0]), 1);
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

#if (LCM_DSI_CMD_MODE)
static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
    unsigned int x0 = x;
    unsigned int y0 = y;
    unsigned int x1 = x0 + width - 1;
    unsigned int y1 = y0 + height - 1;

    unsigned char x0_MSB = ((x0>>8)&0xFF);
    unsigned char x0_LSB = (x0&0xFF);
    unsigned char x1_MSB = ((x1>>8)&0xFF);
    unsigned char x1_LSB = (x1&0xFF);
    unsigned char y0_MSB = ((y0>>8)&0xFF);
    unsigned char y0_LSB = (y0&0xFF);
    unsigned char y1_MSB = ((y1>>8)&0xFF);
    unsigned char y1_LSB = (y1&0xFF);

    unsigned int data_array[16];

    data_array[0]= 0x00053902;
    data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
    data_array[2]= (x1_LSB);
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0]= 0x00053902;
    data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
    data_array[2]= (y1_LSB);
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0]= 0x002c3909;
    dsi_set_cmdq(data_array, 1, 0);

}
#endif




static unsigned int lcm_compare_id(void)
{
    int array[4];
    char buffer[4]={0,0,0,0};
    char id_high=0;
    char id_low=0;
    int id=0;

    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(200);


    array[0]=0x00043902;
    array[1]=0x000798ff;
	array[2]=0x00000001;
    dsi_set_cmdq(array, 3, 1);

    MDELAY(10);
    array[0] = 0x00083700;
    dsi_set_cmdq(array, 1, 1);

    MDELAY(10);
    read_reg_v2(0xD3, buffer, 4);//    NC 0x00  0x98 0x16

    id_high = buffer[1];
    id_low = buffer[2];
    id = (id_high<<8) | id_low;

    #ifdef BUILD_LK

        printf("ILI9806 uboot %s \n", __func__);
        printf("%s id = 0x%08x \n", __func__, id);

    #else
        printk("ILI9806 kernel %s \n", __func__);
        printk("%s id = 0x%08x \n", __func__, id);

    #endif


    return (LCM_ID_ILI9807 == id)?1:0;

}

#if 0
static unsigned int lcm_esd_check(void)
{
    #ifdef BUILD_LK
        //printf("lcm_esd_check()\n");
    #else
        //printk("lcm_esd_check()\n");
    #endif
 #ifndef BUILD_LK
    char  buffer[3];
    int   array[4];

    if(lcm_esd_test)
    {
        lcm_esd_test = FALSE;
        return TRUE;
    }

    array[0] = 0x00013700;
    dsi_set_cmdq(array, 1, 1);

    read_reg_v2(0x0a, buffer, 1);
    if(buffer[0]==0x9c)
    {
        //#ifdef BUILD_LK
        //printf("%s %d\n FALSE", __func__, __LINE__);
        //#else
        //printk("%s %d\n FALSE", __func__, __LINE__);
        //#endif
        return FALSE;
    }
    else
    {
        //#ifdef BUILD_LK
        //printf("%s %d\n FALSE", __func__, __LINE__);
        //#else
        //printk("%s %d\n FALSE", __func__, __LINE__);
        //#endif
        return TRUE;
    }
 #endif

}

static unsigned int lcm_esd_recover(void)
{

    #ifdef BUILD_LK
        printf("lcm_esd_recover()\n");
    #else
        printk("lcm_esd_recover()\n");
    #endif

    lcm_init();

    return TRUE;
}
#endif
LCM_DRIVER ili9807_dsi_vdo_qhd_lcm_drv =
{
    .name            = "ili9807_dsi_vdo_qhd",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id    =  lcm_compare_id,
//    .esd_check = lcm_esd_check,
//    .esd_recover = lcm_esd_recover,
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif	
};

