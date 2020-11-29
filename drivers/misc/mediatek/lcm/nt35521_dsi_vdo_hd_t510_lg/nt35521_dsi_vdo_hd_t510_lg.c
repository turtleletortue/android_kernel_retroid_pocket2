
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
//yufeng

//leung cover here
/*
#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include <mach/mt_gpio.h>
#endif
*/
//cover end

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH                                          (720)
#define FRAME_HEIGHT                                         (1280)

#define REGFLAG_DELAY                                         0XFEE
#define REGFLAG_END_OF_TABLE                                  0xFFE   // END 

//leung add
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif
//add end

#define LCM_ID_NT35521  0x5521
#define LCM_DSI_CMD_MODE                                    0

//HQ_fujin 131104
//static unsigned int lcm_esd_test = FALSE;      ///only for ESD test
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)

#define dsi_set_cmdq(pdata, queue_size, force_update)        lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)

#define wrtie_cmd(cmd)                                    lcm_util.dsi_write_cmd(cmd)

#define write_regs(addr, pdata, byte_nums)                lcm_util.dsi_write_regs(addr, pdata, byte_nums)

#define read_reg                                            lcm_util.dsi_read_reg()

#define read_reg_v2(cmd, buffer, buffer_size)                   lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size) 


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
    params->dbi.te_mode                 = LCM_DBI_TE_MODE_DISABLED;
    //params->dbi.te_edge_polarity        = LCM_POLARITY_RISING;

    params->dsi.mode   = SYNC_PULSE_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE;


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
    params->dsi.vertical_backporch                    = 20;// 20   1
    params->dsi.vertical_frontporch                    = 20; // 1  12
    params->dsi.vertical_active_line                = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active                = 10;// 50  2
    params->dsi.horizontal_backporch                = 60;
    params->dsi.horizontal_frontporch                = 60 ;
    params->dsi.horizontal_active_pixel                = FRAME_WIDTH;

    //params->dsi.LPX=8;

    // Bit rate calculation
    //1 Every lane speed
    //params->dsi.pll_select=1;
    params->dsi.PLL_CLOCK = 260;//240 230
    params->dsi.compatibility_for_nvk = 1;
    
    //yan read device info
    set_global_lcd_name("nt35521_dsi_vdo_hd_t510_lg");
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

