#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif
#include <linux/gpio.h>
#include "lcm_drv.h"
#include <linux/delay.h>

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(640)
#define FRAME_HEIGHT 										(800)


#define REGFLAG_DELAY             							0XFFE
#define REGFLAG_END_OF_TABLE      							0xFFF   // END OF REGISTERS MARKER
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

//#define UDELAY(n) 											(lcm_util.udelay(n))
//#define MDELAY(n) 											(lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)    

#define FPGA_PWR1  (1<<1)
#define FPGA_PWR2  (1<<2)
#define LCM_VS     (1<<3)
#define LCM_RST    (1<<4)
#define HDMI_BOOST (1<<5)

extern void enable_fpga(int able,int module);

struct LCM_setting_table
{
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[20];
};
#if 0
static struct LCM_setting_table lcm_initialization_setting[] = {

{0xFF,3,{0x98,0x81,0x03}},
{0x01,1,{0x00}},
{0x02,1,{0x00}},
{0x03,1,{0x73}},
{0x04,1,{0x00}},
{0x05,1,{0x00}},
{0x06,1,{0x0A}},
{0x07,1,{0x00}},
{0x08,1,{0x00}},
{0x09,1,{0x01}},
{0x0a,1,{0x00}},//00 
{0x0b,1,{0x00}},
{0x0c,1,{0x01}},
{0x0d,1,{0x00}},
{0x0e,1,{0x00}},
{0x0f,1,{0x1D}},
{0x10,1,{0x1D}},//1D
{0x11,1,{0x00}},
{0x12,1,{0x00}},
{0x13,1,{0x00}},
{0x14,1,{0x00}},
{0x15,1,{0x00}},
{0x16,1,{0x00}},
{0x17,1,{0x00}},
{0x18,1,{0x00}},
{0x19,1,{0x00}},
{0x1a,1,{0x00}},
{0x1b,1,{0x00}},
{0x1c,1,{0x00}},
{0x1d,1,{0x00}},
{0x1e,1,{0x40}},
{0x1f,1,{0x80}},
{0x20,1,{0x06}},
{0x21,1,{0x02}},
{0x22,1,{0x00}},
{0x23,1,{0x00}},
{0x24,1,{0x00}},
{0x25,1,{0x00}},
{0x26,1,{0x00}},
{0x27,1,{0x00}},
{0x28,1,{0x33}},
{0x29,1,{0x03}},
{0x2a,1,{0x00}},
{0x2b,1,{0x00}},
{0x2c,1,{0x00}},
{0x2d,1,{0x00}},
{0x2e,1,{0x00}},
{0x2f,1,{0x00}},
{0x30,1,{0x00}},
{0x31,1,{0x00}},
{0x32,1,{0x00}},
{0x33,1,{0x00}},
{0x34,1,{0x04}},
{0x35,1,{0x00}},
{0x36,1,{0x00}},
{0x37,1,{0x00}},
{0x38,1,{0x3C}},
{0x39,1,{0x35}},
{0x3A,1,{0x01}},
{0x3B,1,{0x40}},
{0x3C,1,{0x00}},
{0x3D,1,{0x01}},
{0x3E,1,{0x00}},
{0x3F,1,{0x00}},
{0x40,1,{0x00}},
{0x41,1,{0x88}},
{0x42,1,{0x00}},
{0x43,1,{0x00}},
{0x44,1,{0x1F}},
{0x50,1,{0x01}},
{0x51,1,{0x23}},
{0x52,1,{0x45}},
{0x53,1,{0x67}},
{0x54,1,{0x89}},
{0x55,1,{0xab}},
{0x56,1,{0x01}},
{0x57,1,{0x23}},
{0x58,1,{0x45}},
{0x59,1,{0x67}},
{0x5a,1,{0x89}},
{0x5b,1,{0xab}},
{0x5c,1,{0xcd}},
{0x5d,1,{0xef}},
{0x5e,1,{0x11}},
{0x5f,1,{0x01}},
{0x60,1,{0x00}},
{0x61,1,{0x15}},
{0x62,1,{0x14}},
{0x63,1,{0x0E}},
{0x64,1,{0x0F}},
{0x65,1,{0x0C}},
{0x66,1,{0x0D}},
{0x67,1,{0x06}},
{0x68,1,{0x02}},
{0x69,1,{0x07}},
{0x6a,1,{0x02}},
{0x6b,1,{0x02}},
{0x6c,1,{0x02}},
{0x6d,1,{0x02}},
{0x6e,1,{0x02}},
{0x6f,1,{0x02}},
{0x70,1,{0x02}},
{0x71,1,{0x02}},
{0x72,1,{0x02}},
{0x73,1,{0x02}},
{0x74,1,{0x02}},
{0x75,1,{0x01}},
{0x76,1,{0x00}},
{0x77,1,{0x14}},
{0x78,1,{0x15}},
{0x79,1,{0x0E}},
{0x7a,1,{0x0F}},
{0x7b,1,{0x0C}},
{0x7c,1,{0x0D}},
{0x7d,1,{0x06}},
{0x7e,1,{0x02}},
{0x7f,1,{0x07}},
{0x80,1,{0x02}},
{0x81,1,{0x02}},
{0x82,1,{0x02}},
{0x83,1,{0x02}},
{0x84,1,{0x02}},
{0x85,1,{0x02}},
{0x86,1,{0x02}},
{0x87,1,{0x02}},
{0x88,1,{0x02}},
{0x89,1,{0x02}},
{0x8A,1,{0x02}},
{0xFF,3,{0x98,0x81,0x04}},
{0x70,1,{0x00}}, 
{0x71,1,{0x00}}, 
{0x82,1,{0x0F}},          //VGH_MOD clamp level=15v 
{0x84,1,{0x0F}},          //VGH clamp level 15V 
{0x85,1,{0x0D}},      //VGL clamp level (-10V) 
{0x32,1,{0xAC}}, 
{0x8C,1,{0x80}}, 
{0x3C,1,{0xF5}}, 
{0xB5,1,{0x07}},        //GAMMA OP 
{0x31,1,{0x45}},         //SOURCE OP 
{0x3A,1,{0x24}}, //24        //PS_EN OFF 
{0x88,1,{0x33}},  
{0xFF,3,{0x98,0x81,0x01}},
{0x22,1,{0x09}},         //BGR SS GS 
{0x31,1,{0x00}},         //column inversion 
{0x53,1,{0x8A}},         //VCOM1 
{0x55,1,{0xA2}},         //VCOM2 
{0x50,1,{0x81}},         //VREG1OUT=5V 
{0x51,1,{0x85}},         //VREG2OUT=-5V 
{0x62,1,{0x0D}},         //EQT Time setting 
{0xFF,3,{0x98,0x81,0x01}},
{0xA0,1,{0x00}},
{0xA1,1,{0x1A}},
{0xA2,1,{0x28}},
{0xA3,1,{0x13}},
{0xA4,1,{0x16}},
{0xA5,1,{0x29}},
{0xA6,1,{0x1D}},
{0xA7,1,{0x1E}},
{0xA8,1,{0x84}},
{0xA9,1,{0x1C}},
{0xAA,1,{0x28}},
{0xAB,1,{0x75}},
{0xAC,1,{0x1A}},
{0xAD,1,{0x19}},
{0xAE,1,{0x4D}},
{0xAF,1,{0x22}},
{0xB0,1,{0x28}},
{0xB1,1,{0x54}},
{0xB2,1,{0x66}},
{0xB3,1,{0x39}},
{0xC0,1,{0x00}},
{0xC1,1,{0x1A}},
{0xC2,1,{0x28}},
{0xC3,1,{0x13}},
{0xC4,1,{0x16}},
{0xC5,1,{0x29}},
{0xC6,1,{0x1D}},
{0xC7,1,{0x1E}},
{0xC8,1,{0x84}},
{0xC9,1,{0x1C}},
{0xCA,1,{0x28}},
{0xCB,1,{0x75}},
{0xCC,1,{0x1A}},
{0xCD,1,{0x19}},
{0xCE,1,{0x4D}},
{0xCF,1,{0x22}},
{0xD0,1,{0x28}},
{0xD1,1,{0x54}},
{0xD2,1,{0x66}},
{0xD3,1,{0x39}},
{0xFF,3,{0x98,0x81,0x00}}, 
{0x35,1,{0x00}},  
{0x36,1,{0x03}},
{0xFF,3,{0x98,0x81,0x00}}, 
	{0x28,1,{0x00}},
	{REGFLAG_DELAY, 10, {}},  
	{0x10,1,{0x00}},	
	{REGFLAG_DELAY, 120, {}},  
{0xFF,3,{0x98,0x81,0x00}}, 
	{0x11,1,{0x00}},
	{REGFLAG_DELAY, 120, {}},  
	{0x29,1,{0x00}},	
	{REGFLAG_DELAY, 10, {}},  
	//{REGFLAG_END_OF_TABLE, 0x00, {}}        

};



