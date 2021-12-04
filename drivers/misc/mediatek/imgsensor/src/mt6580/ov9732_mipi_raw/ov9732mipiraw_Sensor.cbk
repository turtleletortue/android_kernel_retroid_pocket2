/*****************************************************************************
 *
 * Filename:
 * ---------
 *     OV9732mipi_Sensor.c
 *
 * Project:
 * --------
 *     ALPS
 *
 * Description:
 * ------------
 *     Source code of Sensor driver
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
//#include <asm/system.h>
//#include <linux/xlog.h>
#include <linux/types.h>
#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "ov9732mipiraw_Sensor.h"

/****************************Modify following Strings for debug****************************/
#define PFX "ov9732_camera_sensor"

//#define LOG_1 LOG_INF("OV9732,MIPI 1LANE\n")
//#define LOG_2 LOG_INF("preview 1470*1100@30fps,256Mbps/lane\n")
/****************************   Modify end    *******************************************/
//#define LOG_INF(format, args...)	xlog_printk(ANDROID_LOG_INFO   , PFX, "[%s] " format, __FUNCTION__, ##args)
#define LOG_INF printk
static DEFINE_SPINLOCK(imgsensor_drv_lock);

#define MIPI_SETTLEDELAY_AUTO     0
#define MIPI_SETTLEDELAY_MANNUAL  1


static imgsensor_info_struct imgsensor_info = { 
	.sensor_id = OV9732MIPI_SENSOR_ID,//OV9732MIPI_SENSOR_ID
	
	.checksum_value = 0x6ab1039,//0x1ec5153d,
	
	.pre = {
		.pclk = 36000000,				//record different mode's pclk
		.linelength = 1478,				//record different mode's linelength
		.framelength = 802,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 1280,		//record different mode's width of grabwindow
		.grabwindow_height = 720,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 14,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,	
	},
	.cap = {
		.pclk = 36000000,
		.linelength = 1478,
		.framelength = 802,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1280,
		.grabwindow_height = 720,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
	},
	.cap1 = {
		.pclk = 36000000,
		.linelength = 1478,
		.framelength = 802,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1280,
		.grabwindow_height = 720,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,	
	},
	.normal_video = {
		.pclk = 36000000,
		.linelength = 1478,
		.framelength = 802,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1280,
		.grabwindow_height = 720,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
	},
	.hs_video = {
		.pclk = 36000000,
		.linelength = 1478,
		.framelength = 802,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1280,
		.grabwindow_height = 720,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
	},
	.slim_video = {
		.pclk = 36000000,
		.linelength = 1478,
		.framelength = 802,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1280,
		.grabwindow_height = 720,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
	},
	.margin = 4,			//sensor framelength & shutter margin
	.min_shutter = 1,		//min shutter
	.max_frame_length = 0x7fff,//max framelength by sensor register's limitation
	.ae_shut_delay_frame = 0,	//shutter delay frame for AE cycle, 2 frame with ispGain_delay-shut_delay=2-0=2
	.ae_sensor_gain_delay_frame = 0,//sensor gain delay frame for AE cycle,2 frame with ispGain_delay-sensor_gain_delay=2-0=2
	.ae_ispGain_delay_frame = 2,//isp gain delay frame for AE cycle
	.ihdr_support = 0,	  //1, support; 0,not support
	.ihdr_le_firstline = 0,  //1,le first ; 0, se first
	.sensor_mode_num = 5,	  //support sensor mode num
	
	.cap_delay_frame = 2,		//enter capture delay frame num
	.pre_delay_frame = 2,		//enter preview delay frame num
	.video_delay_frame = 2, 	//enter video delay frame num
	.hs_video_delay_frame = 2,	//enter high speed video  delay frame num
	.slim_video_delay_frame = 2,//enter slim video delay frame num
	
	.isp_driving_current = ISP_DRIVING_2MA, //mclk driving current
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,//sensor_interface_type
    .mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
    .mipi_settle_delay_mode = 1,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,//SENSOR_OUTPUT_FORMAT_RAW_R, // xen 2010506

	.mclk = 24,//mclk value, suggest 24 or 26 for 24Mhz or 26Mhz
	.mipi_lane_num = SENSOR_MIPI_1_LANE,//mipi lane num
	.i2c_addr_table = {0x6c, 0x20, 0xff},//record sensor support all write id addr, only supprt 4must end with 0xff
    .i2c_speed = 300, // i2c read/write speed
};


static imgsensor_struct imgsensor = {

	.mirror = IMAGE_H_MIRROR, //IMAGE_NORMAL,	//IMAGE_V_MIRROR			//mirrorflip information
	.sensor_mode = IMGSENSOR_MODE_INIT, //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
	.shutter = 0x3D0,					//current shutter
	.gain = 0x100,						//current gain
	.dummy_pixel = 0,					//current dummypixel
	.dummy_line = 0,					//current dummyline
   	.current_fps = 300,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
	.autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
	.test_pattern = KAL_FALSE,		//test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
	.ihdr_en = 0, //sensor need support LE, SE with HDR feature
	.i2c_write_id = 0x6c,
};


/* Sensor output window information */
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] =
{
	
	{ 1280, 720,	  0,  0, 1280, 720, 1280, 720, 0, 0, 1280, 720,	  0,	0, 1280, 720},
	{ 1280, 720,	  0,  0, 1280, 720, 1280, 720, 0, 0, 1280, 720,	  0,	0, 1280, 720}, // Preview 
	{ 1280, 720,	  0,  0, 1280, 720, 1280, 720, 0, 0, 1280, 720,	  0,	0, 1280, 720}, // capture 
	{ 1280, 720,	  0,  0, 1280, 720, 1280, 720, 0, 0, 1280, 720,	  0,	0, 1280, 720}, // video 
	{ 1280, 720,	  0,  0, 1280, 720, 1280, 720, 0, 0, 1280, 720,	  0,	0, 1280, 720} //hight speed video 
	
};// slim video 

// Gain Index
//#define MaxGainIndex (97)

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
	char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
    	kdSetI2CSpeed(imgsensor_info.i2c_speed); // Add this func to set i2c speed by each sensor
	iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, imgsensor.i2c_write_id);
	return get_byte;
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};
    	kdSetI2CSpeed(imgsensor_info.i2c_speed); // Add this func to set i2c speed by each sensor
	iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
}

static void set_dummy(void)
{
	LOG_INF("dummyline = %d, dummypixels = %d \n", imgsensor.dummy_line, imgsensor.dummy_pixel);
    write_cmos_sensor(0x380E, (imgsensor.frame_length >>8) & 0xFF);
    write_cmos_sensor(0x380F, imgsensor.frame_length & 0xFF);	
    write_cmos_sensor(0x380C, (imgsensor.line_length >>8) & 0xFF);
    write_cmos_sensor(0x380D, imgsensor.line_length & 0xFF); 
}	/*	set_dummy  */

extern u32 sensor_pinSetIdx;
static kal_uint32 return_sensor_id(void)
{
//if (sensor_pinSetIdx == 1)
    return ((read_cmos_sensor(0x300a) << 8) | read_cmos_sensor(0x300b));
//else
    //return 0;
}


static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{
	//kal_int16 dummy_line;
	kal_uint32 frame_length = imgsensor.frame_length;
	//unsigned long flags;
	LOG_INF("framerate = %d, min framelength should enable\n", framerate);   
	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	spin_lock(&imgsensor_drv_lock);
	imgsensor.frame_length = (frame_length > imgsensor.min_frame_length)?frame_length:imgsensor.min_frame_length;
	imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	//dummy_line = frame_length - imgsensor.min_frame_length;
	//if (dummy_line < 0)
	//	imgsensor.dummy_line = 0;
	//else
	//	imgsensor.dummy_line = dummy_line;
	//imgsensor.frame_length = frame_length + imgsensor.dummy_line;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
	{
		imgsensor.frame_length = imgsensor_info.max_frame_length;
		imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	}
	if (min_framelength_en)
		imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);
	set_dummy();
}	/*	set_max_framerate  */