/*
 // leung cover here
static void lcm_initialization(void)
{
    unsigned int data_array[16];


    data_array[0] = 0x00063902;
    data_array[1] = 0x52AA55F0;
    data_array[2] = 0x00000008;                 
    dsi_set_cmdq(data_array, 3, 1);

    MDELAY(2);
    data_array[0] = 0x00033902;
    data_array[1] = 0x002168B1;                
    dsi_set_cmdq(data_array, 2, 1);
    MDELAY(2);
    data_array[0] = 0x00023902;
    data_array[1] = 0x0000C8B5;                
    dsi_set_cmdq(data_array, 2, 1);
    MDELAY(2);
    data_array[0] = 0x00023902;
    data_array[1] = 0x00000FB6;                
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00053902;                                    
    data_array[1] = 0x0A0000B8;
    data_array[2] = 0x00000000;                
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00023902;                                  
    data_array[1] = 0x000000B9;            
    dsi_set_cmdq(data_array, 2, 1);    

    data_array[0] = 0x00023902;              
    data_array[1] = 0x000002BA;                
    dsi_set_cmdq(data_array, 2, 1);      

    data_array[0] = 0x00033902;
    data_array[1] = 0x006363BB;                
    dsi_set_cmdq(data_array, 2, 1);  

    data_array[0] = 0x00033902;
    data_array[1] = 0x000000BC;                
    dsi_set_cmdq(data_array, 2, 1); 

    data_array[0] = 0x00063902;
    data_array[1] = 0x0D7F02BD;
    data_array[2] = 0x0000000B;                 
    dsi_set_cmdq(data_array, 3, 1); 

    data_array[0] = 0x00113902;
    data_array[1] = 0x873641CC;
    data_array[2] = 0x10654654;
    data_array[3] = 0x12101412;
    data_array[4] = 0x15084014;
    data_array[5] = 0x00000005;                 
    dsi_set_cmdq(data_array, 6, 1);

    data_array[0] = 0x00023902;
    data_array[1] = 0x000000D0;                  
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00113902;
    data_array[1] = 0x080400D1;
    data_array[2] = 0x1814100C;
    data_array[3] = 0x2824201C;
    data_array[4] = 0x3834302C;
    data_array[5] = 0x0000003C;                 
    dsi_set_cmdq(data_array, 6, 1); 

    data_array[0] = 0x00023902;
    data_array[1] = 0x000000D3;                
    dsi_set_cmdq(data_array, 2, 1); 

    data_array[0] = 0x00033902;
    data_array[1] = 0x004444D6;                
    dsi_set_cmdq(data_array, 2, 1); 


    data_array[0] = 0x000D3902;
    data_array[1] = 0x000000D7;
    data_array[2] = 0x00000000;
    data_array[3] = 0x00000000;
    data_array[4] = 0x00000000;                 
    dsi_set_cmdq(data_array, 5, 1);   

    data_array[0] = 0x000E3902;
    data_array[1] = 0x000000D8;
    data_array[2] = 0x00000000;
    data_array[3] = 0x00000000;
    data_array[4] = 0x00000000;                  
    dsi_set_cmdq(data_array, 5, 1);   

    data_array[0] = 0x00033902;
    data_array[1] = 0x000603D9; //               
    dsi_set_cmdq(data_array, 2, 1); 

    data_array[0] = 0x00033902;
    data_array[1] = 0x00FF00E5;                  
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00053902;
    data_array[1] = 0xE7ECF3E6;
    data_array[2] = 0x000000DF;                
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x000B3902; 
    data_array[1] = 0xCCD9F3E7; 
    data_array[2] = 0x99A6B3CD; 
    data_array[3] = 0x00959999;                
    dsi_set_cmdq(data_array, 4, 1);

    data_array[0] = 0x000B3902; 
    data_array[1] = 0xCCD9F3E8; 
    data_array[2] = 0x99A6B3CD; 
    data_array[3] = 0x00959999;                
    dsi_set_cmdq(data_array, 4, 1);

    data_array[0] = 0x00033902; 
    data_array[1] = 0x000400E9;                
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902; 
    data_array[1] = 0x000000EA;                
    dsi_set_cmdq(data_array, 2, 1);


    data_array[0] = 0x00053902;     
    data_array[1] = 0x007887EE; 
    data_array[2] = 0x00000000;                
    dsi_set_cmdq(data_array, 3, 1); 

    data_array[0] = 0x00033902;
    data_array[1] = 0x00FF07EF;        
    dsi_set_cmdq(data_array, 2, 1);  

    data_array[0] = 0x00063902;
    data_array[1] = 0x52AA55F0; 
    data_array[2] = 0x00000108;                  
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00033902;                  
    data_array[1] = 0x000D0DB0;                  
    dsi_set_cmdq(data_array, 2, 1);              
    data_array[0] = 0x00033902;                  
    data_array[1] = 0x000D0DB1;                  
    dsi_set_cmdq(data_array, 2, 1);              
    data_array[0] = 0x00033902;                  
    data_array[1] = 0x002D2DB3;    
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00033902;    
    data_array[1] = 0x001919B4;    
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00033902;       
    data_array[1] = 0x000404B5; //           
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00033902;            
    data_array[1] = 0x000505B6;            
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;            
    data_array[1] = 0x000505B7;            
    dsi_set_cmdq(data_array, 2, 1);       
    data_array[0] = 0x00033902;            
    data_array[1] = 0x000505B8;       
    dsi_set_cmdq(data_array, 2, 1);  
    data_array[0] = 0x00033902;       
    data_array[1] = 0x003333B9; //      
    dsi_set_cmdq(data_array, 2, 1);  
    data_array[0] = 0x00033902;       
    data_array[1] = 0x001616BA; //      
    dsi_set_cmdq(data_array, 2, 1);  
    data_array[0] = 0x00033902;       
    data_array[1] = 0x00006eBC;    
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00033902;
    data_array[1] = 0x00006eBD;    
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00023902;
    data_array[1] = 0x00002aBE; //    vcom 20  26
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00023902;    
    data_array[1] = 0x000021BF; //

    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00023902;    
    data_array[1] = 0x000004C0; //   
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00023902;    
    data_array[1] = 0x000000C1;    
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00033902;    
    data_array[1] = 0x001919C2;    
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00033902;
    data_array[1] = 0x000A0AC3;    
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00033902;           
    data_array[1] = 0x002323C4;           
    dsi_set_cmdq(data_array, 2, 1);      
    data_array[0] = 0x00043902;           
    data_array[1] = 0x008000C7;           
    dsi_set_cmdq(data_array, 2, 1);      
    data_array[0] = 0x00073902;           
    data_array[1] = 0x000000C9;           
    data_array[2] = 0x00000000;           
    dsi_set_cmdq(data_array, 3, 1);      
    data_array[0] = 0x00023902;           
    data_array[1] = 0x000001CA;           
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;           
    data_array[1] = 0x00530BCB;            
    dsi_set_cmdq(data_array, 2, 1);    
    data_array[0] = 0x00023902;
    data_array[1] = 0x000000CC;    
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00043902;           
    data_array[1] = 0x53520BCD;           
    dsi_set_cmdq(data_array, 2, 1);      
    data_array[0] = 0x00023902;           
    data_array[1] = 0x000044CE;           
    dsi_set_cmdq(data_array, 2, 1);      
    data_array[0] = 0x00043902;           
    data_array[1] = 0x505000CF;           
    dsi_set_cmdq(data_array, 2, 1);      
    data_array[0] = 0x00033902;           
    data_array[1] = 0x005050D0;           
    dsi_set_cmdq(data_array, 2, 1);      
    data_array[0] = 0x00033902;           
    data_array[1] = 0x005050D1;         
    dsi_set_cmdq(data_array, 2, 1);    
    data_array[0] = 0x00023902;
    data_array[1] = 0x000037D2; //         
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;
    data_array[1] = 0x000039D3;          
    dsi_set_cmdq(data_array, 2, 1);    

    data_array[0] = 0x00063902;          
    data_array[1] = 0x52AA55F0;           
    data_array[2] = 0x00000208;          
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00023902;                            
    data_array[1] = 0x000001EE;          
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00113902;          
    data_array[1] = 0x00c700B0; //          
    data_array[2] = 0x01f600d9;           
    data_array[3] = 0x011d010b;           
    data_array[4] = 0x0153013a;           
    data_array[5] = 0x0000007b;        
    dsi_set_cmdq(data_array, 6, 1);

    data_array[0] = 0x00113902;          
    data_array[1] = 0x019c01B1;//           
    data_array[2] = 0x02f701d0;           
    data_array[3] = 0x026a0236;           
    data_array[4] = 0x029a026b;           
    data_array[5] = 0x000000cb;        
    dsi_set_cmdq(data_array, 6, 1); 

    data_array[0] = 0x00113902;          
    data_array[1] = 0x03e902B2; //          
    data_array[2] = 0x03310313;           
    data_array[3] = 0x03700358;           
    data_array[4] = 0x03a60391;           
    data_array[5] = 0x000000c2;        
    dsi_set_cmdq(data_array, 6, 1);   

    data_array[0] = 0x00053902;          
    data_array[1] = 0x03ee03B3; //         
    data_array[2] = 0x000000ff;           
    dsi_set_cmdq(data_array, 3, 1);


    data_array[0] = 0x00063902;          
    data_array[1] = 0x52AA55F0;           
    data_array[2] = 0x00000308;          
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00033902;           
    data_array[1] = 0x000000B0;           
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;//06
    data_array[1] = 0x000000B1;    
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00063902;          
    data_array[1] = 0x000003B2;           
    data_array[2] = 0x00000000;          
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00063902;          
    data_array[1] = 0x000003B3;           
    data_array[2] = 0x00000000;          
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00063902;          
    data_array[1] = 0x000003B4;           
    data_array[2] = 0x00000000;          
    dsi_set_cmdq(data_array, 3, 1);   

    data_array[0] = 0x00063902;          
    data_array[1] = 0x000003B5;           
    data_array[2] = 0x00000000;          
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00063902;          
    data_array[1] = 0x000003B6;           
    data_array[2] = 0x00000000;          
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00063902;          
    data_array[1] = 0x000003B7;           
    data_array[2] = 0x00000000;          
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00063902;          
    data_array[1] = 0x000003B8;           
    data_array[2] = 0x00000000;          
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00063902;          
    data_array[1] = 0x000003B9;           
    data_array[2] = 0x00000000;          
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00063902;          
    data_array[1] = 0x001035BA;           
    data_array[2] = 0x00000000;          
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00063902;          
    data_array[1] = 0x001035BB;           
    data_array[2] = 0x00000000;          
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00063902;          
    data_array[1] = 0x001035BC;           
    data_array[2] = 0x00000000;          
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00063902;          
    data_array[1] = 0x001035BD;           
    data_array[2] = 0x00000000;          
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00053902;          
    data_array[1] = 0x003400C0;           
    data_array[2] = 0x00000000;          
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00053902;          
    data_array[1] = 0x003400C1;           
    data_array[2] = 0x00000000;          
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00053902;          
    data_array[1] = 0x003400C2;           
    data_array[2] = 0x00000000;          
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00053902;          
    data_array[1] = 0x003400C3;           
    data_array[2] = 0x00000000;          
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00023902;    
    data_array[1] = 0x000040C4;    
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;    
    data_array[1] = 0x000040C5;    
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;    
    data_array[1] = 0x000040C6;    
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;    
    data_array[1] = 0x000040C7;    
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;    
    data_array[1] = 0x000000EF;        
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00063902;          
    data_array[1] = 0x52AA55F0;           
    data_array[2] = 0x00000508;          
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00033902;           
    data_array[1] = 0x00101BB0;           
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;           
    data_array[1] = 0x00101BB1;           
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;           
    data_array[1] = 0x00101BB2;           
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;           
    data_array[1] = 0x00101BB3;           
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;           
    data_array[1] = 0x00101BB4;           
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;           
    data_array[1] = 0x00101BB5;           
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;           
    data_array[1] = 0x00101BB6;           
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;           
    data_array[1] = 0x00101BB7;           
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;            
    data_array[1] = 0x000000B8;            
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;            
    data_array[1] = 0x000000B9;            
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;            
    data_array[1] = 0x000000BA;            
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;            
    data_array[1] = 0x000000BB;            
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;            
    data_array[1] = 0x000000BC;            
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00063902;            
    data_array[1] = 0x030303BD;            
    data_array[2] = 0x00000100;            
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00023902;//0C        
    data_array[1] = 0x000003C0;            
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902; //0D       
    data_array[1] = 0x000005C1;            
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;            
    data_array[1] = 0x000003C2;            
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;            
    data_array[1] = 0x000005C3;            
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;            
    data_array[1] = 0x000080C4;            
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;               
    data_array[1] = 0x0000A2C5;               
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;//12           
    data_array[1] = 0x000080C6;               
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;                             
    data_array[1] = 0x0000A2C7;            

    dsi_set_cmdq(data_array, 2, 1);                    
    data_array[0] = 0x00033902;                             
    data_array[1] = 0x002001C8;            

    dsi_set_cmdq(data_array, 2, 1);                           
    data_array[0] = 0x00033902;                            
    data_array[1] = 0x002000C9;            
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;                            
    data_array[1] = 0x000001CA;            
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;                            
    data_array[1] = 0x000000CB;            
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00043902;//18                        
    data_array[1] = 0x010000CC;            
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00043902;//18                        
    data_array[1] = 0x010000CD;            
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00043902;//18                        
    data_array[1] = 0x010000CE;            
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00043902;//18                        
    data_array[1] = 0x010000CF;            
    dsi_set_cmdq(data_array, 2, 1);        

    data_array[0] = 0x00023902; ////
    data_array[1] = 0x000000D0;    
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00063902;    
    data_array[1] = 0x000003D1;    
    data_array[2] = 0x00001007;    
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00063902;    
    data_array[1] = 0x000013D2;    
    data_array[2] = 0x00001107;    
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00063902;    
    data_array[1] = 0x000023D3;    
    data_array[2] = 0x00001007;    
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00063902;    
    data_array[1] = 0x000033D4;    
    data_array[2] = 0x00001107;    
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00023902;    
    data_array[1] = 0x000006E5;    
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;    
    data_array[1] = 0x000006E6;    
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;    
    data_array[1] = 0x000006E7;    
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;    
    data_array[1] = 0x000006E8;    
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;    
    data_array[1] = 0x000006E9;    
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;    
    data_array[1] = 0x000006EA;    
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;    
    data_array[1] = 0x000006EB;    
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;    
    data_array[1] = 0x000006EC;    
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;    
    data_array[1] = 0x000031ED;    
    dsi_set_cmdq(data_array, 2, 1);  

    data_array[0] = 0x00063902;          
    data_array[1] = 0x52AA55F0;           
    data_array[2] = 0x00000608;          
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x001110B0;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x001312B1;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x000008B2;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x002D2DB3;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x00342DB4;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x002D34B5;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x00342DB6;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x003434B7;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x000A02B8;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x000800B9;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x000109BA;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x00030BBB;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x003434BC;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x002D34BD;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x00342DBE;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x002D34BF;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x002D2DC0;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x000901C1;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x001819C2;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x001617C3;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x001819C4;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x001617C5;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x000901C6;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x002D2DC7;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x00342DC8;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x002D34C9;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x00342DCA;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x003434CB;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x00030BCC;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x000109CD;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x000800CE;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x000A02CF;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x003434D0;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x002D34D1;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x00342DD2;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x002D34D3;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x002D2DD4;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x000008D5;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x001110D6;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x001312D7;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00063902;          
    data_array[1] = 0x555555D8;          
    data_array[2] = 0x00005555;                      
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00063902;          
    data_array[1] = 0x555555D9;          
    data_array[2] = 0x00005555;                      
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x003434E5;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00033902;          
    data_array[1] = 0x003434E6;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;          
    data_array[1] = 0x000005E7;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;          
    data_array[1] = 0x00000035;                      
    dsi_set_cmdq(data_array, 2, 1);


    data_array[0] = 0x00110500;               
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);
    data_array[0] = 0x00290500;               
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(60);

    data_array[0] = 0x00023902;          
    data_array[1] = 0x00002c53;                      
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00023902;          
    data_array[1] = 0x0000ff51;                      
    dsi_set_cmdq(data_array, 2, 1);
}
//leung cover end
*/
static struct LCM_setting_table lcm_initialization_setting[] = {

#if 1
//LG4.99_SH03+NT35521S_3Gamma2.20160623-rixin

{0xF0,5,{0x55,0xAA,0x52,0x08,0x00}},
{0xFF,4,{0xAA,0x55,0xA5,0x80}},
{0x6F,2,{0x11,0x00}},
{0xF7,2,{0x20,0x00}},
{0x6F,1,{0x01}},
{0xB1,1,{0x21}},
{0xC8,1,{0x80}},
{0xBD,5,{0x01,0xA0,0x10,0x08,0x01}},
{0xB8,4,{0x01,0x02,0x0C,0x02}},
{0xBB,2,{0x11,0x11}},
{0xBC,2,{0x00,0x00}},
{0xB6,1,{0x02}},
{0xB9,2,{0x13,0x13}},

{0xF0,5,{0x55,0xAA,0x52,0x08,0x01}},
{0xB0,2,{0x09,0x09}},
{0xB1,2,{0x09,0x09}},
{0xBC,2,{0xBC,0x00}},
{0xBD,2,{0xBC,0x00}},
{0xCA,1,{0x00}},
{0xC0,1,{0x0C}},
{0xB5,2,{0x04,0x04}},
//{0xBE,1,{0x00}},//vcom 38 ;+-5/per//38
{0xB3,2,{0x21,0x21}},
{0xB4,2,{0x08,0x08}},

{0xB9,2,{0x24,0x24}},//25
{0xBA,2,{0x36,0x36}},

{0xF0,5,{0x55,0xAA,0x52,0x08,0x02}},
{0xEE,1,{0x01}},
	//R(+) MCR cmd
{0xB0,16,{0x00,0x4B,0x00,0x8B,0x00,0xBC,0x00,0xDA,0x00,0xF2,0x01,0x17,0x01,0x34,0x01,0x65}},
{0xB1,16,{0x01,0x88,0x01,0xC0,0x01,0xEB,0x02,0x2F,0x02,0x65,0x02,0x67,0x02,0x9A,0x02,0xD5}},
{0xB2,16,{0x02,0xFC,0x03,0x33,0x03,0x57,0x03,0x85,0x03,0xA1,0x03,0xC4,0x03,0xD8,0x03,0xEF}},
{0xB3,4,{0x03,0xFE,0x03,0xFF}},
	//G(+) MCR cmd
{0xB4,16,{0x00,0x4B,0x00,0x8B,0x00,0xBC,0x00,0xDA,0x00,0xF2,0x01,0x17,0x01,0x34,0x01,0x65}},
{0xB5,16,{0x01,0x88,0x01,0xC0,0x01,0xEB,0x02,0x2F,0x02,0x65,0x02,0x67,0x02,0x9A,0x02,0xD5}},
{0xB6,16,{0x02,0xFC,0x03,0x33,0x03,0x57,0x03,0x85,0x03,0xA1,0x03,0xC4,0x03,0xD8,0x03,0xEF}},
{0xB7,4,{0x03,0xFE,0x03,0xFF}},
	//B(+) MCR cmd
{0xB8,16,{0x00,0x4B,0x00,0x8B,0x00,0xBC,0x00,0xDA,0x00,0xF2,0x01,0x17,0x01,0x34,0x01,0x65}},
{0xB9,16,{0x01,0x88,0x01,0xC0,0x01,0xEB,0x02,0x2F,0x02,0x65,0x02,0x67,0x02,0x9A,0x02,0xD5}},
{0xBA,16,{0x02,0xFC,0x03,0x33,0x03,0x57,0x03,0x85,0x03,0xA1,0x03,0xC4,0x03,0xD8,0x03,0xEF}},
{0xBB,4,{0x03,0xFE,0x03,0xFF}},
	//R(-) MCR cmd
{0xBC,16,{0x00,0x4B,0x00,0x8B,0x00,0xBC,0x00,0xDA,0x00,0xF2,0x01,0x17,0x01,0x34,0x01,0x65}},
{0xBD,16,{0x01,0x88,0x01,0xC0,0x01,0xEB,0x02,0x2F,0x02,0x65,0x02,0x67,0x02,0x9A,0x02,0xD5}},
{0xBE,16,{0x02,0xFC,0x03,0x33,0x03,0x57,0x03,0x85,0x03,0xA1,0x03,0xC4,0x03,0xD8,0x03,0xEF}},
{0xBF,4,{0x03,0xFE,0x03,0xFF}},
	//G(-) MCR cmd
{0xC0,16,{0x00,0x4B,0x00,0x8B,0x00,0xBC,0x00,0xDA,0x00,0xF2,0x01,0x17,0x01,0x34,0x01,0x65}},
{0xC1,16,{0x01,0x88,0x01,0xC0,0x01,0xEB,0x02,0x2F,0x02,0x65,0x02,0x67,0x02,0x9A,0x02,0xD5}},
{0xC2,16,{0x02,0xFC,0x03,0x33,0x03,0x57,0x03,0x85,0x03,0xA1,0x03,0xC4,0x03,0xD8,0x03,0xEF}},
{0xC3,4,{0x03,0xFE,0x03,0xFF}},
	//B(-) MCR cmd ---------------------------------------------------
{0xC4,16,{0x00,0x4B,0x00,0x8B,0x00,0xBC,0x00,0xDA,0x00,0xF2,0x01,0x17,0x01,0x34,0x01,0x65}},
{0xC5,16,{0x01,0x88,0x01,0xC0,0x01,0xEB,0x02,0x2F,0x02,0x65,0x02,0x67,0x02,0x9A,0x02,0xD5}},
{0xC6,16,{0x02,0xFC,0x03,0x33,0x03,0x57,0x03,0x85,0x03,0xA1,0x03,0xC4,0x03,0xD8,0x03,0xEF}},
{0xC7,4,{0x03,0xFE,0x03,0xFF}},

{0x6F,1,{0x02}},
{0xF7,1,{0x47}},
{0x6F,1,{0x0A}},
{0xF7,1,{0x02}},
{0x6F,1,{0x17}},
{0xF4,1,{0x60}},
	//#for X499 BW
{0xF0,5,{0x55,0xAA,0x52,0x08,0x03}},
{0xB0,2,{0x20,0x00}},//22
{0xB1,2,{0x20,0x00}},//22
{0xB2,5,{0x15,0x00,0x60,0x00,0x00}},
{0xB3,5,{0x15,0x00,0x60,0x00,0x00}},
{0xB4,5,{0x05,0x00,0x60,0x00,0x00}},
{0xB5,5,{0x05,0x00,0x60,0x00,0x00}},
{0xBA,5,{0x44,0x10,0x60,0x01,0x90}},
{0xBB,5,{0x44,0x10,0x60,0x01,0x90}},
{0xBC,5,{0x44,0x10,0x60,0x01,0x90}},
{0xBD,5,{0x44,0x10,0x60,0x01,0x90}},
{0xC0,4,{0x00,0x34,0x00,0x00}},
{0xC1,4,{0x00,0x00,0x34,0x00}},
{0xC2,4,{0x00,0x00,0x34,0x00}},
{0xC3,4,{0x00,0x00,0x34,0x00}},
{0xC4,1,{0x60}},
{0xC5,1,{0xC0}},
{0xC6,1,{0x00}},
{0xC7,1,{0x00}},

{0xF0,5,{0x55,0xAA,0x52,0x08,0x05}},
{0xB0,2,{0x17,0x06}},
{0xB1,2,{0x17,0x06}},
{0xB2,2,{0x17,0x06}},
{0xB3,2,{0x17,0x06}},
{0xB4,2,{0x17,0x06}},
{0xB5,2,{0x17,0x06}},
{0xB6,2,{0x14,0x03}},
{0xB7,2,{0x00,0x00}},
{0xB8,1,{0x0c}},
{0xB9,2,{0x00,0x03}},
{0xBA,2,{0x00,0x01}},
{0xBB,2,{0x0a,0x03}},
{0xBC,2,{0x02,0x03}},
{0xBD,5,{0x03,0x03,0x01,0x03,0x03}},
{0xC0,1,{0x07}},
{0xC1,1,{0x06}},
{0xC2,1,{0xA6}},
{0xC3,1,{0x05}},
{0xC4,1,{0xA6}},
{0xC5,1,{0xA6}},
{0xC6,1,{0xA6}},
{0xC7,1,{0xA6}},
{0xC8,2,{0x05,0x20}},
{0xC9,2,{0x04,0x20}},
{0xCA,2,{0x01,0x25}},
{0xCB,2,{0x01,0x60}},
{0xCC,3,{0x00,0x00,0x01}},
{0xCD,3,{0x00,0x00,0x01}},
{0xCE,3,{0x00,0x00,0x02}},
{0xCF,3,{0x00,0x00,0x02}},
{0xD0,7,{0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
{0xD1,5,{0x00,0x35,0x01,0x07,0x10}},
{0xD2,5,{0x10,0x35,0x02,0x03,0x10}},
{0xD3,5,{0x20,0x00,0x43,0x07,0x10}},
{0xD4,5,{0x30,0x00,0x43,0x07,0x10}},
{0xD5,6,{0x00,0x00,0x00,0x00,0x00,0x00}},
{0x6F,1,{0x06}},
{0xD5,5,{0x00,0x00,0x00,0x00,0x00}},
{0xD6,6,{0x00,0x00,0x00,0x00,0x00,0x00}},
{0x6F,1,{0x06}},
{0xD6,5,{0x00,0x00,0x00,0x00,0x00}},
{0xD7,6,{0x00,0x00,0x00,0x00,0x00,0x00}},
{0x6F,1,{0x06}},
{0xD7,5,{0x00,0x00,0x00,0x00,0x00}},
{0xD8,5,{0x00,0x00,0x00,0x00,0x00}},
{0xE5,1,{0x06}},
{0xE6,1,{0x06}},
{0xE7,1,{0x06}},
{0xE8,1,{0x06}},
{0xE9,1,{0x06}},
{0xEA,1,{0x06}},
{0xEB,1,{0x00}},
{0xEC,1,{0x00}},

{0xED,1,{0x30}},

{0xF0,5,{0x55,0xAA,0x52,0x08,0x06}},
{0xB5,2,{0x10,0x13}},

{0xB6,2,{0x12,0x11}},

{0xB7,2,{0x00,0x01}},

{0xB8,2,{0x08,0x31}},

{0xB9,2,{0x31,0x31}},
{0xBA,2,{0x31,0x31}},
{0xBB,2,{0x31,0x08}},

{0xBC,2,{0x03,0x02}},
{0xBD,2,{0x17,0x18}},
{0xBE,2,{0x19,0x16}},

{0xD8,5,{0x00,0x00,0x00,0x00,0x00}},
{0xD9,5,{0x00,0x00,0x00,0x00,0x00}},

{0xE5,2,{0x00,0x00}},
{0xE7,1,{0x00}},

{0x6F,1,{0x01}},
{0xF9,1,{0x46}},
{0x6F,1,{0x11}},
{0xF3,1,{0x01}},

{0xF0,5,{0x55,0xAA,0x52,0x08,0x01}},
{0xBE,1,{0x2A}},//vcom 38 ;+-5/per// 0x2B

{0xFF, 5, {0xAA,0x55,0x25,0x01}},//3 line NT35521S
{0x6F, 2, {0x16}},
{0xF7, 2, {0x10}},

{0x35,1,{0x00}},
#endif

    {0x11,1,{0x00}},
    {REGFLAG_DELAY, 120, {}},

    {0x29,1,{0x00}},
    {REGFLAG_DELAY, 50, {}},


    {REGFLAG_END_OF_TABLE, 0x00, {}}

};

static void lcm_init(void)
{
    //unsigned int data_array[16];   
    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(50);
    SET_RESET_PIN(1);
    MDELAY(120);
    // lcm_initialization();
    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) 
            / sizeof(struct LCM_setting_table), 1);

}
/*
 //leung cover
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
    //MDELAY(1);

    data_array[0]= 0x00053902;
    data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
    data_array[2]= (y1_LSB);
    dsi_set_cmdq(data_array, 3, 1);
    //MDELAY(1);

    data_array[0]= 0x00290508;
    dsi_set_cmdq(data_array, 1, 1);
    //MDELAY(1);

    data_array[0]= 0x002c3909;
    dsi_set_cmdq(data_array, 1, 0);
    //MDELAY(1);

}
//cover end
*/