static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
	{0xFF,3,{0x98,0x81,0x03}},
{0x01,1,{0x00}},
{0x02,1,{0x00}},
{0x03,1,{0x73}},
{0x04,1,{0x00}},
{0x05,1,{0x00}},
{0x06,1,{0x0A}},
{0x07,1,{0x00}},
{0x08,1,{0x00}},
{0x09,1,{0x01}},
{0x0a,1,{0x00}},//00 
{0x0b,1,{0x00}},
{0x0c,1,{0x01}},
{0x0d,1,{0x00}},
{0x0e,1,{0x00}},
{0x0f,1,{0x1D}},
{0x10,1,{0x1D}},//1D
{0x11,1,{0x00}},
{0x12,1,{0x00}},
{0x13,1,{0x00}},
{0x14,1,{0x00}},
{0x15,1,{0x00}},
{0x16,1,{0x00}},
{0x17,1,{0x00}},
{0x18,1,{0x00}},
{0x19,1,{0x00}},
{0x1a,1,{0x00}},
{0x1b,1,{0x00}},
{0x1c,1,{0x00}},
{0x1d,1,{0x00}},
{0x1e,1,{0x40}},
{0x1f,1,{0x80}},
{0x20,1,{0x06}},
{0x21,1,{0x02}},
{0x22,1,{0x00}},
{0x23,1,{0x00}},
{0x24,1,{0x00}},
{0x25,1,{0x00}},
{0x26,1,{0x00}},
{0x27,1,{0x00}},
{0x28,1,{0x33}},
{0x29,1,{0x03}},
{0x2a,1,{0x00}},
{0x2b,1,{0x00}},
{0x2c,1,{0x00}},
{0x2d,1,{0x00}},
{0x2e,1,{0x00}},
{0x2f,1,{0x00}},
{0x30,1,{0x00}},
{0x31,1,{0x00}},
{0x32,1,{0x00}},
{0x33,1,{0x00}},
{0x34,1,{0x04}},
{0x35,1,{0x00}},
{0x36,1,{0x00}},
{0x37,1,{0x00}},
{0x38,1,{0x3C}},
{0x39,1,{0x35}},
{0x3A,1,{0x01}},
{0x3B,1,{0x40}},
{0x3C,1,{0x00}},
{0x3D,1,{0x01}},
{0x3E,1,{0x00}},
{0x3F,1,{0x00}},
{0x40,1,{0x00}},
{0x41,1,{0x88}},
{0x42,1,{0x00}},
{0x43,1,{0x00}},
{0x44,1,{0x1F}},
{0x50,1,{0x01}},
{0x51,1,{0x23}},
{0x52,1,{0x45}},
{0x53,1,{0x67}},
{0x54,1,{0x89}},
{0x55,1,{0xab}},
{0x56,1,{0x01}},
{0x57,1,{0x23}},
{0x58,1,{0x45}},
{0x59,1,{0x67}},
{0x5a,1,{0x89}},
{0x5b,1,{0xab}},
{0x5c,1,{0xcd}},
{0x5d,1,{0xef}},
{0x5e,1,{0x11}},
{0x5f,1,{0x01}},
{0x60,1,{0x00}},
{0x61,1,{0x15}},
{0x62,1,{0x14}},
{0x63,1,{0x0E}},
{0x64,1,{0x0F}},
{0x65,1,{0x0C}},
{0x66,1,{0x0D}},
{0x67,1,{0x06}},
{0x68,1,{0x02}},
{0x69,1,{0x07}},
{0x6a,1,{0x02}},
{0x6b,1,{0x02}},
{0x6c,1,{0x02}},
{0x6d,1,{0x02}},
{0x6e,1,{0x02}},
{0x6f,1,{0x02}},
{0x70,1,{0x02}},
{0x71,1,{0x02}},
{0x72,1,{0x02}},
{0x73,1,{0x02}},
{0x74,1,{0x02}},
{0x75,1,{0x01}},
{0x76,1,{0x00}},
{0x77,1,{0x14}},
{0x78,1,{0x15}},
{0x79,1,{0x0E}},
{0x7a,1,{0x0F}},
{0x7b,1,{0x0C}},
{0x7c,1,{0x0D}},
{0x7d,1,{0x06}},
{0x7e,1,{0x02}},
{0x7f,1,{0x07}},
{0x80,1,{0x02}},
{0x81,1,{0x02}},
{0x82,1,{0x02}},
{0x83,1,{0x02}},
{0x84,1,{0x02}},
{0x85,1,{0x02}},
{0x86,1,{0x02}},
{0x87,1,{0x02}},
{0x88,1,{0x02}},
{0x89,1,{0x02}},
{0x8A,1,{0x02}},
{0xFF,3,{0x98,0x81,0x04}},
{0x70,1,{0x00}}, 
{0x71,1,{0x00}}, 
{0x82,1,{0x0F}},          //VGH_MOD clamp level=15v 
{0x84,1,{0x0F}},          //VGH clamp level 15V 
{0x85,1,{0x0D}},      //VGL clamp level (-10V) 
{0x32,1,{0xAC}}, 
{0x8C,1,{0x80}}, 
{0x3C,1,{0xF5}}, 
{0xB5,1,{0x07}},        //GAMMA OP 
{0x31,1,{0x45}},         //SOURCE OP 
{0x3A,1,{0x24}}, //24        //PS_EN OFF 
{0x88,1,{0x33}},  
{0xFF,3,{0x98,0x81,0x01}},
{0x22,1,{0x09}},         //BGR SS GS 
{0x31,1,{0x00}},         //column inversion 
{0x53,1,{0x8A}},         //VCOM1 
{0x55,1,{0xA2}},         //VCOM2 
{0x50,1,{0x81}},         //VREG1OUT=5V 
{0x51,1,{0x85}},         //VREG2OUT=-5V 
{0x62,1,{0x0D}},         //EQT Time setting 
{0xFF,3,{0x98,0x81,0x01}},
{0xA0,1,{0x00}},
{0xA1,1,{0x1A}},
{0xA2,1,{0x28}},
{0xA3,1,{0x13}},
{0xA4,1,{0x16}},
{0xA5,1,{0x29}},
{0xA6,1,{0x1D}},
{0xA7,1,{0x1E}},
{0xA8,1,{0x84}},
{0xA9,1,{0x1C}},
{0xAA,1,{0x28}},
{0xAB,1,{0x75}},
{0xAC,1,{0x1A}},
{0xAD,1,{0x19}},
{0xAE,1,{0x4D}},
{0xAF,1,{0x22}},
{0xB0,1,{0x28}},
{0xB1,1,{0x54}},
{0xB2,1,{0x66}},
{0xB3,1,{0x39}},
{0xC0,1,{0x00}},
{0xC1,1,{0x1A}},
{0xC2,1,{0x28}},
{0xC3,1,{0x13}},
{0xC4,1,{0x16}},
{0xC5,1,{0x29}},
{0xC6,1,{0x1D}},
{0xC7,1,{0x1E}},
{0xC8,1,{0x84}},
{0xC9,1,{0x1C}},
{0xCA,1,{0x28}},
{0xCB,1,{0x75}},
{0xCC,1,{0x1A}},
{0xCD,1,{0x19}},
{0xCE,1,{0x4D}},
{0xCF,1,{0x22}},
{0xD0,1,{0x28}},
{0xD1,1,{0x54}},
{0xD2,1,{0x66}},
{0xD3,1,{0x39}},
{0xFF,3,{0x98,0x81,0x00}}, 
{0x35,1,{0x00}},  
{0x36,1,{0x03}},
{0xFF,3,{0x98,0x81,0x00}}, 
	{0x28,1,{0x00}},
	{REGFLAG_DELAY, 10, {}},  
	{0x10,1,{0x00}},	
    {REGFLAG_DELAY, 120, {}},
{0xFF,3,{0x98,0x81,0x00}}, 
	{0x11,1,{0x00}},
	{REGFLAG_DELAY, 120, {}},  
	{0x29,1,{0x00}},	
	{REGFLAG_DELAY, 10, {}},  
	//{REGFLAG_END_OF_TABLE, 0x00, {}}       
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Display off sequence
	{0x28, 0, {0x00}},

    // Sleep Mode On
	{0x10, 0, {0x00}},

	//{REGFLAG_END_OF_TABLE, 0x00, {}}
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
				//MDELAY(10);//soso add or it will fail to send register
       	}
    }
	
}
#endif

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

