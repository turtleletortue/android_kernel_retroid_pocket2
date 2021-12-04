#include "lcm_drv.h"

#include <linux/string.h>
#include <linux/kernel.h>
#include <mt-plat/mt_gpio.h>


// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  			(480)
#define FRAME_HEIGHT 			(854)


#define REGFLAG_DELAY                                                                   0XFFE
#define REGFLAG_END_OF_TABLE                                                            0xFFF   // END OF REGISTERS MARKER

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    	(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 		(lcm_util.udelay(n))
#define MDELAY(n) 		(lcm_util.mdelay(n))



// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)        lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)           lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                                                          lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                                      lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)                                                                                   lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)                                   lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)         

struct LCM_setting_table 
{
    unsigned int cmd;
    unsigned char count;
    unsigned char para_list[64];
};



static struct LCM_setting_table lcm_initialization_setting[] = {
	
#if 0//old	
{0xFF,5, {0x77,0x01,0x00,0x00,0x10}},
{0xC0,2, {0xe9,0x03}},
{0xC1,2, {0x0D,0x02}},
{0xC2,2, {0x31,0x06}},
{0xcc,1, {0x10}},
{0xB0,16,{0x00,0x07,0x93,0x13,0x19,0x0B,0x0B,0x09,0x08,0x1F,0x08,0x15,0x11,0x0F,0x18,0x17}},
{0xB1,16,{0x00,0x07,0x92,0x12,0x15,0x09,0x08,0x09,0x09,0x1F,0x07,0x15,0x11,0x15,0x18,0x17}},
         
{0xFF,5, {0x77,0x01,0x00,0x00,0x11}},
{0xb0,1, {0x4D}},
{0xb1,1, {0x47}},
{0xb2,1, {0x07}},
{0xb3,1, {0x80}},
{0xb5,1, {0x47}},
{0xb7,1, {0x8a}},
{0xb8,1, {0x20}},
{0xb9,1, {0x10}},
{0xC1,1, {0x78}},
{0xC2,1, {0x78}},
{0xD0,1, {0x88}},
{REGFLAG_DELAY,100},
{0xe0,3, {0x00,0x00,0x02}},
{0xe1,11,{0x02,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x40,0x40}},
{0xe2,13,{0x33,0x33,0x34,0x34,0x62,0x00,0x63,0x00,0x61,0x00,0x64,0x00,0x00}},
{0xe3,4, {0x00,0x00,0x33,0x33}},
{0xe4,2, {0x44,0x44}},
{0xe5,16,{0x04,0x6B,0xA0,0xA0,0x06,0x6B,0xA0,0xA0,0x08,0x6B,0xA0,0xA0,0x0A,0x6B,0xA0,0xA0}},
{0xe6,4, {0x00,0x00,0x33,0x33}},
{0xe7,2, {0x44,0x44}},
{0xe8,16,{0x03,0x6B,0xA0,0xA0,0x05,0x6B,0xA0,0xA0,0x07,0x6B,0xA0,0xA0,0x09,0x6B,0xA0,0xA0}},
{0xeb,7, {0x02,0x00,0x39,0x39,0x88,0x33,0x10}},
{0xeC,2, {0x02,0x00}},
{0xed,16,{0xFF,0x04,0x56,0x7F,0x89,0xf2,0xff,0x3F,0xf3,0xff,0x2f,0x98,0xF7,0x65,0x40,0xFF}},
         
{0xFF,5, {0x77,0x01,0x00,0x00,0x00}},
	{0x11,1,{0x00}},				  // Sleep-Out
	{REGFLAG_DELAY,120},
	{0x29,1,{0x00}},				  // Display On
	{REGFLAG_DELAY,50},
#endif
#if 1	//new
//ST7701+IVO4.98 

	{0x11, 1,{0x00}},
	//------------------------------------------Bank0 Setting----------------------------------------------------//
	//------------------------------------Display Control setting----------------------------------------------//
	{REGFLAG_DELAY,20},
	{0xff, 5,{0x77, 0x01, 0x00, 0x00, 0x10}},

	{0xC0, 2,{0xE9, 0x03}},

	{0xC1, 2,{0x12, 0x02}},

	{0xC2, 2,{0x01, 0x08}},

	{0xCC, 1,{0x10}},
	//-------------------------------------Gamma Cluster Setting-------------------------------------------//
	{0xB0, 16,{0x40, 0x03, 0x0C, 0x14, 0x19, 0x0C, 0x0F, 0x09, 0x09, 0x21, 0x07, 0x13, 0x12, 0x10, 0x1A, 0x14}},

	{0xB1, 16,{0x40, 0x03, 0xCC, 0x10, 0x17, 0x0B, 0x0F, 0x09, 0x09, 0x27, 0x09, 0x17, 0x12, 0x1C, 0x1B, 0x14}},
	//---------------------------------------End Gamma Setting----------------------------------------------//
	//------------------------------------End Display Control setting----------------------------------------//
	//-----------------------------------------Bank0 Setting End---------------------------------------------//
	//-------------------------------------------Bank1 Setting---------------------------------------------------//
	//-------------------------------- Power Control Registers Initial --------------------------------------//
	{0xff, 5,{0x77, 0x01, 0x00, 0x00, 0x11}},

	{0xb0, 1,{0x4d}},
	//-------------------------------------------Vcom Setting---------------------------------------------------//
	{0xb1, 1,{0x4B}},//VCOM 4B
	//-----------------------------------------End Vcom Setting-----------------------------------------------//
	{0xb2, 1,{0x07}},//07

	{0xb3, 1,{0x80}},

	{0xb5, 1,{0x47}},//47

	{0xb7, 1,{0x85}},

	{0xB8, 1,{0x21}},//21

	{0xB9, 1,{0x10}},

	{0xC1, 1,{0x78}},

	{0xC2, 1,{0x78}},//78

	{0xD0, 1,{0x88}},
	//---------------------------------End Power Control Registers Initial -------------------------------//
	{REGFLAG_DELAY,100},
	//---------------------------------------------GIP Setting----------------------------------------------------//
	{0xe0, 3,{0x00, 0x00, 0x02}},

	{0xe1, 11,{0x0B, 0x00, 0x0D, 0x00, 0x0C, 0x00, 0x0E, 0x00, 0x00, 0x44, 0x44}},

	{0xe2, 13,{0x33, 0x33, 0x44, 0x44, 0x64, 0x00, 0x66, 0x00, 0x65, 0x00, 0x67, 0x00, 0x00}},

	{0xe3, 4,{0x00, 0x00, 0x33, 0x33}},

	{0xe4, 2,{0x44, 0x44}},

	{0xe5, 16,{0x0C, 0x78, 0xa0, 0xa0, 0x0E, 0x78, 0xa0, 0xa0, 0x10, 0x78, 0xa0, 0xa0, 0x12, 0x78, 0xa0, 0xa0}},

	{0xe6, 5,{0x00, 0x00, 0x33, 0x33}},

	{0xe7, 2,{0x44, 0x44}},

	{0xe8, 16,{0x0d, 0x78, 0xa0, 0xa0, 0x0f, 0x78, 0xa0, 0xa0, 0x11, 0x78, 0xa0, 0xa0, 0x13, 0x78, 0xa0, 0xa0}},

	{0xeb, 7,{0x02, 0x00, 0x39, 0x39, 0xEE, 0x44, 0x00}},

	{0xec, 2,{0x00, 0x00}},

	{0xed, 16,{0xff, 0xf1, 0x04, 0x56, 0x72, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xf3, 0x27, 0x65, 0x40, 0x1f, 0xff}},
	//---------------------------------------------End GIP Setting-----------------------------------------------//
	//------------------------------ Power Control Registers Initial End-----------------------------------//
	//------------------------------------------Bank1 Setting----------------------------------------------------//
	{REGFLAG_DELAY,10},
	{0xFF, 5,{0x77, 0x01, 0x00, 0x00, 0x00}},

	{0x29, 1,{0x00}},  	// DSPON
	{REGFLAG_DELAY,10},
#endif	
	
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_sleep_out_setting[] = {
	// Sleep Out
	{0x11, 0, {0x00}},
	{REGFLAG_DELAY, 120, {}},

	// Display ON
	{0x29, 0, {0x00}},
	{REGFLAG_DELAY, 10, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_sleep_in_setting[] = {
        // Display off sequence
        {0x28, 1, {0x00}},
        {REGFLAG_DELAY, 10, {}},

	// Sleep Mode On
        {0x10, 1, {0x00}},
        {REGFLAG_DELAY, 120, {}},

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
	params->dbi.te_mode	           	  = LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity            = LCM_POLARITY_RISING;

	params->dsi.mode   = SYNC_PULSE_VDO_MODE;

	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM	            = LCM_TWO_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	// Video mode setting           
	params->dsi.intermediat_buffer_num = 2;//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage
    
    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    
    params->dsi.word_count=FRAME_WIDTH*3;	//DSI CMD mode need set these two bellow params, different to 6577
    params->dsi.vertical_active_line=FRAME_HEIGHT;

		params->dsi.vertical_sync_active				= 4;// 3    2
		params->dsi.vertical_backporch					= 20;// 20   1
		params->dsi.vertical_frontporch					= 20; // 1  12
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

		params->dsi.horizontal_sync_active				= 10;// 50  2
		params->dsi.horizontal_backporch				= 78;
		params->dsi.horizontal_frontporch				= 80;
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

		params->dsi.PLL_CLOCK = 200;

}


static void lcm_suspend(void)
{
        push_table(lcm_sleep_in_setting, sizeof(lcm_sleep_in_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_init(void)
{

	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(120);

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_resume(void)
{
        //lcm_init();
        push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
}


#if 0
static unsigned int lcm_compare_id()
{
	int id=0;
	unsigned char buffer[4];
	unsigned int array[16];  
	//unsigned char params[5] = {0xFF,0x98,0x07,0x00,0x01};
	char id_high=0;
	char id_low=0;
        
	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(300);

	//array[0] = 0x00013700;//???กงก้1??byte
	array[0] = 0x00023700; //???กงก้2??byte
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0xA1, &buffer[0], 2);

	id_high=buffer[0];
	id_low=buffer[1];
	id=(id_high<<8)| id_low;
	#ifdef BUILD_LK
	printf("st7701_fwvga_dsi_vdo_lcm_drv %s:0x%2x,0x%2x,0x%2x,0x%2x id=0x%x\n", __func__,buffer[0],buffer[1],buffer[2],buffer[3], id);
	#else
	printk("st7701_fwvga_dsi_vdo_lcm_drv %s:0x%2x,0x%2x,0x%2x,0x%2x id=0x%x\n", __func__,buffer[0],buffer[1],buffer[2],buffer[3], id);
        #endif 
	return ((0x7701 == id) ? 1:0); 
	//return 1;
}
#endif
// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER st7701_fwvga_dsi_vdo_boe_drv = 
{
    	.name		= "st7701_fwvga_dsi_vdo_boe",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
//	.compare_id    = lcm_compare_id,		

};