/*************************************************************************
* FUNCTION
*    set_shutter
*
* DESCRIPTION
*    This function set e-shutter of sensor to change exposure time.
*
* PARAMETERS
*    iShutter : exposured lines
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void set_shutter(kal_uint16 shutter)
{
    unsigned long flags;
    kal_uint16 realtime_fps = 0;
   // kal_uint32 frame_length = 0;
    spin_lock_irqsave(&imgsensor_drv_lock, flags);
    imgsensor.shutter = shutter;
    spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

    //write_shutter(shutter);
    /* 0x3500, 0x3501, 0x3502 will increase VBLANK to get exposure larger than frame exposure */
    /* AE doesn't update sensor gain at capture mode, thus extra exposure lines must be updated here. */

    // OV Recommend Solution
    // if shutter bigger than frame_length, should extend frame length first
	if(!shutter) shutter = 1; /*avoid 0*/
    spin_lock(&imgsensor_drv_lock);
    if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
        imgsensor.frame_length = shutter + imgsensor_info.margin;
    else
        imgsensor.frame_length = imgsensor.min_frame_length;
    if (imgsensor.frame_length > imgsensor_info.max_frame_length)
        imgsensor.frame_length = imgsensor_info.max_frame_length;
    spin_unlock(&imgsensor_drv_lock);
    shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
    shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;

    if (imgsensor.autoflicker_en) {
        realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
        if(realtime_fps >= 297 && realtime_fps <= 305)
            set_max_framerate(296,0);
        else if(realtime_fps >= 147 && realtime_fps <= 150)
            set_max_framerate(146,0);
        else {
        // Extend frame length
		write_cmos_sensor(0x380E, (imgsensor.frame_length >> 8) & 0xFF);
		write_cmos_sensor(0x380F, imgsensor.frame_length & 0xFF);
        }
    } else {
        // Extend frame length
		write_cmos_sensor(0x380E, (imgsensor.frame_length >> 8) & 0xFF);
		write_cmos_sensor(0x380F, imgsensor.frame_length & 0xFF);

    }

    // Update Shutter
	write_cmos_sensor(0x3502, (shutter << 4) & 0xFF);
	write_cmos_sensor(0x3501, (shutter >> 4) & 0xFF);	  
	write_cmos_sensor(0x3500, (shutter >> 12) & 0x0F);	
    LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);

}    /*    set_shutter */

/*************************************************************************
* FUNCTION
*	set_gain
*
* DESCRIPTION
*	This function is to set global gain to sensor.
*
* PARAMETERS
*	iGain : sensor global gain(base: 0x40)
*
* RETURNS
*	the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint16 set_gain(kal_uint16 gain)
{
	kal_uint16 reg_gain;

	/* 0x350A[0:1], 0x350B[0:7] AGC real gain */
	/* [0:3] = N meams N /16 X	*/
	/* [4:9] = M meams M X		 */
	/* Total gain = M + N /16 X   */
	if (gain < BASEGAIN || gain >= 16 * BASEGAIN) {
		LOG_INF("Error gain setting");

		if (gain < BASEGAIN)
			gain = BASEGAIN;
		else if (gain >= 16 * BASEGAIN)
			gain = 15.9 * BASEGAIN;		 
	}
	/*reg_gain = gain2reg(gain);*/
	
	reg_gain = gain/4; //sensor gain base 1x= 16, reg_gain = gain/(64*16);
	reg_gain &= 0x3ff;
		
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain; 
	spin_unlock(&imgsensor_drv_lock);
	LOG_INF("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);
	write_cmos_sensor(0x350a, (reg_gain>>8)& 0xFF);
	write_cmos_sensor(0x350b, reg_gain & 0xFF);	
	return gain;
}	/*	set_gain  */

static void set_mirror_flip(kal_uint8 image_mirror)
{
	LOG_INF("image_mirror = %d\n", image_mirror);

	/********************************************************
	   *
	   *   0x0101 Sensor mirror flip 
	   *
	   *   ISP and Sensor flip or mirror register bit should be the same!!
	   *
	   ********************************************************/
	/*kal_uint8  iTemp; 
	
	iTemp = read_cmos_sensor(0x0101);
	iTemp&= ~0x03; //Clear the mirror and flip bits.
	switch (image_mirror) 
	{
		case IMAGE_NORMAL:
			write_cmos_sensor_8(0x0101, iTemp);    //Set normal
			break;
		case IMAGE_H_MIRROR:
            write_cmos_sensor_8(0x0101, iTemp | 0x01); //Set mirror
			break;
		case IMAGE_V_MIRROR:
            write_cmos_sensor_8(0x0101, iTemp | 0x02); //Set flip
			break;
		case IMAGE_HV_MIRROR:
            write_cmos_sensor_8(0x0101, iTemp | 0x03); //Set mirror and flip
			break;
		default:
			LOG_INF("Error image_mirror setting\n");
	}*/

}

/*************************************************************************
* FUNCTION
*	night_mode
*
* DESCRIPTION
*	This function night mode of sensor.
*
* PARAMETERS
*	bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void night_mode(kal_bool enable)
{
/*No Need to implement this function*/ 
}	/*	night_mode	*/

static void sensor_init(void)
{
	LOG_INF("E\n");
	write_cmos_sensor(0x0103, 0x01); //;S/W reset
	mdelay(10); 									//;insert 10ms delay here

}	/*	sensor_init  */