#ifdef CONFIG_RETROARCH_PLATFORM

static void lcm_get_params(LCM_PARAMS *params)
{
    memset(params, 0, sizeof(LCM_PARAMS));
    params->type   = LCM_TYPE_DSI;
    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;
	params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;
    params->dsi.mode   = SYNC_PULSE_VDO_MODE;//SYNC_EVENT_VDO_MODE; //BURST_VDO_MODE; //SYNC_PULSE_VDO_MODE;

	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM				= LCM_TWO_LANE; //LCM_THREE_LANE;  //LCM_TWO_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	params->dsi.packet_size=256;
	// Video mode setting           
	params->dsi.intermediat_buffer_num = 0;
    
    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    
	params->dsi.vertical_sync_active				= 8; //8;	//2;
	params->dsi.vertical_backporch					= 8; //18;	//14;
	params->dsi.vertical_frontporch					= 18; //20;	//16;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active				= 40;	//2;
	params->dsi.horizontal_backporch				= 50;//  
	params->dsi.horizontal_frontporch				= 50;//100;	//60;	//44;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	params->dsi.PLL_CLOCK = 240;//250;//200;//230;
	params->dsi.ssc_disable = 1;
	params->dsi.ssc_range = 4;
}


//static int lcm_vs_init = 0;