static void lcm_suspend(void)
{
    unsigned int data_array[16];
    data_array[0]=0x00280500;
    dsi_set_cmdq(data_array,1,1);
    MDELAY(10);
    data_array[0]=0x00100500;
    dsi_set_cmdq(data_array,1,1);
    MDELAY(100);
}


static void lcm_resume(void)
{
//yan
#if 0
    int len = sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table);
    int i = 0;
    for (i = 0; i < len; i++)
    {
        if (lcm_initialization_setting[i].cmd == 0xBE
            && lcm_initialization_setting[i].count == 1)
        {
            lcm_initialization_setting[i].para_list[0] += 1;
        }
    }
    lcm_init();
#else
    unsigned int data_array[16];
    data_array[0]=0x00110500;
    dsi_set_cmdq(data_array,1,1);
    MDELAY(100);
    data_array[0]=0x00290500;
    dsi_set_cmdq(data_array,1,1);
    MDELAY(10);
#endif
}


static unsigned int lcm_compare_id(void)
{
    unsigned int id=0;
    unsigned char buffer[3];
    unsigned int array[16]; 
    unsigned int data_array[16];

    SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(50);

    SET_RESET_PIN(1);
    MDELAY(120);

    data_array[0] = 0x00063902;
    data_array[1] = 0x52AA55F0; 
    data_array[2] = 0x00000108;               
    dsi_set_cmdq(data_array, 3, 1);

    array[0] = 0x00033700;// read id return two byte,version and id
    dsi_set_cmdq(array, 1, 1);

    read_reg_v2(0xC5, buffer, 3);
    id = buffer[1]; //we only need ID
#ifdef BUILD_LK
    printf("%s, LK nt35590 debug: nt35590 id = 0x%08x buffer[0]=0x%08x, buffer[1]=0x%08x,buffer[2]=0x%08x\n",
            __func__, id,buffer[0],buffer[1],buffer[2]);

#else
    printk("%s, LK nt35590 debug: nt35590 id = 0x%08x buffer[0]=0x%08x,buffer[1]=0x%08x,buffer[2]=0x%08x\n",
            __func__, id,buffer[0],buffer[1],buffer[2]);

#endif

    // if(id == LCM_ID_NT35521)
    if(buffer[0]==0x55 && buffer[1]==0x21)
        return 1;
    else
        return 0;


}