static void preview_setting(void)
{
    write_cmos_sensor(0x0100,0x00);
    write_cmos_sensor(0x3001,0x00);
    write_cmos_sensor(0x3002,0x00);
    write_cmos_sensor(0x3007,0x00);
    write_cmos_sensor(0x3009,0x02);
    write_cmos_sensor(0x3010,0x00);
    write_cmos_sensor(0x3011,0x08);
    write_cmos_sensor(0x3014,0x22);
    write_cmos_sensor(0x301e,0x15);
    write_cmos_sensor(0x3030,0x19);
    write_cmos_sensor(0x3080,0x02);
    write_cmos_sensor(0x3081,0x3c);
    write_cmos_sensor(0x3082,0x04);
    write_cmos_sensor(0x3083,0x00);
    write_cmos_sensor(0x3084,0x02);
    write_cmos_sensor(0x3085,0x01);
    write_cmos_sensor(0x3086,0x01);
    write_cmos_sensor(0x3089,0x01);
    write_cmos_sensor(0x308a,0x00);
    write_cmos_sensor(0x3103,0x01);
    write_cmos_sensor(0x3600,0xff);
    write_cmos_sensor(0x3601,0x72);
    write_cmos_sensor(0x3610,0x0c);
    write_cmos_sensor(0x3611,0xb0);
    write_cmos_sensor(0x3612,0x35);
    write_cmos_sensor(0x3654,0x10);
    write_cmos_sensor(0x3655,0x77);
    write_cmos_sensor(0x3656,0x77);
    write_cmos_sensor(0x3657,0x07);
    write_cmos_sensor(0x3658,0x22);
    write_cmos_sensor(0x3659,0x22);
    write_cmos_sensor(0x365a,0x02);
    write_cmos_sensor(0x3700,0x1f);
    write_cmos_sensor(0x3701,0x10);
    write_cmos_sensor(0x3702,0x0c);
    write_cmos_sensor(0x3703,0x07);
    write_cmos_sensor(0x3704,0x3c);
    write_cmos_sensor(0x3705,0x41);
    write_cmos_sensor(0x370d,0x10);
    write_cmos_sensor(0x3710,0x0c);
    write_cmos_sensor(0x3783,0x08);
    write_cmos_sensor(0x3784,0x05);
    write_cmos_sensor(0x3785,0x55);
    write_cmos_sensor(0x37c0,0x07);
    write_cmos_sensor(0x3800,0x00);
    write_cmos_sensor(0x3801,0x04);
    write_cmos_sensor(0x3802,0x00);
    write_cmos_sensor(0x3803,0x04);
    write_cmos_sensor(0x3804,0x05);
    write_cmos_sensor(0x3805,0x0b);
    write_cmos_sensor(0x3806,0x02);
    write_cmos_sensor(0x3807,0xdb);
    write_cmos_sensor(0x3808,0x05);
    write_cmos_sensor(0x3809,0x00);
    write_cmos_sensor(0x380a,0x02);
    write_cmos_sensor(0x380b,0xd0);
    write_cmos_sensor(0x380c,0x05);
    write_cmos_sensor(0x380d,0xc6);
    write_cmos_sensor(0x380e,0x03);
    write_cmos_sensor(0x380f,0x22);
    write_cmos_sensor(0x3810,0x00);
    write_cmos_sensor(0x3811,0x04);
    write_cmos_sensor(0x3812,0x00);
    write_cmos_sensor(0x3813,0x04);
    write_cmos_sensor(0x3816,0x00);
    write_cmos_sensor(0x3817,0x00);
    write_cmos_sensor(0x3818,0x00);
    write_cmos_sensor(0x3819,0x04);
    write_cmos_sensor(0x3820,0x10);
    write_cmos_sensor(0x3821,0x00);
    write_cmos_sensor(0x382c,0x06);
    write_cmos_sensor(0x3500,0x00);
    write_cmos_sensor(0x3501,0x31);
    write_cmos_sensor(0x3502,0x00);
    write_cmos_sensor(0x3503,0x03);
    write_cmos_sensor(0x3504,0x00);
    write_cmos_sensor(0x3505,0x00);
    write_cmos_sensor(0x3509,0x10);
    write_cmos_sensor(0x350a,0x00);
    write_cmos_sensor(0x350b,0x40);
    write_cmos_sensor(0x3d00,0x00);
    write_cmos_sensor(0x3d01,0x00);
    write_cmos_sensor(0x3d02,0x00);
    write_cmos_sensor(0x3d03,0x00);
    write_cmos_sensor(0x3d04,0x00);
    write_cmos_sensor(0x3d05,0x00);
    write_cmos_sensor(0x3d06,0x00);
    write_cmos_sensor(0x3d07,0x00);
    write_cmos_sensor(0x3d08,0x00);
    write_cmos_sensor(0x3d09,0x00);
    write_cmos_sensor(0x3d0a,0x00);
    write_cmos_sensor(0x3d0b,0x00);
    write_cmos_sensor(0x3d0c,0x00);
    write_cmos_sensor(0x3d0d,0x00);
    write_cmos_sensor(0x3d0e,0x00);
    write_cmos_sensor(0x3d0f,0x00);
    write_cmos_sensor(0x3d80,0x00);
    write_cmos_sensor(0x3d81,0x00);
    write_cmos_sensor(0x3d82,0x38);
    write_cmos_sensor(0x3d83,0xa4);
    write_cmos_sensor(0x3d84,0x00);
    write_cmos_sensor(0x3d85,0x00);
    write_cmos_sensor(0x3d86,0x1f);
    write_cmos_sensor(0x3d87,0x03);
    write_cmos_sensor(0x3d8b,0x00);
    write_cmos_sensor(0x3d8f,0x00);
    write_cmos_sensor(0x4001,0xe0);
    write_cmos_sensor(0x4004,0x00);
    write_cmos_sensor(0x4005,0x02);
    write_cmos_sensor(0x4006,0x01);
    write_cmos_sensor(0x4007,0x40);
    write_cmos_sensor(0x4009,0x0b);
    write_cmos_sensor(0x4300,0x03);
    write_cmos_sensor(0x4301,0xff);
    write_cmos_sensor(0x4304,0x00);
    write_cmos_sensor(0x4305,0x00);
    write_cmos_sensor(0x4309,0x00);
    write_cmos_sensor(0x4600,0x00);
    write_cmos_sensor(0x4601,0x04);
    write_cmos_sensor(0x4800,0x00);
    write_cmos_sensor(0x4805,0x00);
    write_cmos_sensor(0x4821,0x50);
    write_cmos_sensor(0x4823,0x50);
    write_cmos_sensor(0x4837,0x2d);
    write_cmos_sensor(0x4a00,0x00);
    write_cmos_sensor(0x4f00,0x80);
    write_cmos_sensor(0x4f01,0x10);
    write_cmos_sensor(0x4f02,0x00);
    write_cmos_sensor(0x4f03,0x00);
    write_cmos_sensor(0x4f04,0x00);
    write_cmos_sensor(0x4f05,0x00);
    write_cmos_sensor(0x4f06,0x00);
    write_cmos_sensor(0x4f07,0x00);
    write_cmos_sensor(0x4f08,0x00);
    write_cmos_sensor(0x4f09,0x00);
    write_cmos_sensor(0x5000,0x2f);
    write_cmos_sensor(0x500c,0x00);
    write_cmos_sensor(0x500d,0x00);
    write_cmos_sensor(0x500e,0x00);
    write_cmos_sensor(0x500f,0x00);
    write_cmos_sensor(0x5010,0x00);
    write_cmos_sensor(0x5011,0x00);
    write_cmos_sensor(0x5012,0x00);
    write_cmos_sensor(0x5013,0x00);
    write_cmos_sensor(0x5014,0x00);
    write_cmos_sensor(0x5015,0x00);
    write_cmos_sensor(0x5016,0x00);
    write_cmos_sensor(0x5017,0x00);
    write_cmos_sensor(0x5080,0x00);
    write_cmos_sensor(0x5180,0x01);
    write_cmos_sensor(0x5181,0x00);
    write_cmos_sensor(0x5182,0x01);
    write_cmos_sensor(0x5183,0x00);
    write_cmos_sensor(0x5184,0x01);
    write_cmos_sensor(0x5185,0x00);
    write_cmos_sensor(0x5708,0x06);
    write_cmos_sensor(0x5781,0x00);
    write_cmos_sensor(0x5783,0x0f);
    write_cmos_sensor(0x0100,0x01);
    write_cmos_sensor(0x3600,0xf6);
    write_cmos_sensor(0x5c80,0x05);
    write_cmos_sensor(0x5c81,0x60);
    write_cmos_sensor(0x5c82,0x09);
    write_cmos_sensor(0x5c83,0x5f);
    write_cmos_sensor(0x5c85,0x6c);
    write_cmos_sensor(0x5601,0x04);
    write_cmos_sensor(0x5602,0x02);
    write_cmos_sensor(0x5603,0x01);
    write_cmos_sensor(0x5604,0x04);
    write_cmos_sensor(0x5605,0x02);
    write_cmos_sensor(0x5606,0x01);
    write_cmos_sensor(0x5400,0xff);
    write_cmos_sensor(0x5401,0xc8);
    write_cmos_sensor(0x5402,0x9f);
    write_cmos_sensor(0x5403,0x8b);
    write_cmos_sensor(0x5404,0x83);
    write_cmos_sensor(0x5405,0x86);
    write_cmos_sensor(0x5406,0x91);
    write_cmos_sensor(0x5407,0xa9);
    write_cmos_sensor(0x5408,0xe1);
    write_cmos_sensor(0x5409,0xff);
    write_cmos_sensor(0x540a,0x7e);
    write_cmos_sensor(0x540b,0x60);
    write_cmos_sensor(0x540c,0x50);
    write_cmos_sensor(0x540d,0x47);
    write_cmos_sensor(0x540e,0x43);
    write_cmos_sensor(0x540f,0x44);
    write_cmos_sensor(0x5410,0x49);
    write_cmos_sensor(0x5411,0x55);
    write_cmos_sensor(0x5412,0x65);
    write_cmos_sensor(0x5413,0x8c);
    write_cmos_sensor(0x5414,0x4f);
    write_cmos_sensor(0x5415,0x3c);
    write_cmos_sensor(0x5416,0x31);
    write_cmos_sensor(0x5417,0x2a);
    write_cmos_sensor(0x5418,0x27);
    write_cmos_sensor(0x5419,0x28);
    write_cmos_sensor(0x541a,0x2c);
    write_cmos_sensor(0x541b,0x33);
    write_cmos_sensor(0x541c,0x3f);
    write_cmos_sensor(0x541d,0x56);
    write_cmos_sensor(0x541e,0x34);
    write_cmos_sensor(0x541f,0x26);
    write_cmos_sensor(0x5420,0x1d);
    write_cmos_sensor(0x5421,0x18);
    write_cmos_sensor(0x5422,0x15);
    write_cmos_sensor(0x5423,0x15);
    write_cmos_sensor(0x5424,0x19);
    write_cmos_sensor(0x5425,0x1f);
    write_cmos_sensor(0x5426,0x29);
    write_cmos_sensor(0x5427,0x39);
    write_cmos_sensor(0x5428,0x27);
    write_cmos_sensor(0x5429,0x1a);
    write_cmos_sensor(0x542a,0x11);
    write_cmos_sensor(0x542b,0x0a);
    write_cmos_sensor(0x542c,0x07);
    write_cmos_sensor(0x542d,0x07);
    write_cmos_sensor(0x542e,0x0b);
    write_cmos_sensor(0x542f,0x12);
    write_cmos_sensor(0x5430,0x1c);
    write_cmos_sensor(0x5431,0x2b);
    write_cmos_sensor(0x5432,0x20);
    write_cmos_sensor(0x5433,0x14);
    write_cmos_sensor(0x5434,0x0a);
    write_cmos_sensor(0x5435,0x05);
    write_cmos_sensor(0x5436,0x02);
    write_cmos_sensor(0x5437,0x02);
    write_cmos_sensor(0x5438,0x05);
    write_cmos_sensor(0x5439,0x0c);
    write_cmos_sensor(0x543a,0x16);
    write_cmos_sensor(0x543b,0x24);
    write_cmos_sensor(0x543c,0x21);
    write_cmos_sensor(0x543d,0x14);
    write_cmos_sensor(0x543e,0x0b);
    write_cmos_sensor(0x543f,0x04);
    write_cmos_sensor(0x5440,0x02);
    write_cmos_sensor(0x5441,0x02);
    write_cmos_sensor(0x5442,0x06);
    write_cmos_sensor(0x5443,0x0c);
    write_cmos_sensor(0x5444,0x16);
    write_cmos_sensor(0x5445,0x25);
    write_cmos_sensor(0x5446,0x29);
    write_cmos_sensor(0x5447,0x1b);
    write_cmos_sensor(0x5448,0x12);
    write_cmos_sensor(0x5449,0x0c);
    write_cmos_sensor(0x544a,0x08);
    write_cmos_sensor(0x544b,0x09);
    write_cmos_sensor(0x544c,0x0d);
    write_cmos_sensor(0x544d,0x14);
    write_cmos_sensor(0x544e,0x1e);
    write_cmos_sensor(0x544f,0x2c);
    write_cmos_sensor(0x5450,0x36);
    write_cmos_sensor(0x5451,0x28);
    write_cmos_sensor(0x5452,0x1f);
    write_cmos_sensor(0x5453,0x19);
    write_cmos_sensor(0x5454,0x17);
    write_cmos_sensor(0x5455,0x17);
    write_cmos_sensor(0x5456,0x1b);
    write_cmos_sensor(0x5457,0x21);
    write_cmos_sensor(0x5458,0x2b);
    write_cmos_sensor(0x5459,0x3d);
    write_cmos_sensor(0x545a,0x54);
    write_cmos_sensor(0x545b,0x40);
    write_cmos_sensor(0x545c,0x34);
    write_cmos_sensor(0x545d,0x2e);
    write_cmos_sensor(0x545e,0x2b);
    write_cmos_sensor(0x545f,0x2b);
    write_cmos_sensor(0x5460,0x2f);
    write_cmos_sensor(0x5461,0x38);
    write_cmos_sensor(0x5462,0x46);
    write_cmos_sensor(0x5463,0x5d);
    write_cmos_sensor(0x5464,0x8a);
    write_cmos_sensor(0x5465,0x6a);
    write_cmos_sensor(0x5466,0x58);
    write_cmos_sensor(0x5467,0x4f);
    write_cmos_sensor(0x5468,0x4b);
    write_cmos_sensor(0x5469,0x4d);
    write_cmos_sensor(0x546a,0x52);
    write_cmos_sensor(0x546b,0x5e);
    write_cmos_sensor(0x546c,0x72);
    write_cmos_sensor(0x546d,0x9f);
    write_cmos_sensor(0x546e,0xff);
    write_cmos_sensor(0x546f,0xe9);
    write_cmos_sensor(0x5470,0xb6);
    write_cmos_sensor(0x5471,0x9e);
    write_cmos_sensor(0x5472,0x96);
    write_cmos_sensor(0x5473,0x96);
    write_cmos_sensor(0x5474,0xa4);
    write_cmos_sensor(0x5475,0xc6);
    write_cmos_sensor(0x5476,0xff);
    write_cmos_sensor(0x5477,0xff);
    write_cmos_sensor(0x5478,0x6d);
    write_cmos_sensor(0x5479,0x70);
    write_cmos_sensor(0x547a,0x71);
    write_cmos_sensor(0x547b,0x74);
    write_cmos_sensor(0x547c,0x73);
    write_cmos_sensor(0x547d,0x73);
    write_cmos_sensor(0x547e,0x74);
    write_cmos_sensor(0x547f,0x73);
    write_cmos_sensor(0x5480,0x73);
    write_cmos_sensor(0x5481,0x70);
    write_cmos_sensor(0x5482,0x70);
    write_cmos_sensor(0x5483,0x6f);
    write_cmos_sensor(0x5484,0x6f);
    write_cmos_sensor(0x5485,0x70);
    write_cmos_sensor(0x5486,0x71);
    write_cmos_sensor(0x5487,0x71);
    write_cmos_sensor(0x5488,0x71);
    write_cmos_sensor(0x5489,0x71);
    write_cmos_sensor(0x548a,0x6f);
    write_cmos_sensor(0x548b,0x72);
    write_cmos_sensor(0x548c,0x70);
    write_cmos_sensor(0x548d,0x70);
    write_cmos_sensor(0x548e,0x72);
    write_cmos_sensor(0x548f,0x74);
    write_cmos_sensor(0x5490,0x75);
    write_cmos_sensor(0x5491,0x76);
    write_cmos_sensor(0x5492,0x75);
    write_cmos_sensor(0x5493,0x74);
    write_cmos_sensor(0x5494,0x71);
    write_cmos_sensor(0x5495,0x72);
    write_cmos_sensor(0x5496,0x71);
    write_cmos_sensor(0x5497,0x72);
    write_cmos_sensor(0x5498,0x74);
    write_cmos_sensor(0x5499,0x77);
    write_cmos_sensor(0x549a,0x79);
    write_cmos_sensor(0x549b,0x79);
    write_cmos_sensor(0x549c,0x79);
    write_cmos_sensor(0x549d,0x77);
    write_cmos_sensor(0x549e,0x74);
    write_cmos_sensor(0x549f,0x74);
    write_cmos_sensor(0x54a0,0x72);
    write_cmos_sensor(0x54a1,0x73);
    write_cmos_sensor(0x54a2,0x77);
    write_cmos_sensor(0x54a3,0x7c);
    write_cmos_sensor(0x54a4,0x7f);
    write_cmos_sensor(0x54a5,0x80);
    write_cmos_sensor(0x54a6,0x7f);
    write_cmos_sensor(0x54a7,0x7c);
    write_cmos_sensor(0x54a8,0x77);
    write_cmos_sensor(0x54a9,0x78);
    write_cmos_sensor(0x54aa,0x74);
    write_cmos_sensor(0x54ab,0x73);
    write_cmos_sensor(0x54ac,0x79);
    write_cmos_sensor(0x54ad,0x7f);
    write_cmos_sensor(0x54ae,0x83);
    write_cmos_sensor(0x54af,0x84);
    write_cmos_sensor(0x54b0,0x84);
    write_cmos_sensor(0x54b1,0x7f);
    write_cmos_sensor(0x54b2,0x7a);
    write_cmos_sensor(0x54b3,0x79);
    write_cmos_sensor(0x54b4,0x72);
    write_cmos_sensor(0x54b5,0x73);
    write_cmos_sensor(0x54b6,0x78);
    write_cmos_sensor(0x54b7,0x7f);
    write_cmos_sensor(0x54b8,0x83);
    write_cmos_sensor(0x54b9,0x84);
    write_cmos_sensor(0x54ba,0x83);
    write_cmos_sensor(0x54bb,0x7f);
    write_cmos_sensor(0x54bc,0x7a);
    write_cmos_sensor(0x54bd,0x79);
    write_cmos_sensor(0x54be,0x72);
    write_cmos_sensor(0x54bf,0x72);
    write_cmos_sensor(0x54c0,0x76);
    write_cmos_sensor(0x54c1,0x7b);
    write_cmos_sensor(0x54c2,0x7e);
    write_cmos_sensor(0x54c3,0x7f);
    write_cmos_sensor(0x54c4,0x7f);
    write_cmos_sensor(0x54c5,0x7b);
    write_cmos_sensor(0x54c6,0x77);
    write_cmos_sensor(0x54c7,0x77);
    write_cmos_sensor(0x54c8,0x71);
    write_cmos_sensor(0x54c9,0x71);
    write_cmos_sensor(0x54ca,0x74);
    write_cmos_sensor(0x54cb,0x77);
    write_cmos_sensor(0x54cc,0x79);
    write_cmos_sensor(0x54cd,0x7a);
    write_cmos_sensor(0x54ce,0x7a);
    write_cmos_sensor(0x54cf,0x77);
    write_cmos_sensor(0x54d0,0x75);
    write_cmos_sensor(0x54d1,0x74);
    write_cmos_sensor(0x54d2,0x71);
    write_cmos_sensor(0x54d3,0x70);
    write_cmos_sensor(0x54d4,0x72);
    write_cmos_sensor(0x54d5,0x74);
    write_cmos_sensor(0x54d6,0x76);
    write_cmos_sensor(0x54d7,0x76);
    write_cmos_sensor(0x54d8,0x76);
    write_cmos_sensor(0x54d9,0x75);
    write_cmos_sensor(0x54da,0x71);
    write_cmos_sensor(0x54db,0x73);
    write_cmos_sensor(0x54dc,0x72);
    write_cmos_sensor(0x54dd,0x6f);
    write_cmos_sensor(0x54de,0x70);
    write_cmos_sensor(0x54df,0x71);
    write_cmos_sensor(0x54e0,0x71);
    write_cmos_sensor(0x54e1,0x72);
    write_cmos_sensor(0x54e2,0x72);
    write_cmos_sensor(0x54e3,0x71);
    write_cmos_sensor(0x54e4,0x71);
    write_cmos_sensor(0x54e5,0x72);
    write_cmos_sensor(0x54e6,0x6b);
    write_cmos_sensor(0x54e7,0x70);
    write_cmos_sensor(0x54e8,0x71);
    write_cmos_sensor(0x54e9,0x72);
    write_cmos_sensor(0x54ea,0x73);
    write_cmos_sensor(0x54eb,0x73);
    write_cmos_sensor(0x54ec,0x73);
    write_cmos_sensor(0x54ed,0x72);
    write_cmos_sensor(0x54ee,0x71);
    write_cmos_sensor(0x54ef,0x6c);
    write_cmos_sensor(0x54f0,0x8a);
    write_cmos_sensor(0x54f1,0x8f);
    write_cmos_sensor(0x54f2,0x8f);
    write_cmos_sensor(0x54f3,0x8e);
    write_cmos_sensor(0x54f4,0x90);
    write_cmos_sensor(0x54f5,0x8e);
    write_cmos_sensor(0x54f6,0x8f);
    write_cmos_sensor(0x54f7,0x8f);
    write_cmos_sensor(0x54f8,0x8e);
    write_cmos_sensor(0x54f9,0x90);
    write_cmos_sensor(0x54fa,0x8a);
    write_cmos_sensor(0x54fb,0x89);
    write_cmos_sensor(0x54fc,0x88);
    write_cmos_sensor(0x54fd,0x88);
    write_cmos_sensor(0x54fe,0x87);
    write_cmos_sensor(0x54ff,0x88);
    write_cmos_sensor(0x5500,0x89);
    write_cmos_sensor(0x5501,0x89);
    write_cmos_sensor(0x5502,0x8c);
    write_cmos_sensor(0x5503,0x8c);
    write_cmos_sensor(0x5504,0x87);
    write_cmos_sensor(0x5505,0x84);
    write_cmos_sensor(0x5506,0x83);
    write_cmos_sensor(0x5507,0x82);
    write_cmos_sensor(0x5508,0x82);
    write_cmos_sensor(0x5509,0x83);
    write_cmos_sensor(0x550a,0x83);
    write_cmos_sensor(0x550b,0x85);
    write_cmos_sensor(0x550c,0x87);
    write_cmos_sensor(0x550d,0x8c);
    write_cmos_sensor(0x550e,0x81);
    write_cmos_sensor(0x550f,0x80);
    write_cmos_sensor(0x5510,0x80);
    write_cmos_sensor(0x5511,0x7f);
    write_cmos_sensor(0x5512,0x7f);
    write_cmos_sensor(0x5513,0x80);
    write_cmos_sensor(0x5514,0x81);
    write_cmos_sensor(0x5515,0x82);
    write_cmos_sensor(0x5516,0x85);
    write_cmos_sensor(0x5517,0x86);
    write_cmos_sensor(0x5518,0x7f);
    write_cmos_sensor(0x5519,0x7e);
    write_cmos_sensor(0x551a,0x7e);
    write_cmos_sensor(0x551b,0x7f);
    write_cmos_sensor(0x551c,0x80);
    write_cmos_sensor(0x551d,0x81);
    write_cmos_sensor(0x551e,0x81);
    write_cmos_sensor(0x551f,0x82);
    write_cmos_sensor(0x5520,0x83);
    write_cmos_sensor(0x5521,0x86);
    write_cmos_sensor(0x5522,0x7d);
    write_cmos_sensor(0x5523,0x7c);
    write_cmos_sensor(0x5524,0x7d);
    write_cmos_sensor(0x5525,0x7f);
    write_cmos_sensor(0x5526,0x80);
    write_cmos_sensor(0x5527,0x81);
    write_cmos_sensor(0x5528,0x82);
    write_cmos_sensor(0x5529,0x82);
    write_cmos_sensor(0x552a,0x83);
    write_cmos_sensor(0x552b,0x85);
    write_cmos_sensor(0x552c,0x7d);
    write_cmos_sensor(0x552d,0x7c);
    write_cmos_sensor(0x552e,0x7d);
    write_cmos_sensor(0x552f,0x80);
    write_cmos_sensor(0x5530,0x81);
    write_cmos_sensor(0x5531,0x82);
    write_cmos_sensor(0x5532,0x82);
    write_cmos_sensor(0x5533,0x82);
    write_cmos_sensor(0x5534,0x82);
    write_cmos_sensor(0x5535,0x85);
    write_cmos_sensor(0x5536,0x7e);
    write_cmos_sensor(0x5537,0x7d);
    write_cmos_sensor(0x5538,0x7f);
    write_cmos_sensor(0x5539,0x80);
    write_cmos_sensor(0x553a,0x81);
    write_cmos_sensor(0x553b,0x81);
    write_cmos_sensor(0x553c,0x82);
    write_cmos_sensor(0x553d,0x82);
    write_cmos_sensor(0x553e,0x83);
    write_cmos_sensor(0x553f,0x84);
    write_cmos_sensor(0x5540,0x81);
    write_cmos_sensor(0x5541,0x80);
    write_cmos_sensor(0x5542,0x7f);
    write_cmos_sensor(0x5543,0x7f);
    write_cmos_sensor(0x5544,0x7f);
    write_cmos_sensor(0x5545,0x80);
    write_cmos_sensor(0x5546,0x81);
    write_cmos_sensor(0x5547,0x82);
    write_cmos_sensor(0x5548,0x83);
    write_cmos_sensor(0x5549,0x86);
    write_cmos_sensor(0x554a,0x87);
    write_cmos_sensor(0x554b,0x84);
    write_cmos_sensor(0x554c,0x82);
    write_cmos_sensor(0x554d,0x81);
    write_cmos_sensor(0x554e,0x81);
    write_cmos_sensor(0x554f,0x82);
    write_cmos_sensor(0x5550,0x82);
    write_cmos_sensor(0x5551,0x83);
    write_cmos_sensor(0x5552,0x85);
    write_cmos_sensor(0x5553,0x88);
    write_cmos_sensor(0x5554,0x8a);
write_cmos_sensor(0x5555,0x89);
write_cmos_sensor(0x5556,0x88);
write_cmos_sensor(0x5557,0x87);
write_cmos_sensor(0x5558,0x86);
write_cmos_sensor(0x5559,0x87);
write_cmos_sensor(0x555a,0x87);
write_cmos_sensor(0x555b,0x88);
write_cmos_sensor(0x555c,0x89);
write_cmos_sensor(0x555d,0x8b);
write_cmos_sensor(0x555e,0x88);
write_cmos_sensor(0x555f,0x8e);
write_cmos_sensor(0x5560,0x8c);
write_cmos_sensor(0x5561,0x8d);
write_cmos_sensor(0x5562,0x8e);
write_cmos_sensor(0x5563,0x8c);
write_cmos_sensor(0x5564,0x8c);
write_cmos_sensor(0x5565,0x8b);
write_cmos_sensor(0x5566,0x8d);
write_cmos_sensor(0x5567,0x89);
write_cmos_sensor(0x5570,0x54);
write_cmos_sensor(0x5940,0x50);
write_cmos_sensor(0x5941,0x04);
write_cmos_sensor(0x5942,0x08);
write_cmos_sensor(0x5943,0x10);
write_cmos_sensor(0x5944,0x18);
write_cmos_sensor(0x5945,0x30);
write_cmos_sensor(0x5946,0x40);
write_cmos_sensor(0x5947,0x80);
write_cmos_sensor(0x5948,0xf0);
write_cmos_sensor(0x5949,0x10);
write_cmos_sensor(0x594a,0x08);
write_cmos_sensor(0x594b,0x18);
write_cmos_sensor(0x5b80,0x04);
write_cmos_sensor(0x5b81,0x0a);
write_cmos_sensor(0x5b82,0x18);
write_cmos_sensor(0x5b83,0x20);
write_cmos_sensor(0x5b84,0x30);
write_cmos_sensor(0x5b85,0x44);
write_cmos_sensor(0x5b86,0x10);
write_cmos_sensor(0x5b87,0x12);
write_cmos_sensor(0x5b88,0x14);
write_cmos_sensor(0x5b89,0x15);
write_cmos_sensor(0x5b8a,0x16);
write_cmos_sensor(0x5b8b,0x18);
write_cmos_sensor(0x5b8c,0x10);
write_cmos_sensor(0x5b8d,0x12);
write_cmos_sensor(0x5b8e,0x14);
write_cmos_sensor(0x5b8f,0x15);
write_cmos_sensor(0x5b90,0x16);
write_cmos_sensor(0x5b91,0x18);
}	/*	preview_setting  */