static void lcm_suspend(void)
{
	mdelay(50);
	SET_RESET_PIN(0);         // PINMUX_GPIO70__FUNC_LCM_RST
	mdelay(100);
	enable_fpga(0,FPGA_PWR2); // PINMUX_GPIO16__FUNC_GPIO16 
	mdelay(100);
	enable_fpga(0,FPGA_PWR1); // PINMUX_GPIO20__FUNC_GPIO20 
	mdelay(100);
	enable_fpga(0,LCM_VS);    // PINMUX_GPIO74__FUNC_GPIO74
	mdelay(50);
	enable_fpga(0,HDMI_BOOST);
	printk("lcm_suspend\n");
}

static void lcm_resume(void)
{
	// Disable VSP/VSN and Assert Reset (Active low)
	SET_RESET_PIN(0);
	enable_fpga(0,LCM_VS);
	mdelay(10);
	
	// Power up FPGA, need as least 210ms 10% before Config_Done
	enable_fpga(1,FPGA_PWR1);
	mdelay(10);
	enable_fpga(1,FPGA_PWR2);
	mdelay(250); 
	
	// FPGA need 55ms at most to complete LCM reset process
	SET_RESET_PIN(1);		
	mdelay(100);
	
	// Enable LCM VSP/VSN and Deassert Reset (Active High)	
	enable_fpga(1,LCM_VS);	
	enable_fpga(1,HDMI_BOOST);
	printk("lcm_resume\n");
}