/*
 //leung cover here
static int err_count = 0;


static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_LK
    unsigned char buffer[8] = {0};
    unsigned int array[4];
    //int i =0;

    array[0] = 0x00013700;   
    dsi_set_cmdq(array, 1,1);
    read_reg_v2(0x0A, buffer,8);

    printk( "nt35521_JDI lcm_esd_check: buffer[0] = %d,buffer[1] = %d,buffer[2\
            ] = %d,buffer[3] = %d,buffer[4] = %d,buffer[5] = %d,buffer[6] = %d,buffer[7]\
            = %d\n",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7]);


    if((buffer[0] != 0x9C))//LCD work status error,need re-initalize//
    {
        printk( "nt35521_JDI lcm_esd_check buffer[0] = %d\n",buffer[0]);
        return TRUE;
    }
    else
    {
        if(buffer[3] != 0x02) //error data type is 0x02
        {
            //return FALSE;
            err_count = 0;
        }
        else
        {
            //if(((buffer[4] != 0) && (buffer[4] != 0x40)) ||  (buffer[5] != 0x80))

                if( (buffer[4] == 0x40) || (buffer[5] == 0x80))
                {
                    err_count = 0;
                }
                else
                {
                    err_count++;
                }            
            if(err_count >=2 )
            {
                err_count = 0;
                printk( "nt35521_JDI lcm_esd_check buffer[4] = %d , buffer[5\
                        ] = %d\n",buffer[4],buffer[5]);

                return TRUE;
            }
        }
        return FALSE;
    }
#endif

}


static unsigned int lcm_esd_recover(void)
{
    lcm_init();
    lcm_resume();

    return TRUE;
}
*/
//cover end

#if 0
static unsigned int lcm_compare_id(void)
{
    unsigned int id =0;
    unsigned char buffer[2];
    unsigned int arry[16];


    SET_RESET_PIN(1);
    MDELAY(10); 
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(20);

    arry[0]= 0x00023700; ; 

    dsi_set_cmdq(arry, 1, 1);

    read_reg_v2(0xF4,buffer,2);

    id = buffer[0];

#ifdef BUILD_LK
    printf("%s,LK nt35521 debug: nt35521 id =%x\n",__func__,id);
#else
    printk("%s,kernel nt35521 horse debug: nt35521 id =%x\n",__func__,id);
#endif

    if(id == LCM_ID)
        return 1;
    else
        return 0;
}
#endif
// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER nt35521_dsi_vdo_hd_t510_lg_drv = 
{
    .name            = "nt35521_dsi_vdo_hd_t510_lg",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id    = lcm_compare_id,
#if 0//defined(LCM_DSI_CMD_MODE)
    //    .set_backlight    = lcm_setbacklight,
    //.set_pwm        = lcm_setpwm,
    //.get_pwm        = lcm_getpwm,
    .update         = lcm_update
#endif
};