static void capture_setting(kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d\n",currefps);
    preview_setting();
}

static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d\n",currefps);
	preview_setting();
}
static void hs_video_setting(void)
{
	preview_setting();
}

static void slim_video_setting(void)
{
	//5.1.2hs_video 1472x832 30fps 24M MCLK 4lane 160Mbps/lane
	LOG_INF("E\n");
	preview_setting();
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
    LOG_INF("enable: %d\n", enable);
    if(enable) 
	{   // enable color bar
		//test_pattern_flag=TRUE;
		write_cmos_sensor(0x4709,0x01);
    } 
	else 
	{   
		//test_pattern_flag=FALSE;
        write_cmos_sensor(0x4709,0x00); // disable color bar test pattern
    }
    spin_lock(&imgsensor_drv_lock);
    imgsensor.test_pattern = enable;
    spin_unlock(&imgsensor_drv_lock);
	LOG_INF("[OV9732MIPI] exit OV9732MIPISetTestPatternMode function \n");
    return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*	get_imgsensor_id
*
* DESCRIPTION
*	This function get the sensor ID 
*
* PARAMETERS
*	*sensorID : return the sensor ID 
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 get_imgsensor_id(UINT32 *sensor_id) 
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	//sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
            *sensor_id = return_sensor_id();
			printk("Time:%s, ov9732 read ID is :0x%x, original ID is :0x%x\n",__TIME__,*sensor_id,imgsensor_info.sensor_id);
			if (*sensor_id == imgsensor_info.sensor_id) {				
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);	  
				return ERROR_NONE;
			}	
            printk("ov9732 Read sensor id fail, write id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
			retry--;
		} while(retry > 0);
		i++;
		retry = 2;
	}
	if (*sensor_id != imgsensor_info.sensor_id) {
		// if Sensor ID is not correct, Must set *sensor_id to 0xFFFFFFFF 
		*sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*	open
*
* DESCRIPTION
*	This function initialize the registers of CMOS sensor
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 open(void)
{
	//const kal_uint8 i2c_addr[] = {IMGSENSOR_WRITE_ID_1, IMGSENSOR_WRITE_ID_2};
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	kal_uint32 sensor_id = 0; 
	//LOG_1;	
	//LOG_2;
	
	//sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
            sensor_id = return_sensor_id();
			if (sensor_id == imgsensor_info.sensor_id) {				
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);	  
				break;
			}	
            printk("Read sensor id fail, write id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
			retry--;
		} while(retry > 0);
		i++;
		if (sensor_id == imgsensor_info.sensor_id)
			break;
		retry = 2;
	}
	if (imgsensor_info.sensor_id != sensor_id)
		return ERROR_SENSOR_CONNECT_FAIL;
	
	/* initail sequence write in  */
	sensor_init();

	spin_lock(&imgsensor_drv_lock);

	imgsensor.autoflicker_en= KAL_FALSE;
	imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
	imgsensor.shutter = 0x3D0;
	imgsensor.gain = 0x100;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.dummy_pixel = 0;
	imgsensor.dummy_line = 0;
	imgsensor.ihdr_en = KAL_FALSE;
	imgsensor.test_pattern = KAL_FALSE;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}	/*	open  */