static void lcm_init(void)
{
	// Disable VSP/VSN and Assert Reset (Active low)
	SET_RESET_PIN(0);
	enable_fpga(0,LCM_VS);
	mdelay(10);
	
	// Power up FPGA, need as least 210ms 10% before Config_Done
	enable_fpga(1,FPGA_PWR1);
	mdelay(10);
	enable_fpga(1,FPGA_PWR2);
	mdelay(250); 
	
	// FPGA need 55ms at most to complete LCM reset process
	SET_RESET_PIN(1);		
	mdelay(100);
	
	// Enable LCM VSP/VSN and Deassert Reset (Active High)	
	enable_fpga(1,LCM_VS);	
	enable_fpga(1,HDMI_BOOST);
	
	printk("lcm_init\n");
}
#endif
#ifdef CONFIG_RETROARCH_PLATFORM_POCKET1
static void lcm_get_params(LCM_PARAMS *params)
{
    memset(params, 0, sizeof(LCM_PARAMS));
    params->type   = LCM_TYPE_DSI;
    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;
	params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;
    params->dsi.mode   = SYNC_PULSE_VDO_MODE;//SYNC_EVENT_VDO_MODE; //BURST_VDO_MODE; //SYNC_PULSE_VDO_MODE;

	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM				= LCM_TWO_LANE; //LCM_THREE_LANE;  //LCM_TWO_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	params->dsi.packet_size=256;
	// Video mode setting           
	params->dsi.intermediat_buffer_num = 0;
    
    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    
	params->dsi.vertical_sync_active				= 8; //8;	//2;
	params->dsi.vertical_backporch					= 8; //18;	//14;
	params->dsi.vertical_frontporch					= 8; //20;	//16;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active				= 16;	//2;
	params->dsi.horizontal_backporch				= 43;//  
	params->dsi.horizontal_frontporch				= 43;//100;	//60;	//44;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	params->dsi.PLL_CLOCK = 230;//250;//200;//230;
	params->dsi.ssc_disable = 1;
	params->dsi.ssc_range = 4;
}