/*************************************************************************
* FUNCTION
*	close
*
* DESCRIPTION
*	
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 close(void)
{
	LOG_INF("E\n");

	/*No Need to implement this function*/ 
	
	return ERROR_NONE;
}	/*	close  */


/*************************************************************************
* FUNCTION
* preview
*
* DESCRIPTION
*	This function start the sensor preview.
*
* PARAMETERS
*	*image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	//imgsensor.video_mode = KAL_FALSE;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.frame_length = imgsensor_info.pre.framelength; 
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	preview_setting();

 //modified by xen 2010506
	set_mirror_flip(imgsensor.mirror);//set_mirror_flip(IMAGE_NORMAL);

	return ERROR_NONE;
}	/*	preview   */

/*************************************************************************
* FUNCTION
*	capture
*
* DESCRIPTION
*	This function setup the CMOS sensor in capture MY_OUTPUT mode
*
* PARAMETERS
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                          MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    LOG_INF("E\n");
	
    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
    if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {//PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M
        imgsensor.pclk = imgsensor_info.cap1.pclk;
        imgsensor.line_length = imgsensor_info.cap1.linelength;
        imgsensor.frame_length = imgsensor_info.cap1.framelength;
        imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
        imgsensor.autoflicker_en = KAL_FALSE;
    } else {
        if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
            LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",imgsensor.current_fps,imgsensor_info.cap.max_framerate/10);
        imgsensor.pclk = imgsensor_info.cap.pclk;
        imgsensor.line_length = imgsensor_info.cap.linelength;
        imgsensor.frame_length = imgsensor_info.cap.framelength;
        imgsensor.min_frame_length = imgsensor_info.cap.framelength;
        imgsensor.autoflicker_en = KAL_FALSE;
    }
    spin_unlock(&imgsensor_drv_lock);
    capture_setting(imgsensor.current_fps);
	if(imgsensor.test_pattern == KAL_TRUE)
	{
		set_test_pattern_mode(TRUE);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.test_pattern = KAL_FALSE;
		spin_unlock(&imgsensor_drv_lock);
	}

 //modified by xen 2010506
	set_mirror_flip(imgsensor.mirror);//set_mirror_flip(IMAGE_NORMAL);

    return ERROR_NONE;
}    /* capture() */
static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");
	
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;  
	imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
	//imgsensor.current_fps = 300;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	normal_video_setting(imgsensor.current_fps);

 //modified by xen 2010506
	set_mirror_flip(imgsensor.mirror);//set_mirror_flip(IMAGE_NORMAL);

	return ERROR_NONE;
}	/*	normal_video   */

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");
	
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	imgsensor.pclk = imgsensor_info.hs_video.pclk;
	//imgsensor.video_mode = KAL_TRUE;
	imgsensor.line_length = imgsensor_info.hs_video.linelength;
	imgsensor.frame_length = imgsensor_info.hs_video.framelength; 
	imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	//imgsensor.current_fps = 300;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	hs_video_setting();

 //modified by xen 2010506
	set_mirror_flip(imgsensor.mirror);//set_mirror_flip(IMAGE_NORMAL);

	return ERROR_NONE;
}	/*	hs_video   */

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");
	
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	imgsensor.pclk = imgsensor_info.slim_video.pclk;
	//imgsensor.video_mode = KAL_TRUE;
	imgsensor.line_length = imgsensor_info.slim_video.linelength;
	imgsensor.frame_length = imgsensor_info.slim_video.framelength; 
	imgsensor.min_frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	//imgsensor.current_fps = 300;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	slim_video_setting();

 //modified by xen 2010506
	set_mirror_flip(imgsensor.mirror);//set_mirror_flip(IMAGE_NORMAL);

	return ERROR_NONE;
}	/*	slim_video	 */