//static int lcm_vs_init = 0;

static void lcm_suspend(void)
{
	SET_RESET_PIN(0);         // PINMUX_GPIO70__FUNC_LCM_RST
	mdelay(50);
	enable_fpga(0,FPGA_PWR2); // PINMUX_GPIO16__FUNC_GPIO16 
	mdelay(50);
	enable_fpga(0,FPGA_PWR1); // PINMUX_GPIO20__FUNC_GPIO20 
	mdelay(50);
	enable_fpga(0,LCM_VS);    // PINMUX_GPIO74__FUNC_GPIO74
	enable_fpga(0,HDMI_BOOST);
	printk("lcm_suspend\n");
}

static void lcm_resume(void)
{
	SET_RESET_PIN(0);
	enable_fpga(0,LCM_VS);
	mdelay(20);
	enable_fpga(1,FPGA_PWR1);
	mdelay(20);
	enable_fpga(1,FPGA_PWR2);
	mdelay(300);
	SET_RESET_PIN(1);
	mdelay(50);
	enable_fpga(1,LCM_VS);
	enable_fpga(1,HDMI_BOOST);
	printk("lcm_resume\n");
}

static void lcm_init(void)
{
	SET_RESET_PIN(0);
	enable_fpga(0,LCM_VS);
	mdelay(20);
	enable_fpga(1,FPGA_PWR1);
	mdelay(20);
	enable_fpga(1,FPGA_PWR2);
	mdelay(300);
	SET_RESET_PIN(1);
	mdelay(50);
	enable_fpga(1,LCM_VS);
	printk("lcm_init\n");
	//push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}
#endif

LCM_DRIVER ili9881c_hd_dsi_vdo_lcm_drv = 
{
    .name			= "ili9881c_hd_dsi_vdo",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
};