static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
	LOG_INF("E\n");
	sensor_resolution->SensorFullWidth = imgsensor_info.cap.grabwindow_width;
	sensor_resolution->SensorFullHeight = imgsensor_info.cap.grabwindow_height;
	
	sensor_resolution->SensorPreviewWidth = imgsensor_info.pre.grabwindow_width;
	sensor_resolution->SensorPreviewHeight = imgsensor_info.pre.grabwindow_height;

	sensor_resolution->SensorVideoWidth = imgsensor_info.normal_video.grabwindow_width;
	sensor_resolution->SensorVideoHeight = imgsensor_info.normal_video.grabwindow_height;		

	
	sensor_resolution->SensorHighSpeedVideoWidth	 = imgsensor_info.hs_video.grabwindow_width;
	sensor_resolution->SensorHighSpeedVideoHeight	 = imgsensor_info.hs_video.grabwindow_height;
	
	sensor_resolution->SensorSlimVideoWidth	 = imgsensor_info.slim_video.grabwindow_width;
	sensor_resolution->SensorSlimVideoHeight	 = imgsensor_info.slim_video.grabwindow_height;
	return ERROR_NONE;
}	/*	get_resolution	*/

static kal_uint32 get_info(MSDK_SCENARIO_ID_ENUM scenario_id,
					  MSDK_SENSOR_INFO_STRUCT *sensor_info,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	
	//sensor_info->SensorVideoFrameRate = imgsensor_info.normal_video.max_framerate/10; /* not use */
	//sensor_info->SensorStillCaptureFrameRate= imgsensor_info.cap.max_framerate/10; /* not use */
	//imgsensor_info->SensorWebCamCaptureFrameRate= imgsensor_info.v.max_framerate; /* not use */

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW; /* not use */
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW; // inverse with datasheet
	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4; /* not use */
	sensor_info->SensorResetActiveHigh = FALSE; /* not use */
	sensor_info->SensorResetDelayCount = 5; /* not use */

	sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;
	sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
	sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
	sensor_info->SensorOutputDataFormat = imgsensor_info.sensor_output_dataformat;

	sensor_info->CaptureDelayFrame = imgsensor_info.cap_delay_frame; 
	sensor_info->PreviewDelayFrame = imgsensor_info.pre_delay_frame; 
	sensor_info->VideoDelayFrame = imgsensor_info.video_delay_frame;
	sensor_info->HighSpeedVideoDelayFrame = imgsensor_info.hs_video_delay_frame;
	sensor_info->SlimVideoDelayFrame = imgsensor_info.slim_video_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0; /* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;
	
	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame; 		 /* The frame of setting shutter default 0 for TG int */
	sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;	/* The frame of setting sensor gain */
	sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;	
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;
	
	sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num; 
	sensor_info->SensorClockFreq = imgsensor_info.mclk;
	sensor_info->SensorClockDividCount = 3; /* not use */
	sensor_info->SensorClockRisingCount = 0;
	sensor_info->SensorClockFallingCount = 2; /* not use */
	sensor_info->SensorPixelClockCount = 3; /* not use */
	sensor_info->SensorDataLatchCount = 2; /* not use */
	
	sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->SensorWidthSampling = 0;  // 0 is default 1x
	sensor_info->SensorHightSampling = 0;	// 0 is default 1x 
	sensor_info->SensorPacketECCOrder = 1;

	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			sensor_info->SensorGrabStartX = imgsensor_info.pre.startx; 
			sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;		
			
			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
			
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			sensor_info->SensorGrabStartX = imgsensor_info.cap.startx; 
			sensor_info->SensorGrabStartY = imgsensor_info.cap.starty;
				  
			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.cap.mipi_data_lp2hs_settle_dc; 

			break;	 
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			
			sensor_info->SensorGrabStartX = imgsensor_info.normal_video.startx; 
			sensor_info->SensorGrabStartY = imgsensor_info.normal_video.starty;
	   
			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.normal_video.mipi_data_lp2hs_settle_dc; 

			break;	  
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:			
			sensor_info->SensorGrabStartX = imgsensor_info.hs_video.startx; 
			sensor_info->SensorGrabStartY = imgsensor_info.hs_video.starty;
				  
			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.hs_video.mipi_data_lp2hs_settle_dc; 

			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			sensor_info->SensorGrabStartX = imgsensor_info.slim_video.startx; 
			sensor_info->SensorGrabStartY = imgsensor_info.slim_video.starty;
				  
			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.slim_video.mipi_data_lp2hs_settle_dc; 

            break;
		default:			
			sensor_info->SensorGrabStartX = imgsensor_info.pre.startx; 
			sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;		
			
			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
			break;
	}
	
	return ERROR_NONE;
}	/*	get_info  */


static kal_uint32 control(MSDK_SCENARIO_ID_ENUM scenario_id, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.current_scenario_id = scenario_id;
	spin_unlock(&imgsensor_drv_lock);
	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			preview(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			capture(image_window, sensor_config_data);
			break;	
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			normal_video(image_window, sensor_config_data);
			break;	  
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			hs_video(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			slim_video(image_window, sensor_config_data);
			break;	  
		default:
			LOG_INF("Error ScenarioId setting");
			preview(image_window, sensor_config_data);
			return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}	/* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{
	LOG_INF("framerate = %d\n ", framerate);
	// SetVideoMode Function should fix framerate
	if (framerate == 0)
		// Dynamic frame rate
		return ERROR_NONE;
	spin_lock(&imgsensor_drv_lock);
	if ((framerate == 300) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 296;
	else if ((framerate == 150) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 146;
	else
		imgsensor.current_fps = framerate;
	spin_unlock(&imgsensor_drv_lock);
	set_max_framerate(imgsensor.current_fps,1);
	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(kal_bool enable, UINT16 framerate)
{
	LOG_INF("enable = %d, framerate = %d \n", enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable) //enable auto flicker	  
		imgsensor.autoflicker_en = KAL_TRUE;
	else //Cancel Auto flick
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}


static kal_uint32 set_max_framerate_by_scenario(MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate) 
{
	kal_uint32 frame_length; 
	LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength):0;
			imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			set_dummy();			
			break;			
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			if(framerate == 0)
				return ERROR_NONE;
			frame_length = imgsensor_info.normal_video.pclk / framerate * 10 / imgsensor_info.normal_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.normal_video.framelength) ? (frame_length - imgsensor_info.normal_video.framelength):0;			
			imgsensor.frame_length = imgsensor_info.normal_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			set_dummy();			
			break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        	  if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {
                frame_length = imgsensor_info.cap1.pclk / framerate * 10 / imgsensor_info.cap1.linelength;
                spin_lock(&imgsensor_drv_lock);
		            imgsensor.dummy_line = (frame_length > imgsensor_info.cap1.framelength) ? (frame_length - imgsensor_info.cap1.framelength) : 0;
		            imgsensor.frame_length = imgsensor_info.cap1.framelength + imgsensor.dummy_line;
		            imgsensor.min_frame_length = imgsensor.frame_length;
		            spin_unlock(&imgsensor_drv_lock);
            } else {
        		    if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
                    LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",framerate,imgsensor_info.cap.max_framerate/10);
                frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
                spin_lock(&imgsensor_drv_lock);
		            imgsensor.dummy_line = (frame_length > imgsensor_info.cap.framelength) ? (frame_length - imgsensor_info.cap.framelength) : 0;
		            imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
		            imgsensor.min_frame_length = imgsensor.frame_length;
		            spin_unlock(&imgsensor_drv_lock);
            }
			set_dummy();			
			break;	
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.hs_video.framelength) ? (frame_length - imgsensor_info.hs_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			set_dummy();			
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ? (frame_length - imgsensor_info.slim_video.framelength): 0;
			imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			set_dummy();	
			break;		
		default:  //coding with  preview scenario by default
			frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			set_dummy();	
			LOG_INF("error scenario_id = %d, we use preview scenario \n", scenario_id);
			break;
	}	
	return ERROR_NONE;
}


static kal_uint32 get_default_framerate_by_scenario(MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate) 
{
	//LOG_INF("scenario_id = %d, framerate=%x\n", scenario_id, framerate);

	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			*framerate = imgsensor_info.pre.max_framerate;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*framerate = imgsensor_info.normal_video.max_framerate;
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*framerate = imgsensor_info.cap.max_framerate;
			break;		
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*framerate = imgsensor_info.hs_video.max_framerate;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO: 
			*framerate = imgsensor_info.slim_video.max_framerate;
			break;
		default:
			*framerate = imgsensor_info.pre.max_framerate;
			break;
	}

    return ERROR_NONE;
}

static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
							 UINT8 *feature_para,UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16=(UINT16 *) feature_para;
	UINT16 *feature_data_16=(UINT16 *) feature_para;
	UINT32 *feature_return_para_32=(UINT32 *) feature_para;
	UINT32 *feature_data_32=(UINT32 *) feature_para;
    unsigned long long *feature_data=(unsigned long long *)feature_para;
	
	SENSOR_WINSIZE_INFO_STRUCT *wininfo;	
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data=(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;
 
	//LOG_INF("feature_id = %d, feature_data=%x, para_len=%x\n", feature_id, feature_data, *feature_para_len);
	switch (feature_id) {
		case SENSOR_FEATURE_GET_PERIOD:
			*feature_return_para_16++ = imgsensor.line_length;
			*feature_return_para_16 = imgsensor.frame_length;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:	 
			LOG_INF("feature_Control imgsensor.pclk = %d,imgsensor.current_fps = %d\n", imgsensor.pclk,imgsensor.current_fps);
			*feature_return_para_32 = imgsensor.pclk;
			*feature_para_len=4;
			break;		   
		case SENSOR_FEATURE_SET_ESHUTTER:
                      set_shutter(*feature_data);
			break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
            night_mode((BOOL) *feature_data);
			break;
		case SENSOR_FEATURE_SET_GAIN:		
                     set_gain((UINT16) *feature_data);
			break;
		case SENSOR_FEATURE_SET_FLASHLIGHT:
			break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			break;
		case SENSOR_FEATURE_SET_REGISTER:
			write_cmos_sensor(sensor_reg_data->RegAddr, sensor_reg_data->RegData);
			break;
		case SENSOR_FEATURE_GET_REGISTER:
			sensor_reg_data->RegData = read_cmos_sensor(sensor_reg_data->RegAddr);
			break;
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
			// if EEPROM does not exist in camera module.
			*feature_return_para_32=LENS_DRIVER_ID_DO_NOT_CARE;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_SET_VIDEO_MODE:
                       set_video_mode(*feature_data);
			break; 
		case SENSOR_FEATURE_CHECK_SENSOR_ID:
			get_imgsensor_id(feature_return_para_32); 
			break; 
		case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
			set_auto_flicker_mode((BOOL)*feature_data_16,*(feature_data_16+1));
			break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
            set_max_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM)*feature_data, *(feature_data+1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
            get_default_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM)*(feature_data), (MUINT32 *)(uintptr_t)(*(feature_data+1)));
			break;
		case SENSOR_FEATURE_SET_TEST_PATTERN:
            set_test_pattern_mode((BOOL)*feature_data);
			break;
		case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE: //for factory mode auto testing			 
			*feature_return_para_32 = imgsensor_info.checksum_value;
			*feature_para_len=4;							 
			break;				
		case SENSOR_FEATURE_SET_FRAMERATE:
            LOG_INF("current fps :%d\n", (UINT32)*feature_data);
			spin_lock(&imgsensor_drv_lock);
            imgsensor.current_fps = *feature_data;
			spin_unlock(&imgsensor_drv_lock);		
			break;
		case SENSOR_FEATURE_SET_HDR:
            LOG_INF("ihdr enable :%d\n", (BOOL)*feature_data);
			spin_lock(&imgsensor_drv_lock);
            imgsensor.ihdr_en = (BOOL)*feature_data;
			spin_unlock(&imgsensor_drv_lock);		
			break;
		case SENSOR_FEATURE_GET_CROP_INFO:
            LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n", (UINT32)*feature_data);
            wininfo = (SENSOR_WINSIZE_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));
		
			switch (*feature_data_32) {
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[1],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;	  
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[2],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[3],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_SLIM_VIDEO:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[4],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
				default:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[0],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
			}
            break;
		case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
			LOG_INF("SENSOR_SET_SENSOR_IHDR is no support");
			//LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",(UINT16)*feature_data_32,(UINT16)*(feature_data_32+1),(UINT16)*(feature_data_32+2)); 
			//ihdr_write_shutter_gain((UINT16)*feature_data_32,(UINT16)*(feature_data_32+1),(UINT16)*(feature_data_32+2));	
			break;
		default:
			break;
	}
  
	return ERROR_NONE;
}	/*	feature_control()  */

static SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 OV9732MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&sensor_func;
	return ERROR_NONE;
}	/*	OV5693_MIPI_RAW_SensorInit	*/
