/*
 *
 * FocalTech TouchScreen driver.
 * 
 * Copyright (c) 2010-2015, Focaltech Ltd. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

 /*******************************************************************************
*
* File Name: focaltech_core.c
*
*    Author: Tsai HsiangYu
*
*   Created: 2015-03-02
*
*  Abstract:
*
* Reference:
*
*******************************************************************************/

/*******************************************************************************
* 1.Included header files
*******************************************************************************/
/*
//user defined include header files
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/earlysuspend.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/input/mt.h>
#include <linux/switch.h>
#include <linux/gpio.h>
#include "focaltech_core.h"
#include "tpd.h"
#include "tpd_custom_fts.h"
*/
 
#include "focaltech_core.h"

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/dma-mapping.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
#include "mt_boot_common.h"
#include <linux/of.h>
#include <linux/of_irq.h>
#else
#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot.h>
#include <cust_eint.h>
#include "cust_gpio_usage.h"
#endif
//#include "focaltech_core.h"
//+add by hzb for dsm
#ifdef CONFIG_ONTIM_DSM	

#include <ontim/ontim_dsm.h>

struct dsm_dev focaltech_dsm_dev=
{
	.type=OMTIM_DSM_DEV_TYPE_IO,
	.id=OMTIM_DSM_DEV_ID_TP,
	.name="Focaltech Ft6336s TP",
	.buff_size=1024,
};
static struct dsm_client *focaltech_dsm_client=NULL;
#endif
//-add by hzb for dsm

/*******************************************************************************
* 2.Private constant and macro definitions using #define
*******************************************************************************/
/*register define*/
#define FTS_RESET_PIN										GPIO_CTP_RST_PIN
#define TPD_OK 												0
#define DEVICE_MODE 										0x00
#define GEST_ID 												0x01
#define TD_STATUS 											0x02
#define TOUCH1_XH 											0x03
#define TOUCH1_XL 											0x04
#define TOUCH1_YH 											0x05
#define TOUCH1_YL 											0x06
#define TOUCH2_XH 											0x09
#define TOUCH2_XL 											0x0A
#define TOUCH2_YH 											0x0B
#define TOUCH2_YL 											0x0C
#define TOUCH3_XH 											0x0F
#define TOUCH3_XL 											0x10
#define TOUCH3_YH 											0x11
#define TOUCH3_YL 											0x12
#define TPD_MAX_RESET_COUNT 								3


//+add by hzb
//#include <ontim/ontim_dev_dgb.h>

//static char ft6336s_version[]="ft6336s ontim ver 1.0";
static char ft6336s_vendor_name[50]="ofilm-ft6336s";
//static u32 ft6336s_wakeup_support=0;
static char ft6336s_wakeup_enable=0;
//+add by hzb for dsm
#ifdef CONFIG_ONTIM_DSM	
static u32  ft6336s_irq_count=0;
static u32  ft6336s_irq_run_count=0;
#endif
//-add by hzb for dsm

//DEV_ATTR_DECLARE(touch_screen)
//DEV_ATTR_DEFINE("version",ft6336s_version)
//DEV_ATTR_DEFINE("vendor",ft6336s_vendor_name)
//DEV_ATTR_VAL_DEFINE("wakeup_support",&ft6336s_wakeup_support,ONTIM_DEV_ARTTR_TYPE_VAL_RO)
//DEV_ATTR_VAL_DEFINE("wakeup_enable",&ft6336s_wakeup_enable,ONTIM_DEV_ARTTR_TYPE_VAL_8BIT)
//+add by hzb for dsm
#ifdef CONFIG_ONTIM_DSM	
DEV_ATTR_VAL_DEFINE("irq_count",&ft6336s_irq_count,ONTIM_DEV_ARTTR_TYPE_VAL_RO)
DEV_ATTR_VAL_DEFINE("irq_run_count",&ft6336s_irq_run_count,ONTIM_DEV_ARTTR_TYPE_VAL_RO)
#endif
//-add by hzb for dsm
//DEV_ATTR_DECLARE_END;
//ONTIM_DEBUG_DECLARE_AND_INIT(touch_screen,touch_screen,8);
//-add by hzb

extern bool tp_probe_ok;//add by liuwei
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
#define CTP_RST    0
#define CTP_INT    1

unsigned int touch_irq = 0;
#endif
/*
#define TPD_PROXIMITY
				
*/
#define FTS_CTL_IIC
#define SYSFS_DEBUG
#define FTS_APK_DEBUG

#define REPORT_TOUCH_DEBUG  								0
/*
for tp esd check
*/

#if FT_ESD_PROTECT
	#define TPD_ESD_CHECK_CIRCLE        						200
	static struct delayed_work gtp_esd_check_work;
	static struct workqueue_struct *gtp_esd_check_workqueue 	= NULL;
	static int count_irq 										= 0;
	static u8 run_check_91_register 							= 0;
	static unsigned long esd_check_circle 						= TPD_ESD_CHECK_CIRCLE;
	static void gtp_esd_check_func(struct work_struct *);
	
	
	
#endif

//#if FT_ESD_PROTECT
int apk_debug_flag=0;
//#endif
/* PROXIMITY */
#ifdef TPD_PROXIMITY
	#include <linux/hwmsensor.h>
	#include <linux/hwmsen_dev.h>
	#include <linux/sensors_io.h>
#endif

#ifdef TPD_PROXIMITY
	#define APS_ERR(fmt,arg...)           							printk("<<proximity>> "fmt"\n",##arg)
	#define TPD_PROXIMITY_DEBUG(fmt,arg...) 					printk("<<proximity>> "fmt"\n",##arg)
	#define TPD_PROXIMITY_DMESG(fmt,arg...) 					printk("<<proximity>> "fmt"\n",##arg)
	static u8 tpd_proximity_flag 								= 0;	
	static u8 tpd_proximity_flag_one 							= 0; // add for tpd_proximity by wangdongfang	
	static u8 tpd_proximity_detect 							= 1;	// 0-->close ; 1--> far away
#endif
/*dma declare, allocate and release*/
#define __MSG_DMA_MODE__
#ifdef __MSG_DMA_MODE__
	u8 *g_dma_buff_va = NULL;
	dma_addr_t g_dma_buff_pa = 0;   
#endif

#ifdef __MSG_DMA_MODE__

	static void msg_dma_alloct(void)
	{
		g_dma_buff_va = (u8 *)dma_alloc_coherent(NULL, 128, &g_dma_buff_pa, GFP_KERNEL);	// DMA size 4096 for customer
	    	if(!g_dma_buff_va)
		{
	        	TPD_DMESG("[DMA][Error] Allocate DMA I2C Buffer failed!\n");
	    	}
	}
	static void msg_dma_release(void){
		if(g_dma_buff_va)
		{
	     		dma_free_coherent(NULL, 128, g_dma_buff_va, g_dma_buff_pa);
	        	g_dma_buff_va = NULL;
	        	g_dma_buff_pa = 0;
			TPD_DMESG("[DMA][release] Allocate DMA I2C Buffer release!\n");
	    	}
	}
#endif
#ifdef TPD_HAVE_BUTTON 
	static int tpd_keys_local[TPD_KEY_COUNT] 			= TPD_KEYS;
	static int tpd_keys_dim_local[TPD_KEY_COUNT][4] 	= TPD_KEYS_DIM;
#endif
#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
	static int tpd_wb_start_local[TPD_WARP_CNT] 		= TPD_WARP_START;
	static int tpd_wb_end_local[TPD_WARP_CNT]   		= TPD_WARP_END;
#endif
#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
	static int tpd_calmat_local[8]     					= TPD_CALIBRATION_MATRIX;
	static int tpd_def_calmat_local[8] 					= TPD_CALIBRATION_MATRIX;
#endif
/*******************************************************************************
* 3.Private enumerations, structures and unions using typedef
*******************************************************************************/

/* touch info */
#if A_TYPE==0
struct touch_info {
    int y[10];
    int x[10];
    int p[10];
    int id[10];
    int pressure[10];
    int count;
};
#else
struct touch_info {
    int y[10];
    int x[10];
    int p[10];
    int id[10];
    int count;
};
#endif
/* register driver and device info */ 
static const struct i2c_device_id fts_tpd_id[] 		= {{"fts",0},{}};
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 18, 19))
static struct i2c_board_info __initdata fts_i2c_tpd	={ I2C_BOARD_INFO("fts", (0x70>>1))};
#endif
 


 
/*******************************************************************************
* 4.Static variables
*******************************************************************************/
struct i2c_client *fts_i2c_client 				= NULL;
 struct input_dev *fts_input_dev				=NULL;
struct task_struct *thread 						= NULL;
int up_flag									=0;
int up_count									=0;
static int tpd_flag 								= 0;
static int tpd_halt								=0;
//static int point_num 							= 0;
//static int total_point							=0;
//
//static u8 buf_addr[2] 							= { 0 };
//static u8 buf_value[2] 							= { 0 };
//static bool is_update = false;
/*******************************************************************************
* 5.Global variable or extern global variabls/functions
*******************************************************************************/



/*******************************************************************************
* 6.Static function prototypes
*******************************************************************************/







static DECLARE_WAIT_QUEUE_HEAD(waiter);
static DEFINE_MUTEX(i2c_access);
static DEFINE_MUTEX(i2c_rw_access);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 18, 19))
static void tpd_eint_interrupt_handler(void);
#endif
static int __devinit tpd_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tpd_detect (struct i2c_client *client, struct i2c_board_info *info);
static int __devexit tpd_remove(struct i2c_client *client);
static int touch_event_handler(void *unused);
extern void mt_eint_mask(unsigned int eint_num);
extern void mt_eint_unmask(unsigned int eint_num);
extern void mt_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
extern void mt_eint_set_polarity(unsigned int eint_num, unsigned int pol);
extern unsigned int mt_eint_set_sens(unsigned int eint_num, unsigned int sens);
extern void mt_eint_registration(unsigned int eint_num, unsigned int flow, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);



#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
static const struct of_device_id ft6336s_dt_match[] = {
	{.compatible = "mediatek,cap_touch1"},
	{},
};
MODULE_DEVICE_TABLE(of, ft6336s_dt_match);
#endif

static struct i2c_driver tpd_i2c_driver = {
  .driver 		= {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
	.of_match_table = of_match_ptr(ft6336s_dt_match),
#endif
  .name 		= "fts",
  // .owner 	= THIS_MODULE,
  },
  .probe 		= tpd_probe,
  .remove 	= __devexit_p(tpd_remove),
  .id_table 	= fts_tpd_id,
  .detect 		= tpd_detect,

 };



/*
* open/release/(I/O) control tpd device
*
*/
//#define VELOCITY_CUSTOM_fts
#ifdef VELOCITY_CUSTOM_fts
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>

/* for magnify velocity */
#ifndef TPD_VELOCITY_CUSTOM_X
	#define TPD_VELOCITY_CUSTOM_X 							10
#endif
#ifndef TPD_VELOCITY_CUSTOM_Y
	#define TPD_VELOCITY_CUSTOM_Y 							10
#endif

#define TOUCH_IOC_MAGIC 									'A'
#define TPD_GET_VELOCITY_CUSTOM_X 						_IO(TOUCH_IOC_MAGIC,0)
#define TPD_GET_VELOCITY_CUSTOM_Y 						_IO(TOUCH_IOC_MAGIC,1)

int g_v_magnify_x =TPD_VELOCITY_CUSTOM_X;
int g_v_magnify_y =TPD_VELOCITY_CUSTOM_Y;


/************************************************************************
* Name: tpd_misc_open
* Brief: open node
* Input: node, file point
* Output: no
* Return: fail <0
***********************************************************************/
static int tpd_misc_open(struct inode *inode, struct file *file)
{
	return nonseekable_open(inode, file);
}
/************************************************************************
* Name: tpd_misc_release
* Brief: release node
* Input: node, file point
* Output: no
* Return: 0
***********************************************************************/
static int tpd_misc_release(struct inode *inode, struct file *file)
{
	return 0;
}
/************************************************************************
* Name: tpd_unlocked_ioctl
* Brief: I/O control for apk
* Input: file point, command
* Output: no
* Return: fail <0
***********************************************************************/

static long tpd_unlocked_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)
{

	void __user *data;
	
	long err = 0;
	
	if(_IOC_DIR(cmd) & _IOC_READ)
	{
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	}
	else if(_IOC_DIR(cmd) & _IOC_WRITE)
	{
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	}

	if(err)
	{
		printk("tpd: access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}

	switch(cmd)
	{
		case TPD_GET_VELOCITY_CUSTOM_X:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}			
			
			if(copy_to_user(data, &g_v_magnify_x, sizeof(g_v_magnify_x)))
			{
				err = -EFAULT;
				break;
			}				 
			break;

	   case TPD_GET_VELOCITY_CUSTOM_Y:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}			
			
			if(copy_to_user(data, &g_v_magnify_y, sizeof(g_v_magnify_y)))
			{
				err = -EFAULT;
				break;
			}				 
			break;


		default:
			printk("tpd: unknown IOCTL: 0x%08x\n", cmd);
			err = -ENOIOCTLCMD;
			break;
			
	}

	return err;
}


static struct file_operations tpd_fops = {
	// .owner = THIS_MODULE,
	.open 			= tpd_misc_open,
	.release 			= tpd_misc_release,
	.unlocked_ioctl 	= tpd_unlocked_ioctl,
};

static struct miscdevice tpd_misc_device = {
	.minor 			= MISC_DYNAMIC_MINOR,
	.name 			= "touch",
	.fops 			= &tpd_fops,
};
#endif

 
/************************************************************************
* Name: fts_i2c_read
* Brief: i2c read
* Input: i2c info, write buf, write len, read buf, read len
* Output: get data in the 3rd buf
* Return: fail <0
***********************************************************************/
int fts_i2c_read(struct i2c_client *client, char *writebuf,int writelen, char *readbuf, int readlen)
{
	int ret=0;

	// for DMA I2c transfer
	
	mutex_lock(&i2c_rw_access);
	
	if((NULL!=client) && (writelen>0) && (writelen<=128))
	{
		// DMA Write
		memcpy(g_dma_buff_va, writebuf, writelen);
		client->addr = (client->addr & I2C_MASK_FLAG) | I2C_DMA_FLAG;
		if((ret=i2c_master_send(client, (unsigned char *)g_dma_buff_pa, writelen))!=writelen)
			//dev_err(&client->dev, "###%s i2c write len=%x,buffaddr=%x\n", __func__,ret,*g_dma_buff_pa);
			printk("i2c write failed\n");
		client->addr = client->addr & I2C_MASK_FLAG &(~ I2C_DMA_FLAG);
	}

	// DMA Read 

	if((NULL!=client) && (readlen>0) && (readlen<=128))

	{
		client->addr = (client->addr & I2C_MASK_FLAG) | I2C_DMA_FLAG;

		ret = i2c_master_recv(client, (unsigned char *)g_dma_buff_pa, readlen);

		memcpy(readbuf, g_dma_buff_va, readlen);

		client->addr = client->addr & I2C_MASK_FLAG &(~ I2C_DMA_FLAG);
	}
	
	mutex_unlock(&i2c_rw_access);
#ifdef CONFIG_ONTIM_DSM	
	if (tp_probe_ok && (ret<0))
	{
		int error=OMTIM_DSM_TP_READ_I2C_ERROR;
		int i=0;
	 	if ( (focaltech_dsm_client ) && dsm_client_ocuppy(focaltech_dsm_client))
	 	{
	 		if ((focaltech_dsm_client->dump_buff) && (focaltech_dsm_client->buff_size)&&(focaltech_dsm_client->buff_flag == OMTIM_DSM_BUFF_OK))
	 		{
				focaltech_dsm_client->used_size = sprintf(focaltech_dsm_client->dump_buff,"Type=%d; ID=%d; error_id=%d; CTP info:%s; read error = %d;{",focaltech_dsm_client->client_type,focaltech_dsm_client->client_id,error,ft6336s_vendor_name,ret );
				focaltech_dsm_client->used_size += sprintf(focaltech_dsm_client->dump_buff+focaltech_dsm_client->used_size,"Reg len=%d; ",writelen );
				if ( writelen && writebuf )
				{
					focaltech_dsm_client->used_size += sprintf(focaltech_dsm_client->dump_buff+focaltech_dsm_client->used_size,"Reg[" );
					for(i=0;i<writelen;i++)
					{
						focaltech_dsm_client->used_size += sprintf(focaltech_dsm_client->dump_buff+focaltech_dsm_client->used_size," 0x%x ",writebuf[i] );
					}
					focaltech_dsm_client->used_size += sprintf(focaltech_dsm_client->dump_buff+focaltech_dsm_client->used_size,"]" );
	 			}
				focaltech_dsm_client->used_size += sprintf(focaltech_dsm_client->dump_buff+focaltech_dsm_client->used_size,"Read len=%d}\n",readlen );
				dsm_client_notify(focaltech_dsm_client,error);
	 		}
	 	}
		else
		{
			printk(KERN_ERR "%s: dsm ocuppy error!!!",__func__);
		}
	}
#endif
	return ret;

	/*
	int ret,i;

	mutex_lock(&i2c_rw_access);
	if ((writelen > 0) && (readlen>=0) && (NULL!=client)) {
		struct i2c_msg msgs[] = {
			{
			 .addr = client->addr,
			 .flags = 0,
			 .len = writelen,
			 .buf = writebuf,
			 },
			{
			 .addr = client->addr,
			 .flags = I2C_M_RD,
			 .len = readlen,
			 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret < 0)
			dev_err(&client->dev, "f%s: i2c read error.\n",
				__func__);
	}
	else {
		if ((readlen>0) && (NULL!=client)) {
		struct i2c_msg msgs[] = {
			{
			 .addr = client->addr,
			 .flags = I2C_M_RD,
			 .len = readlen,
			 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret < 0)
			dev_err(&client->dev, "%s:i2c read error.\n", __func__);
		}
	}
	mutex_unlock(&i2c_rw_access);
	return ret;
	*/

}

/************************************************************************
* Name: fts_i2c_write
* Brief: i2c write
* Input: i2c info, write buf, write len
* Output: no
* Return: fail <0
***********************************************************************/
int fts_i2c_write(struct i2c_client *client, char *writebuf, int writelen)
{
	int ret=0;

	mutex_lock(&i2c_rw_access);
	
 	//client->addr = client->addr & I2C_MASK_FLAG;

	//ret = i2c_master_send(client, writebuf, writelen);
	if((NULL!=client) && (writelen>0) && (writelen<=128))
	{
		memcpy(g_dma_buff_va, writebuf, writelen);
		
		client->addr = (client->addr & I2C_MASK_FLAG) | I2C_DMA_FLAG;
		if((ret=i2c_master_send(client, (unsigned char *)g_dma_buff_pa, writelen))!=writelen)
			//dev_err(&client->dev, "###%s i2c write len=%x,buffaddr=%x\n", __func__,ret,*g_dma_buff_pa);
			printk("i2c write failed\n");
		client->addr = client->addr & I2C_MASK_FLAG &(~ I2C_DMA_FLAG);
	}
	mutex_unlock(&i2c_rw_access);
#ifdef CONFIG_ONTIM_DSM	
	if (tp_probe_ok && (ret<0))
	{
		int error=OMTIM_DSM_TP_WRITE_I2C_ERROR;
		int i=0;
	 	if ( (focaltech_dsm_client ) && dsm_client_ocuppy(focaltech_dsm_client))
	 	{
	 		if ((focaltech_dsm_client->dump_buff) && (focaltech_dsm_client->buff_size)&&(focaltech_dsm_client->buff_flag == OMTIM_DSM_BUFF_OK))
	 		{
				focaltech_dsm_client->used_size = sprintf(focaltech_dsm_client->dump_buff,"Type=%d; ID=%d; error_id=%d; CTP info:%s; Write error = %d;{",focaltech_dsm_client->client_type,focaltech_dsm_client->client_id,error,ft6336s_vendor_name,ret );
				focaltech_dsm_client->used_size += sprintf(focaltech_dsm_client->dump_buff+focaltech_dsm_client->used_size,"Write len=%d; ",writelen );
				if ( writelen && writebuf )
				{
					focaltech_dsm_client->used_size += sprintf(focaltech_dsm_client->dump_buff+focaltech_dsm_client->used_size,"buff data [" );
					for(i=0;i<writelen;i++)
					{
						focaltech_dsm_client->used_size += sprintf(focaltech_dsm_client->dump_buff+focaltech_dsm_client->used_size," 0x%x ",writebuf[i] );
					}
					focaltech_dsm_client->used_size += sprintf(focaltech_dsm_client->dump_buff+focaltech_dsm_client->used_size,"]" );
	 			}
				focaltech_dsm_client->used_size += sprintf(focaltech_dsm_client->dump_buff+focaltech_dsm_client->used_size,"}\n" );
				dsm_client_notify(focaltech_dsm_client,error);
	 		}
	 	}
		else
		{
			printk(KERN_ERR "%s: dsm ocuppy error!!!",__func__);
		}
	}
#endif
	
	return ret;

	/*
	int ret;
	int i = 0;
	mutex_lock(&i2c_rw_access);
	if ((writelen > 0) && (NULL!=client)) {
   	client->addr = client->addr & I2C_MASK_FLAG;


	struct i2c_msg msg[] = {
		{
		 .addr = client->addr,
		 .flags = 0,
		 .len = writelen,
		 .buf = writebuf,
		 },
	};

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret < 0)
		dev_err(&client->dev, "%s i2c write error.\n", __func__);
	}
	mutex_unlock(&i2c_rw_access);
	return ret;
	*/

}
/************************************************************************
* Name: fts_write_reg
* Brief: write register
* Input: i2c info, reg address, reg value
* Output: no
* Return: fail <0
***********************************************************************/
int fts_write_reg(struct i2c_client *client, u8 regaddr, u8 regvalue)
{
	unsigned char buf[2] = {0};

	buf[0] = regaddr;
	buf[1] = regvalue;

	return fts_i2c_write(client, buf, sizeof(buf));
}
/************************************************************************
* Name: fts_read_reg
* Brief: read register
* Input: i2c info, reg address, reg value
* Output: get reg value
* Return: fail <0
***********************************************************************/
int fts_read_reg(struct i2c_client *client, u8 regaddr, u8 *regvalue)
{

	return fts_i2c_read(client, &regaddr, 1, regvalue, 1);

}
/************************************************************************
* Name: tpd_down
* Brief: down info
* Input: x pos, y pos, id number
* Output: no
* Return: no
***********************************************************************/
#if A_TYPE==0
static int tpd_history_x=0, tpd_history_y=0;
static void tpd_down(int x, int y,int press, int id) 
{
	if ((!press) && (!id))
    {
        input_report_abs(tpd->dev, ABS_MT_PRESSURE, 100);
        input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 100);
    }
    else
    {
        input_report_abs(tpd->dev, ABS_MT_PRESSURE, press);
        input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, press);
        /* track id Start 0 */
        input_report_abs(tpd->dev, ABS_MT_TRACKING_ID, id);
    }
	
	input_report_key(tpd->dev, BTN_TOUCH, 1);
	
	input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
	input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);	
	input_mt_sync(tpd->dev);
    	TPD_DEBUG_SET_TIME;
	TPD_EM_PRINT(x, y, x, y, id, 1);
   	 tpd_history_x=x;
   	 tpd_history_y=y;
#ifdef TPD_HAVE_BUTTON
     	if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
     	{   
       	tpd_button(x, y, 1);  
     	}
#endif
	TPD_DOWN_DEBUG_TRACK(x,y);
 }
#elif A_TYPE==1
static void tpd_down(int x, int y, int p)
{
	
	if(x > TPD_RES_X)
	{
		TPD_DEBUG("warning: IC have sampled wrong value.\n");;
		return;
	}
	input_report_key(tpd->dev, BTN_TOUCH, 1);
	input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 20);
	input_report_abs(tpd->dev, ABS_MT_PRESSURE, 0x3f);
	input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
	input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
	//printk("tpd:D[%4d %4d %4d] ", x, y, p);
	// track id Start 0
     	//input_report_abs(tpd->dev, ABS_MT_TRACKING_ID, p); 
	input_mt_sync(tpd->dev);
     	if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
     	{   
       	tpd_button(x, y, 1);  
     	}
	if(y > TPD_RES_Y) 	// virtual key debounce to avoid android ANR issue
	{
       	//msleep(50);
		printk("D virtual key \n");
	 }
	 TPD_EM_PRINT(x, y, x, y, p-1, 1);
	 
 }
#elif A_TYPE==2
#if 0//no use hank mark
static void tpd_down(int x, int y, int p) {
	
	if(x > TPD_RES_X)
	{
		TPD_DEBUG("warning: IC have sampled wrong value.\n");;
		return;
	}
	input_report_key(tpd->dev, BTN_TOUCH, 1);
	input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 20);
	input_report_abs(tpd->dev, ABS_MT_PRESSURE, 0x3f);
	input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
	input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
	//printk("tpd:D[%4d %4d %4d] ", x, y, p);
	/* track id Start 0 */
     	//input_report_abs(tpd->dev, ABS_MT_TRACKING_ID, p); 
	input_mt_sync(tpd->dev);
     	if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
     	{   
       	tpd_button(x, y, 1);  
     	}
	if(y > TPD_RES_Y) 	// virtual key debounce to avoid android ANR issue
	{
       	//msleep(50);
		printk("D virtual key \n");
	 }
	 TPD_EM_PRINT(x, y, x, y, p-1, 1);
 }
#endif
#endif
 
 /************************************************************************
* Name: tpd_up
* Brief: up info
* Input: x pos, y pos, count
* Output: no
* Return: no
***********************************************************************/
#if A_TYPE==0
static  void tpd_up(int x, int y,int id)
{	 
	 input_report_key(tpd->dev, BTN_TOUCH, 0);
	 //printk("U[%4d %4d %4d] ", x, y, 0);
	 input_mt_sync(tpd->dev);
	TPD_DEBUG_SET_TIME;
	 TPD_EM_PRINT(tpd_history_x, tpd_history_y, tpd_history_x, tpd_history_y, id, 0);
	tpd_history_x=0;
    	tpd_history_y=0;
#ifdef TPD_HAVE_BUTTON
	if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
	{   
		tpd_button(x, y, 0); 
	}
#endif	
 }
#elif A_TYPE==1
static  void tpd_up(int x, int y,int *count)
{
	 
	 input_report_key(tpd->dev, BTN_TOUCH, 0);
	 //printk("U[%4d %4d %4d] ", x, y, 0);
	 input_mt_sync(tpd->dev);
	 TPD_EM_PRINT(x, y, x, y, 0, 0);

	if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
	{   
		tpd_button(x, y, 0); 
	}
	
 }
 #elif A_TYPE==2
#if 0//hank mark
 static  void tpd_up(int x, int y,int *count)
{
	 input_report_key(tpd->dev, BTN_TOUCH, 0);
	 //printk("U[%4d %4d %4d] ", x, y, 0);
	 input_mt_sync(tpd->dev);
	 TPD_EM_PRINT(x, y, x, y, 0, 0);

	if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
	{   
		tpd_button(x, y, 0); 
	}   		 
 }
#endif
#endif
 /************************************************************************
* Name: tpd_touchinfo
* Brief: touch info
* Input: touch info point, no use
* Output: no
* Return: success nonzero
***********************************************************************/
#if A_TYPE==0
static int tpd_touchinfo(struct touch_info *cinfo, struct touch_info *pinfo,struct touch_info *ptest)
{
	int i = 0;
	char data[128] = {0};
       u16 high_byte,low_byte,reg;
	u8 report_rate =0;
	u8 pointid = FTS_MAX_ID;
	if (tpd_halt)
	{
		TPD_DMESG( "tpd_touchinfo return ..\n");
		return false;
	}
	//mutex_lock(&i2c_access);

       reg = 0x00;
	fts_i2c_read(fts_i2c_client, &reg, 1, data, 64);
	//mutex_unlock(&i2c_access);
	
	#if REPORT_TOUCH_DEBUG	
		for(i=0;i<64;i++)
		{
			printk("\n [fts] zax buf[%d] =(0x%02x)  \n", i,data[i]);
		}
	#endif

	point_num= data[2] & 0x0f;
	#if REPORT_TOUCH_DEBUG	
		printk(" zax point_num=%d \n",point_num);
	#endif
	memset(cinfo, 0, sizeof(struct touch_info));
	total_point = 0;	
	for(i = 0; i < fts_updateinfo_curr.TPD_MAX_POINTS/*point_num*/; i++)  
	{
		pointid = (data[FTS_TOUCH_ID_POS + FTS_TOUCH_STEP * i]) >> 4;
		if (pointid >= FTS_MAX_ID)
			break;
		else
			total_point++;
		cinfo->p[i] = data[3+6*i] >> 6; 							// event flag 
     		cinfo->id[i] = data[3+6*i+2]>>4; 						// touch id
	   	
		high_byte = data[3+6*i];
		high_byte <<= 8;
		high_byte &= 0x0f00;
		low_byte = data[3+6*i + 1];
		cinfo->x[i] = high_byte |low_byte;	
		high_byte = data[3+6*i+2];
		high_byte <<= 8;
		high_byte &= 0x0f00;
		low_byte = data[3+6*i+3];
		cinfo->y[i] = high_byte |low_byte;

		cinfo->pressure[i] =
			(data[FTS_TOUCH_XY_POS + FTS_TOUCH_STEP * i]);	// cannot constant value

			
		#if REPORT_TOUCH_DEBUG	
			printk(" zax tpd i=%d,  x= (0x%02x), y= (0x%02x), evt= %d,id=%d,pre=%d \n",i, cinfo->x[i], cinfo->y[i], cinfo->p[i],cinfo->id[i],cinfo->pressure[i]);
		#endif
	}

	
	return true;

}
#elif A_TYPE==1
static int tpd_touchinfo(struct touch_info *cinfo, struct touch_info *pinfo,struct touch_info *ptest)
{
	int i = 0;
	char data[128] = {0};
       u16 high_byte,low_byte,reg;
	u8 report_rate =0;
	if (tpd_halt)
	{
		TPD_DMESG( "tpd_touchinfo return ..\n");
		return false;
	}
	//mutex_lock(&i2c_access);

       reg = 0x00;
	fts_i2c_read(fts_i2c_client, &reg, 1, data, 64);
	//mutex_unlock(&i2c_access);
	
	// get the number of the touch points

	point_num= data[2] & 0x0f;
	if(up_flag==2)
	{
		up_flag=0;
		for(i = 0; i < fts_updateinfo_curr.TPD_MAX_POINTS; i++)  
		{
			cinfo->p[i] = data[3+6*i] >> 6; 			// event flag 
			cinfo->id[i] = data[3+6*i+2]>>4; 		// touch id
		   	//get the X coordinate, 2 bytes
			high_byte = data[3+6*i];
			high_byte <<= 8;
			high_byte &= 0x0f00;
			low_byte = data[3+6*i + 1];
			cinfo->x[i] = high_byte |low_byte;	
			high_byte = data[3+6*i+2];
			high_byte <<= 8;
			high_byte &= 0x0f00;
			low_byte = data[3+6*i+3];
			cinfo->y[i] = high_byte |low_byte;

			
			if(point_num>=i+1)
				continue;
			if(up_count==0)
				continue;
			cinfo->p[i] = ptest->p[i-point_num]; 	// event flag 
			
			
			cinfo->id[i] = ptest->id[i-point_num]; 	// touch id
	
			cinfo->x[i] = ptest->x[i-point_num];	
			
			cinfo->y[i] = ptest->y[i-point_num];
			//dev_err(&fts_i2c_client->dev," zax add two x = %d, y = %d, evt = %d,id=%d\n", cinfo->x[i], cinfo->y[i], cinfo->p[i], cinfo->id[i]);
			up_count--;
			
				
		}
		
		return true;
	}
	up_count=0;
	for(i = 0; i < fts_updateinfo_curr.TPD_MAX_POINTS; i++)  
	{
		cinfo->p[i] = data[3+6*i] >> 6; 				// event flag 
		
		if(0==cinfo->p[i])
		{
			//dev_err(&fts_i2c_client->dev,"\n  zax enter add   \n");
			up_flag=1;
     		}
     		cinfo->id[i] = data[3+6*i+2]>>4; 			// touch id
	   	// get the X coordinate, 2 bytes
		high_byte = data[3+6*i];
		high_byte <<= 8;
		high_byte &= 0x0f00;
		low_byte = data[3+6*i + 1];
		cinfo->x[i] = high_byte |low_byte;	
		high_byte = data[3+6*i+2];
		high_byte <<= 8;
		high_byte &= 0x0f00;
		low_byte = data[3+6*i+3];
		cinfo->y[i] = high_byte |low_byte;

		if(up_flag==1 && 1==cinfo->p[i])
		{
			up_flag=2;
			point_num++;
			ptest->x[up_count]=cinfo->x[i];
			ptest->y[up_count]=cinfo->y[i];
			ptest->id[up_count]=cinfo->id[i];
			ptest->p[up_count]=cinfo->p[i];
			//dev_err(&fts_i2c_client->dev," zax add x = %d, y = %d, evt = %d,id=%d\n", ptest->x[j], ptest->y[j], ptest->p[j], ptest->id[j]);
			cinfo->p[i]=2;
			up_count++;
		}
	}
	if(up_flag==1)
		up_flag=0;
	//printk(" tpd cinfo->x[0] = %d, cinfo->y[0] = %d, cinfo->p[0] = %d\n", cinfo->x[0], cinfo->y[0], cinfo->p[0]);
	return true;

}
 #elif A_TYPE==2
#if 0//hank mark
 static int tpd_touchinfo(struct touch_info *cinfo, struct touch_info *pinfo)
{
	int i = 0;
	char data[128] = {0};
       u16 high_byte,low_byte;
	char reg;

	if (tpd_halt)
	{
		TPD_DMESG( "tpd_touchinfo return ..\n");
		return false;
	}
	//mutex_lock(&i2c_access);

       reg = 0x00;
	fts_i2c_read(fts_i2c_client, &reg, 1, data, 64);
	//mutex_unlock(&i2c_access);
	
	/* get the number of the touch points */

	point_num= data[2] & 0x0f;
	
	for(i = 0; i < point_num; i++)  
	{
		cinfo->p[i] = data[3+6*i] >> 6; 		// event flag 
     		cinfo->id[i] = data[3+6*i+2]>>4; 	// touch id
	   	/* get the X coordinate, 2 bytes */
		high_byte = data[3+6*i];
		high_byte <<= 8;
		high_byte &= 0x0f00;
		low_byte = data[3+6*i + 1];
		cinfo->x[i] = high_byte |low_byte;	
		high_byte = data[3+6*i+2];
		high_byte <<= 8;
		high_byte &= 0x0f00;
		low_byte = data[3+6*i+3];
		cinfo->y[i] = high_byte |low_byte;
	}

	//printk(" tpd cinfo->x[0] = %d, cinfo->y[0] = %d, cinfo->p[0] = %d\n", cinfo->x[0], cinfo->y[0], cinfo->p[0]);
	return true;

 }
#endif
#endif



 /************************************************************************
* Name: fts_read_Touchdata
* Brief: report the point information
* Input: event info
* Output: get touch data in pinfo
* Return: success is zero
***********************************************************************/

static int fts_read_Touchdata(struct ts_event *data)
{
       u8 buf[POINT_READ_BUF] = { 0 };//0xFF
	int ret = -1;
	int i = 0;
	u8 pointid = FTS_MAX_ID;
	//u8 pt00f=0;
	if (tpd_halt)
	{
		TPD_DMESG( "tpd_touchinfo return ..\n");
		return false;
	}

	mutex_lock(&i2c_access);
	ret = fts_i2c_read(fts_i2c_client, buf, 1, buf, POINT_READ_BUF);
	if (ret < 0) 
	{
		dev_err(&fts_i2c_client->dev, "%s read touchdata failed.\n",__func__);
		mutex_unlock(&i2c_access);
		return ret;
	}
	#if REPORT_TOUCH_DEBUG	
		for(i=0;i<POINT_READ_BUF;i++)
		{
			dev_err(&fts_i2c_client->dev,"\n [fts] zax buf[%d] =(0x%02x)  \n", i,buf[i]);
		}
	#endif
	mutex_unlock(&i2c_access);	
	memset(data, 0, sizeof(struct ts_event));
	data->touch_point = 0;	
	data->touch_point_num=buf[FT_TOUCH_POINT_NUM] & 0x0F;
	//printk("tpd  fts_updateinfo_curr.TPD_MAX_POINTS=%d fts_updateinfo_curr.chihID=%d \n", fts_updateinfo_curr.TPD_MAX_POINTS,fts_updateinfo_curr.CHIP_ID);
	for (i = 0; i < fts_updateinfo_curr.TPD_MAX_POINTS; i++)
	{
		pointid = (buf[FTS_TOUCH_ID_POS + FTS_TOUCH_STEP * i]) >> 4;
		if (pointid >= FTS_MAX_ID)
			break;
		else
			data->touch_point++;
		data->au16_x[i] =
		    (s16) (buf[FTS_TOUCH_X_H_POS + FTS_TOUCH_STEP * i] & 0x0F) <<
		    8 | (s16) buf[FTS_TOUCH_X_L_POS + FTS_TOUCH_STEP * i];
		data->au16_y[i] =
		    (s16) (buf[FTS_TOUCH_Y_H_POS + FTS_TOUCH_STEP * i] & 0x0F) <<
		    8 | (s16) buf[FTS_TOUCH_Y_L_POS + FTS_TOUCH_STEP * i];
		data->au8_touch_event[i] =
		    buf[FTS_TOUCH_EVENT_POS + FTS_TOUCH_STEP * i] >> 6;
		data->au8_finger_id[i] =
		    (buf[FTS_TOUCH_ID_POS + FTS_TOUCH_STEP * i]) >> 4;

		data->pressure[i] =
			(buf[FTS_TOUCH_XY_POS + FTS_TOUCH_STEP * i]);//cannot constant value
		data->area[i] =
			(buf[FTS_TOUCH_MISC + FTS_TOUCH_STEP * i]) >> 4;
#if REPORT_TOUCH_DEBUG
			dev_err(&fts_i2c_client->dev,"\n [fts] zax data 1  (id= %d ,x=(0x%02x),y= (0x%02x)),point_num=%d, event=%d \n ", data->au8_finger_id[i],data->au16_x[i],data->au16_y[i],data->touch_point,data->au8_touch_event[i]);
		#endif
		if((data->au8_touch_event[i]==0 || data->au8_touch_event[i]==2)&&((data->touch_point_num==0)/*||(data->pressure[i]==0 && data->area[i]==0  )*/))
			return 1;
		#if REPORT_TOUCH_DEBUG
			dev_err(&fts_i2c_client->dev,"\n [fts] zax data  2 (id= %d ,x=(0x%02x),y= (0x%02x)),point_num=%d, event=%d \n ", data->au8_finger_id[i],data->au16_x[i],data->au16_y[i],data->touch_point,data->au8_touch_event[i]);
		#endif
		/*if(data->pressure[i]<=0)
		{
			data->pressure[i]=0x3f;
		}
		if(data->area[i]<=0)
		{
			data->area[i]=0x05;
		}
		*/
		//if ( pinfo->au16_x[i]==0 && pinfo->au16_y[i] ==0)
		//	pt00f++;
	}

	return 0;
}


 /************************************************************************
* Name: fts_report_value
* Brief: report the point information
* Input: event info
* Output: no
* Return: success is zero
***********************************************************************/
static int fts_report_value(struct ts_event *data)
 {
	//struct ts_event *event = NULL;
	int i = 0;//j=0;
	int up_point = 0;
 	int touchs = 0;
	//int touchs_count = 0;
	static int tmp_major=5;
	
	for (i = 0; i < data->touch_point; i++) 
	{
		tmp_major ^=0x01;
		
		 input_mt_slot(tpd->dev, data->au8_finger_id[i]);
 
		if (data->au8_touch_event[i]== 0 || data->au8_touch_event[i] == 2)
		{
			 input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER,true);
			 //input_report_abs(tpd->dev, ABS_MT_PRESSURE,0x3f/*data->pressure[i]*/);
			 input_report_abs(tpd->dev, ABS_MT_POSITION_X,data->au16_x[i]);
			 input_report_abs(tpd->dev, ABS_MT_POSITION_Y,data->au16_y[i]);
			 input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR,tmp_major/*data->area[i]*/);
			 touchs |= BIT(data->au8_finger_id[i]);
   			 data->touchs |= BIT(data->au8_finger_id[i]);
			 #if REPORT_TOUCH_DEBUG
				dev_err(&fts_i2c_client->dev,"\n [fts] zax down (id=%d ,x=%d, y=%d, pres=%d, area=%d) \n", data->au8_finger_id[i],data->au16_x[i],data->au16_y[i],data->pressure[i],data->area[i]);
			 #endif
		}
		else
		{
			#if REPORT_TOUCH_DEBUG
				dev_err(&fts_i2c_client->dev,"\n [fts] zax normal_up 1 (id=%d ,x=%d, y=%d, pres=%d, area=%d) \n", data->au8_finger_id[i],data->au16_x[i],data->au16_y[i],data->pressure[i],data->area[i]);
			 #endif
			 up_point++;
			 input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER,false);
			 data->touchs &= ~BIT(data->au8_finger_id[i]);
		}				 
		 
	}
#if 0
 	//if(unlikely(data->touchs ^ touchs)){
		for(i = 0; i < fts_updateinfo_curr.TPD_MAX_POINTS; i++){
			if(BIT(i) & (data->touchs ^ touchs)){
				
				#if REPORT_TOUCH_DEBUG
					dev_err(&fts_i2c_client->dev,"\n [fts] zax normal_up 2  id=%d \n", i);
				#endif
				data->touchs &= ~BIT(i);
				input_mt_slot(tpd->dev, i);
				input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER, false);
			}
		}
	//}
#endif
	#if REPORT_TOUCH_DEBUG
		dev_err(&fts_i2c_client->dev,"\n [fts] zax 1 touchs=%d, data-touchs=%d, touch_point=%d, up_point=%d \n ", touchs,data->touchs,data->touch_point, up_point);
	#endif
	data->touchs = touchs;
#if 0
	
	 if(/*(last_touchpoint>0)&&*/(data->touch_point_num==0))    // release all touches in final
	{	
		for(j = 0; j <fts_updateinfo_curr.TPD_MAX_POINTS; j++)
		{
			input_mt_slot( tpd->dev, j);
			input_mt_report_slot_state( tpd->dev, MT_TOOL_FINGER, false);
		}
		//last_touchpoint=0;
		data->touchs=0;
		input_report_key(tpd->dev, BTN_TOUCH, 0);
		input_sync(tpd->dev);
		
		//up_point=fts_updateinfo_curr.TPD_MAX_POINTS;
		//data->touch_point = up_point;
		#if REPORT_TOUCH_DEBUG
			dev_err(&fts_i2c_client->dev,"\n [fts] zax  normal_up 3 end 2 touchs=%d, data-touchs=%d, touch_point=%d, up_point=%d \n ", touchs,data->touchs,data->touch_point, up_point);
			printk("\n [fts] end 2 \n ");		
		#endif
		return 0;
    	} 
	#if REPORT_TOUCH_DEBUG
		dev_err(&fts_i2c_client->dev,"\n [fts] zax 2 touchs=%d, data-touchs=%d, touch_point=%d, up_point=%d \n ", touchs,data->touchs,data->touch_point, up_point);
	#endif
	if(data->touch_point == up_point)
		 input_report_key(tpd->dev, BTN_TOUCH, 0);
	else
		 input_report_key(tpd->dev, BTN_TOUCH, 1);
 
	input_sync(tpd->dev);
	#if REPORT_TOUCH_DEBUG
		printk("\n [fts] zax end 1 \n ");	
	#endif
	//last_touchpoint=event->touch_point_num ;	// release all touches in final
#else
            if(data->touch_point == up_point)
            {
		data->touchs = 0;
               for ( i=0; i<fts_updateinfo_curr.TPD_MAX_POINTS; i++)
                {
                    input_mt_slot(tpd->dev, i);
                    input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER, false);
                }
                input_report_key(tpd->dev, BTN_TOUCH, 0);
            }
            else
                input_report_key(tpd->dev, BTN_TOUCH, data->touch_point > 0);
            input_sync(tpd->dev);
#endif
	return 0;
    	//printk("tpd D x =%d,y= %d",event->au16_x[0],event->au16_y[0]);
 }
#ifdef TPD_PROXIMITY
 /************************************************************************
* Name: tpd_read_ps
* Brief: read proximity value
* Input: no
* Output: no
* Return: 0
***********************************************************************/
int tpd_read_ps(void)
{
	tpd_proximity_detect;
	return 0;    
}
 /************************************************************************
* Name: tpd_get_ps_value
* Brief: get proximity value
* Input: no
* Output: no
* Return: 0
***********************************************************************/
static int tpd_get_ps_value(void)
{
	return tpd_proximity_detect;
}
 /************************************************************************
* Name: tpd_enable_ps
* Brief: enable proximity
* Input: enable or not
* Output: no
* Return: 0
***********************************************************************/
static int tpd_enable_ps(int enable)
{
	u8 state;
	int ret = -1;
	
	//i2c_smbus_read_i2c_block_data(fts_i2c_client, 0xB0, 1, &state);

	ret = fts_read_reg(fts_i2c_client, 0xB0,&state);
	if (ret<0) 
	{
		printk("[Focal][Touch] read value fail");
		//return ret;
	}
	
	printk("[proxi_fts]read: 999 0xb0's value is 0x%02X\n", state);

	if (enable)
	{
		state |= 0x01;
		tpd_proximity_flag = 1;
		TPD_PROXIMITY_DEBUG("[proxi_fts]ps function is on\n");	
	}
	else
	{
		state &= 0x00;	
		tpd_proximity_flag = 0;
		TPD_PROXIMITY_DEBUG("[proxi_fts]ps function is off\n");
	}
	
	//ret = i2c_smbus_write_i2c_block_data(fts_i2c_client, 0xB0, 1, &state);
	ret = fts_write_reg(fts_i2c_client, 0xB0,state);
	if (ret<0) 
	{
		printk("[Focal][Touch] write value fail");
		//return ret;
	}
	TPD_PROXIMITY_DEBUG("[proxi_fts]write: 0xB0's value is 0x%02X\n", state);
	return 0;
}
 /************************************************************************
* Name: tpd_ps_operate
* Brief: operate function for proximity 
* Input: point, which operation, buf_in , buf_in len, buf_out , buf_out len, no use
* Output: buf_out
* Return: fail <0
***********************************************************************/
int tpd_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,

		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data *sensor_data;
	TPD_DEBUG("[proxi_fts]command = 0x%02X\n", command);		
	
	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			// Do nothing
			break;
		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{				
				value = *(int *)buff_in;
				if(value)
				{		
					if((tpd_enable_ps(1) != 0))
					{
						APS_ERR("enable ps fail: %d\n", err); 
						return -1;
					}
				}
				else
				{
					if((tpd_enable_ps(0) != 0))
					{
						APS_ERR("disable ps fail: %d\n", err); 
						return -1;
					}
				}
			}
			break;
		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				APS_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				sensor_data = (hwm_sensor_data *)buff_out;				
				if((err = tpd_read_ps()))
				{
					err = -1;;
				}
				else
				{
					sensor_data->values[0] = tpd_get_ps_value();
					TPD_PROXIMITY_DEBUG("huang sensor_data->values[0] 1082 = %d\n", sensor_data->values[0]);
					sensor_data->value_divide = 1;
					sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
				}					
			}
			break;
		default:
			APS_ERR("proxmy sensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}
	return err;	
}
#endif
#if FT_ESD_PROTECT
void esd_switch(s32 on)
{
    //spin_lock(&esd_lock); 
    if (1 == on) // switch on esd 
    {
       // if (!esd_running)
       // {
       //     esd_running = 1;
            //spin_unlock(&esd_lock);
            //printk("\n zax Esd started \n");
            queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, esd_check_circle);
        //}
        //else
        //{
         //   spin_unlock(&esd_lock);
        //}
    }
    else // switch off esd
    {
       // if (esd_running)
       // {
         //   esd_running = 0;
            //spin_unlock(&esd_lock);
            //printk("\n zax Esd cancell \n");
//printk("\n  zax switch off \n");
            cancel_delayed_work(&gtp_esd_check_work);
       // }
       // else
       // {
        //    spin_unlock(&esd_lock);
        //}
    }
}
/************************************************************************
* Name: force_reset_guitar
* Brief: reset
* Input: no
* Output: no
* Return: 0
***********************************************************************/
static void force_reset_guitar(void)
{
    	s32 i;
    	s32 ret;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
	disable_irq(touch_irq);
#else
	mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM); 	// disable interrupt 
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
	tpd_gpio_output(CTP_RST,0);
#else
	mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);  
#endif
	msleep(10);
    	TPD_DMESG("force_reset_guitar\n");

	hwPowerDown(MT6323_POWER_LDO_VGP1,  "TP");
	msleep(200);
	hwPowerOn(MT6323_POWER_LDO_VGP1, VOL_2800, "TP");
	//msleep(5);

	msleep(10);
	TPD_DMESG(" fts ic reset\n");
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
	tpd_gpio_output(CTP_RST,1);
	tpd_gpio_as_int(CTP_INT);
#else
	mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);

	mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
	mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_ENABLE);
	mt_set_gpio_pull_select(GPIO_CTP_EINT_PIN, GPIO_PULL_UP);
#endif
	msleep(300);
	
#ifdef TPD_PROXIMITY
	if (FT_PROXIMITY_ENABLE == tpd_proximity_flag) 
	{
		tpd_enable_ps(FT_PROXIMITY_ENABLE);
	}
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
	enable_irq(touch_irq);
#else
	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);	// enable interrupt
#endif
}

 
#define A3_REG_VALUE								0x54
#define RESET_91_REGVALUE_SAMECOUNT 				5
static u8 g_old_91_Reg_Value 							= 0x00;
static u8 g_first_read_91 								= 0x01;
static u8 g_91value_same_count 						= 0;
/************************************************************************
* Name: gtp_esd_check_func
* Brief: esd check function
* Input: struct work_struct
* Output: no
* Return: 0
***********************************************************************/
static void gtp_esd_check_func(struct work_struct *work)
{
	int i;
	int ret = -1;
	u8 data, data_old;
	u8 flag_error = 0;
	int reset_flag = 0;
	u8 check_91_reg_flag = 0;
//printk("\n zax -1 enter apk_debug_flag=%d \n",apk_debug_flag);
	if (tpd_halt ) 
	{
		return;
	}
//printk("\n zax 0 enter apk_debug_flag=%d \n",apk_debug_flag);
	if(is_update)
	{
		return;
	}
	//printk("\n zax 1 enter apk_debug_flag=%d \n",apk_debug_flag);
	if(apk_debug_flag) 
	{
		//queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, esd_check_circle);
		//printk("\n zax enter apk_debug_flag \n");
		return;
	}

	run_check_91_register = 0;
	for (i = 0; i < 3; i++) 
	{
		//ret = fts_i2c_smbus_read_i2c_block_data(i2c_client, 0xA3, 1, &data);
		ret = fts_read_reg(fts_i2c_client, 0xA3,&data);
		if (ret<0) 
		{
			printk("[FTS][Touch] read value fail");
			//return ret;
		}
		if (ret==1 && A3_REG_VALUE==data) 
		{
		    break;
		}
	}

	if (i >= 3) 
	{
		force_reset_guitar();
		printk("FTS--tpd reset. i >= 3  ret = %d	A3_Reg_Value = 0x%02x\n ", ret, data);
		reset_flag = 1;
		goto FOCAL_RESET_A3_REGISTER;
	}

	// esd check for count
  	//ret = fts_i2c_smbus_read_i2c_block_data(i2c_client, 0x8F, 1, &data);
	ret = fts_read_reg(fts_i2c_client, 0x8F,&data);
	if (ret<0) 
	{
		printk("[FTS][Touch] read value fail");
		//return ret;
	}
	printk("FTS 0x8F:%d, count_irq is %d\n", data, count_irq);
			
	flag_error = 0;
	if((count_irq - data) > 10) 
	{
		if((data+200) > (count_irq+10) )
		{
			flag_error = 1;
		}
	}
	
	if((data - count_irq ) > 10) 
	{
		flag_error = 1;		
	}
		
	if(1 == flag_error) 
	{	
		printk("FTS--tpd reset.1 == flag_error...data=%d	count_irq\n ", data, count_irq);
	    	force_reset_guitar();
		reset_flag = 1;
		goto FOCAL_RESET_INT;
	}

	run_check_91_register = 1;
	//ret = fts_i2c_smbus_read_i2c_block_data(i2c_client, 0x91, 1, &data);
	ret = fts_read_reg(fts_i2c_client, 0x91,&data);
	if (ret<0) 
	{
		printk("[FTS][Touch] read value fail");
		//return ret;
	}
	printk("FTS focal---------91 register value = 0x%02x	old value = 0x%02x\n",	data, g_old_91_Reg_Value);
	if(0x01 == g_first_read_91) 
	{
		g_old_91_Reg_Value = data;
		g_first_read_91 = 0x00;
	} 
	else 
	{
		if(g_old_91_Reg_Value == data)
		{
			g_91value_same_count++;
			printk("\n FTS focal 91 value ==============, g_91value_same_count=%d\n", g_91value_same_count);
			if(RESET_91_REGVALUE_SAMECOUNT == g_91value_same_count) 
			{
				force_reset_guitar();
				printk("focal--tpd reset. g_91value_same_count = 5\n");
				g_91value_same_count = 0;
				reset_flag = 1;
			}
			
			//run_check_91_register = 1;
			esd_check_circle = TPD_ESD_CHECK_CIRCLE / 2;
			g_old_91_Reg_Value = data;
		} 
		else 
		{
			g_old_91_Reg_Value = data;
			g_91value_same_count = 0;
			//run_check_91_register = 0;
			esd_check_circle = TPD_ESD_CHECK_CIRCLE;
		}
	}
FOCAL_RESET_INT:
FOCAL_RESET_A3_REGISTER:
	count_irq=0;
	data=0;
	//fts_i2c_smbus_write_i2c_block_data(i2c_client, 0x8F, 1, &data);
	ret = fts_write_reg(fts_i2c_client, 0x8F,data);
	if (ret<0) 
	{
		printk("[FTS][Touch] write value fail");
		//return ret;
	}
	if(0 == run_check_91_register)
	{
		g_91value_same_count = 0;
	}
	#ifdef TPD_PROXIMITY
	if( (1 == reset_flag) && ( FT_PROXIMITY_ENABLE == tpd_proximity_flag) )
	{
		if((tpd_enable_ps(FT_PROXIMITY_ENABLE) != 0))
		{
			APS_ERR("FTS enable ps fail\n"); 
			return -1;
		}
	}
	#endif
	// end esd check for count

    	if (!tpd_halt)
    	{
        	//queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, TPD_ESD_CHECK_CIRCLE);
        	queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, esd_check_circle);
    	}

    	return;
}
#endif
 /************************************************************************
* Name: touch_event_handler
* Brief: interrupt event from TP, and read/report data to Android system 
* Input: no use
* Output: no
* Return: 0
***********************************************************************/
 static int touch_event_handler(void *unused)
 {
	//struct touch_info cinfo, pinfo,ptest;
	struct ts_event pevent;
	//int i=0;
	int ret = 0;
	u8 state;
	#if A_TYPE==0
	static u8 pre_touch = 0;
	#endif
	struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };
	sched_setscheduler(current, SCHED_RR, &param);
 
	#ifdef TPD_PROXIMITY
		int err;
		hwm_sensor_data sensor_data;
		u8 proximity_status;
	#endif
	
	do
	{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
		//enable_irq(touch_irq);
#else
		 mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM); 
#endif
		 set_current_state(TASK_INTERRUPTIBLE); 
		 wait_event_interruptible(waiter,tpd_flag!=0);
						 
		 tpd_flag = 0;
			 
		 set_current_state(TASK_RUNNING);
		 //printk("tpd touch_event_handler\n");
//+add by hzb for dsm
#ifdef CONFIG_ONTIM_DSM	
		 ft6336s_irq_run_count++;
#endif
//-add by hzb for dsm
	 	 #if FTS_GESTRUE_EN
		 if(ft6336s_wakeup_enable){
			//i2c_smbus_read_i2c_block_data(fts_i2c_client, 0xd0, 1, &state);
			ret = fts_read_reg(fts_i2c_client, 0xd0,&state);
			if (ret<0) 
			{
				printk("[Focal][Touch] read value fail");
				//return ret;
			}
			//printk("tpd fts_read_Gestruedata state=%d\n",state);
		     	if(state ==1)
		     	{
			        fts_read_Gestruedata();
			        continue;
		    	}
		 }
		 #endif

		 #ifdef TPD_PROXIMITY

			 if (tpd_proximity_flag == 1)
			 {

				//i2c_smbus_read_i2c_block_data(fts_i2c_client, 0xB0, 1, &state);

				ret = fts_read_reg(fts_i2c_client, 0xB0,&state);
				if (ret<0) 
				{
					printk("[Focal][Touch] read value fail");
					//return ret;
				}
	           		TPD_PROXIMITY_DEBUG("proxi_fts 0xB0 state value is 1131 0x%02X\n", state);
				if(!(state&0x01))
				{
					tpd_enable_ps(1);
				}
				//i2c_smbus_read_i2c_block_data(fts_i2c_client, 0x01, 1, &proximity_status);
				ret = fts_read_reg(fts_i2c_client, 0x01,&proximity_status);
				if (ret<0) 
				{
					printk("[Focal][Touch] read value fail");
					//return ret;
				}
	            		TPD_PROXIMITY_DEBUG("proxi_fts 0x01 value is 1139 0x%02X\n", proximity_status);
				if (proximity_status == 0xC0)
				{
					tpd_proximity_detect = 0;	
				}
				else if(proximity_status == 0xE0)
				{
					tpd_proximity_detect = 1;
				}

				TPD_PROXIMITY_DEBUG("tpd_proximity_detect 1149 = %d\n", tpd_proximity_detect);
				if ((err = tpd_read_ps()))
				{
					TPD_PROXIMITY_DMESG("proxi_fts read ps data 1156: %d\n", err);	
				}
				sensor_data.values[0] = tpd_get_ps_value();
				sensor_data.value_divide = 1;
				sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
				//if ((err = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
				//{
				//	TPD_PROXIMITY_DMESG(" proxi_5206 call hwmsen_get_interrupt_data failed= %d\n", err);	
				//}
			}  

		#endif
                #if FT_ESD_PROTECT
				esd_switch(0);apk_debug_flag = 1;
			#endif                
		#ifdef MT_PROTOCOL_B
		{
			
            		ret = fts_read_Touchdata(&pevent);
			if (ret == 0)
			fts_report_value(&pevent);
			
		}
		#else
		{
			
			#if A_TYPE==0
			if (tpd_touchinfo(&cinfo, &pinfo,&ptest)) 
			{
		    		//printk("zax 0 tpd point_num = %d\n",point_num);
				//TPD_DEBUG_SET_TIME;
				if(point_num >0) 
				{
					
					for(i = 0; i < total_point/*fts_updateinfo_curr.TPD_MAX_POINTS*/; i++)  
					{
						if((0==cinfo.p[i]) || (2==cinfo.p[i]))
						{
				       		tpd_down(cinfo.x[i], cinfo.y[i], cinfo.pressure[i],cinfo.id[i]);
						}
						
					}
				    	input_sync(tpd->dev);
				}
				else if (pre_touch)
	    			{
	              		tpd_up(0,0,0);//(cinfo.x[0], cinfo.y[0],cinfo.id[0]);
	        	    		TPD_DEBUG("release --->\n");         	   
	        	    		input_sync(tpd->dev);
	        		}
				else
				{
				       TPD_DEBUG("Additional Eint!");
				}
			       pre_touch = point_num;
				/*
				if (tpd != NULL && tpd->dev != NULL)
			       {
			            input_sync(tpd->dev);
			       }
			       */
        		}
			#elif A_TYPE==1
			if (tpd_touchinfo(&cinfo, &pinfo,&ptest)) 
			{
		    		//printk("zax 1 tpd point_num = %d\n",point_num);
				TPD_DEBUG_SET_TIME;
				if(point_num >0) 
				{
					
					for(i = 0; i < fts_updateinfo_curr.TPD_MAX_POINTS; i++)  
					{
						if((0==cinfo.p[i]) || (2==cinfo.p[i]))
						{
				       		tpd_down(cinfo.x[i], cinfo.y[i], cinfo.id[i]);
						}
						
					}
				    	input_sync(tpd->dev);
				}
				else  
	    			{
	              		tpd_up(cinfo.x[0], cinfo.y[0],&cinfo.id[0]);
	        	    		//TPD_DEBUG("release --->\n");         	   
	        	    		input_sync(tpd->dev);
	        		}
        		}
			#elif A_TYPE==2
			if (tpd_touchinfo(&cinfo, &pinfo)) 
			{
		    		//printk("zax 2 tpd point_num = %d\n",point_num);
				TPD_DEBUG_SET_TIME;
				if(point_num >0) 
				{
				    for(i =0; i<point_num; i++)	// only support 3 point
				    {
				         tpd_down(cinfo.x[i], cinfo.y[i], cinfo.id[i]);
				    }
				    input_sync(tpd->dev);
				}
				else  
	    			{
	              		tpd_up(cinfo.x[0], cinfo.y[0],&cinfo.id[0]);
	        	    		//TPD_DEBUG("release --->\n");         	   
	        	    		input_sync(tpd->dev);
	        		}
        		}
			#endif
			
		}
		#endif
		#if FT_ESD_PROTECT
				esd_switch(1);apk_debug_flag = 0;
		#endif
 	}while(!kthread_should_stop());
	return 0;
 }
  /************************************************************************
* Name: fts_reset_tp
* Brief: reset TP
* Input: pull low or high
* Output: no
* Return: 0
***********************************************************************/
void fts_reset_tp(int HighOrLow)
{
	
	if(HighOrLow)
	{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
		tpd_gpio_output(CTP_RST,1);
#else
		mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
	    	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
	    	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);  
#endif
	}
	else
	{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
		tpd_gpio_output(CTP_RST,0);
#else
		mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
	    	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
	    	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
#endif
	}
	
}
   /************************************************************************
* Name: tpd_detect
* Brief: copy device name
* Input: i2c info, board info
* Output: no
* Return: 0
***********************************************************************/
 static int tpd_detect (struct i2c_client *client, struct i2c_board_info *info) 
 {
	 	strcpy(info->type, TPD_DEVICE);	
	  	return 0;
 }
/************************************************************************
* Name: tpd_eint_interrupt_handler
* Brief: deal with the interrupt event
* Input: no
* Output: no
* Return: no
***********************************************************************/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
static irqreturn_t tpd_eint_interrupt_handler(unsigned irq, struct irq_desc *desc)
#else
static void tpd_eint_interrupt_handler(void)
#endif
 {
	 //TPD_DEBUG("TPD interrupt has been triggered\n");
	 TPD_DEBUG_PRINT_INT;
	 tpd_flag = 1;
	 #if FT_ESD_PROTECT
		count_irq ++;
	 #endif
//+add by hzb for dsm
#ifdef CONFIG_ONTIM_DSM	
	 ft6336s_irq_count++;
#endif
//-add by hzb for dsm
	 wake_up_interruptible(&waiter);
	return IRQ_HANDLED;
 }
/************************************************************************
* Name: fts_init_gpio_hw
* Brief: initial gpio
* Input: no
* Output: no
* Return: 0
***********************************************************************/
 static int fts_init_gpio_hw(void)
{

	int ret = 0;
	//int i = 0;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
	tpd_gpio_output(CTP_RST,1);
#else
	mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
#endif
	
	return ret;
}
   
static int tpd_irq_registration(void)
{
        struct device_node *node = NULL;
        int ret = 0;
        u32 ints[2] = { 0, 0 };

        TPD_DMESG("Device Tree Tpd_irq_registration!");

        node = of_find_matching_node(node, touch_of_match);
        if (node) {
                of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));

                touch_irq = irq_of_parse_and_map(node, 0);
                ret =request_irq(touch_irq, (irq_handler_t) tpd_eint_interrupt_handler, IRQF_TRIGGER_FALLING,"TOUCH_PANEL-eint", NULL);
                if (ret > 0) {
                	ret = -1;
                        TPD_DMESG("tpd request_irq IRQ LINE NOT AVAILABLE!.");
               	}
        } else {
                TPD_DMESG("tpd request_irq can not find touch eint device node!.");
                ret = -1;
        }
        TPD_DMESG("[%s]irq:%d, debounce:%d-%d:", __func__, touch_irq, ints[0], ints[1]);
        return ret;
}

#define FTS_REG_FW_MIN_VER                                                                      0xB2
#define FTS_REG_FW_SUB_MIN_VER                                                          0xB3
#define   FTS_REG_FW_VENDOR_ID 0xA8
/************************************************************************
* Name: tpd_probe
* Brief: driver entrance function for initial/power on/create channel 
* Input: i2c info, device id
* Output: no
* Return: 0
***********************************************************************/
 static int __devinit tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
 {	 
	//int upgread_ret=0;  //add by hzb for dsm
	int retval = TPD_OK;
	char data;
	//u8 report_rate=0,
	//u8 err=0;
	s32 errval=0;
	//int reset_count = 0;
	//unsigned char uc_reg_value;
	//unsigned char uc_reg_addr;
	u8 fw_ver[3];
        u8 reg_addr;
        int err1;
	u8 ft6336s_fw_version[20]={'\0'};
	#ifdef TPD_PROXIMITY
		int err;
		struct hwmsen_object obj_ps;
	#endif
	
	//if(CHECK_THIS_DEV_DEBUG_AREADY_EXIT()==0)
    //{
    //   return -EIO;
    //}

	//reset_proc:   
		fts_i2c_client = client;
		fts_input_dev=tpd->dev;
         #ifdef TPD_CLOSE_POWER_IN_SLEEP	 
		
	#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
		tpd_gpio_output(CTP_RST,1);
		tpd_gpio_output(CTP_RST,0);
#else
		mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
	    	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
	    	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);  
#endif
		msleep(10);
		
	#endif	
   	
	// power on, need confirm with SA
	#ifdef TPD_POWER_SOURCE_CUSTOM
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
		tpd->reg = regulator_get(tpd->tpd_dev, "vtouch");
	        retval = regulator_set_voltage(tpd->reg, 2800000, 2800000);        /*set 2.8v*/
        	if (retval) {
		if (!IS_ERR(tpd->reg))
		regulator_put(tpd->reg);
                	TPD_DMESG("regulator_set_voltage(%d) failed!\n", retval);
                	return -1;
        	}	

		retval = regulator_enable(tpd->reg);       /*enable regulator*/
                if (retval)
		{
			regulator_put(tpd->reg);
			TPD_DMESG("regulator_enable() failed!\n");
			return -1;
		}
#else
		hwPowerOn(TPD_POWER_SOURCE_CUSTOM, VOL_2800, "TP");
#endif
	#else
		hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_2800, "TP");
	#endif
	#ifdef TPD_POWER_SOURCE_1800
		hwPowerOn(TPD_POWER_SOURCE_1800, VOL_1800, "TP");
	#endif 


	#ifdef TPD_CLOSE_POWER_IN_SLEEP	 
		hwPowerDown(TPD_POWER_SOURCE,"TP");
		hwPowerOn(TPD_POWER_SOURCE,VOL_3300,"TP");
		msleep(100);
	#else
		
		msleep(10);
		TPD_DMESG(" fts reset\n");
	    	printk(" fts reset\n");
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
		tpd_gpio_output(CTP_RST,1);
#else
		mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
	    	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
	    	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
#endif
	#endif	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
	tpd_gpio_as_int(CTP_INT);
#else
	mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
    	mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
    	mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_ENABLE);
    	mt_set_gpio_pull_select(GPIO_CTP_EINT_PIN, GPIO_PULL_UP);
#endif
	

	msleep(200);

 	errval = i2c_smbus_read_i2c_block_data(fts_i2c_client, 0x00, 1, &data);// if auto upgrade fail, it will not read right value next upgrade.

	//err = fts_read_reg(fts_i2c_client, 0x00,&data);
		

		
	TPD_DMESG("gao_i2c:err %d,data:%d\n", errval,data);
	if(errval < 0 || data!=0)	// reg0 data running state is 0; other state is not 0
	{
		TPD_DMESG("I2C transfer error, line: %d\n", __LINE__);
/*
        if ( ++reset_count < TPD_MAX_RESET_COUNT )
        {
            goto reset_proc;
        }
*/
	#ifdef TPD_POWER_SOURCE_CUSTOM
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
		retval = regulator_disable(tpd->reg);      /*disable regulator*/
                if (retval)
                	TPD_DMESG("regulator_disable() failed!\n");
		regulator_put(tpd->reg);
#else
		hwPowerDown(TPD_POWER_SOURCE_CUSTOM, "TP");
#endif
	#else
		hwPowerDown(MT65XX_POWER_LDO_VGP2, "TP");
	#endif

		return -1; 
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
        if(tpd_irq_registration() < 0)
	{
	retval = regulator_disable(tpd->reg);      /*disable regulator*/
        if (retval)
        	TPD_DMESG("regulator_disable() failed!\n");
        regulator_put(tpd->reg);

	return -1;
	}
        disable_irq(touch_irq);
#endif
	msg_dma_alloct();
	
       fts_init_gpio_hw();
	
	/*
	uc_reg_addr = FTS_REG_POINT_RATE;				
	fts_i2c_write(fts_i2c_client, &uc_reg_addr, 1);
	fts_i2c_read(fts_i2c_client, &uc_reg_addr, 0, &uc_reg_value, 1);
	printk("mtk_tpd[FTS] report rate is %dHz.\n",uc_reg_value * 10);

	uc_reg_addr = FTS_REG_FW_VER;
	fts_i2c_write(fts_i2c_client, &uc_reg_addr, 1);
	fts_i2c_read(fts_i2c_client, &uc_reg_addr, 0, &uc_reg_value, 1);
	printk("mtk_tpd[FTS] Firmware version = 0x%x\n", uc_reg_value);


	uc_reg_addr = FTS_REG_CHIP_ID;
	fts_i2c_write(fts_i2c_client, &uc_reg_addr, 1);
	retval=fts_i2c_read(fts_i2c_client, &uc_reg_addr, 0, &uc_reg_value, 1);
	printk("mtk_tpd[FTS] chip id is %d.\n",uc_reg_value);
    	if(retval<0)
    	{
       	 printk("mtk_tpd[FTS] Read I2C error! driver NOt load!! CTP chip id is %d.\n",uc_reg_value);
		return 0;
	}
	*/
	
	tpd_load_status = 1;
	/*
	mt65xx_eint_set_sens(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_SENSITIVE);
	mt65xx_eint_set_hw_debounce(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_CN);
	mt65xx_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_EN, CUST_EINT_TOUCH_PANEL_POLARITY, tpd_eint_interrupt_handler, 1); 
	mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
	*/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 18, 19))
	mt_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, EINTF_TRIGGER_FALLING, tpd_eint_interrupt_handler, 1);
    	mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
#endif

	
    	#ifdef VELOCITY_CUSTOM_fts
		if((err = misc_register(&tpd_misc_device)))
		{
			printk("mtk_tpd: tpd_misc_device register failed\n");
		
		}
	#endif

	thread = kthread_run(touch_event_handler, 0, TPD_DEVICE);
	 if (IS_ERR(thread))
	{ 
		  retval = PTR_ERR(thread);
		  TPD_DMESG(TPD_DEVICE " failed to create kernel thread: %d\n", retval);
	}


	
	#ifdef SYSFS_DEBUG
                fts_create_sysfs(fts_i2c_client);
	#endif
	HidI2c_To_StdI2c(fts_i2c_client);
	fts_get_upgrade_array();
	#ifdef FTS_CTL_IIC
		 if (fts_rw_iic_drv_init(fts_i2c_client) < 0)
			 dev_err(&client->dev, "%s:[FTS] create fts control iic driver failed\n", __func__);
	#endif
	
	#ifdef FTS_APK_DEBUG
		fts_create_apk_debug_channel(fts_i2c_client);
	#endif
	
	#ifdef TPD_AUTO_UPGRADE
		printk("********************Enter CTP Auto Upgrade********************\n");
		is_update = true;
		upgread_ret=fts_ctpm_auto_upgrade(fts_i2c_client);  //modify by hzb for dsm
		is_update = false;
	#endif

	#ifdef TPD_PROXIMITY
		{
			obj_ps.polling = 1; // 0--interrupt mode;1--polling mode;
			obj_ps.sensor_operate = tpd_ps_operate;
			if ((err = hwmsen_attach(ID_PROXIMITY, &obj_ps)))
			{
				TPD_DEBUG("hwmsen attach fail, return:%d.", err);
			}
		}
	#endif
	#if FT_ESD_PROTECT
   		INIT_DELAYED_WORK(&gtp_esd_check_work, gtp_esd_check_func);
    		gtp_esd_check_workqueue = create_workqueue("gtp_esd_check");
    		queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, TPD_ESD_CHECK_CIRCLE);
	#endif

	
	#if FTS_GESTRUE_EN
		fts_Gesture_init(tpd->dev);		
	#endif
	#ifdef MT_PROTOCOL_B
		input_set_abs_params(tpd->dev, ABS_MT_TOUCH_MAJOR,0, 255, 0, 0);
		input_set_abs_params(tpd->dev, ABS_MT_POSITION_X, 0, TPD_RES_X, 0, 0);
		input_set_abs_params(tpd->dev, ABS_MT_POSITION_Y, 0, TPD_RES_Y, 0, 0);
		//input_set_abs_params(tpd->dev, ABS_MT_PRESSURE, 0, 255, 0, 0);
		#if 0 //(LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0))
			input_mt_init_slots(tpd->dev, MT_MAX_TOUCH_POINTS);
		#else
			input_mt_init_slots(tpd->dev,fts_updateinfo_curr.TPD_MAX_POINTS,(INPUT_MT_POINTER | INPUT_MT_DIRECT));
		#endif
	#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
	enable_irq(touch_irq);
#else
	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
#endif
	
        reg_addr = FTS_REG_FW_VER;
        err1 = fts_i2c_read(fts_i2c_client, &reg_addr, 1, &fw_ver[0], 1);
        if (err1 < 0)
                dev_err(&client->dev, "fw major version read failed");

        reg_addr = FTS_REG_FW_MIN_VER;
        err1 = fts_i2c_read(fts_i2c_client, &reg_addr, 1, &fw_ver[1], 1);
        if (err1 < 0)
                dev_err(&client->dev, "fw minor version read failed");

        reg_addr = FTS_REG_FW_SUB_MIN_VER;
        err1 = fts_i2c_read(fts_i2c_client, &reg_addr, 1, &fw_ver[2], 1);
        if (err1 < 0)
                dev_err(&client->dev, "fw sub minor version read failed");

        dev_info(&client->dev, "Firmware version = %d.%d.%d\n",
                fw_ver[0], fw_ver[1], fw_ver[2]);

	{
		u8 fw_vendor_id = 0x00;
        	u8 reg_addr;
        	int err;

        	reg_addr = FTS_REG_FW_VENDOR_ID;
        	err = fts_i2c_read(fts_i2c_client, &reg_addr, 1, &fw_vendor_id, 1);
        	if (err < 0)
                	dev_err(&client->dev, "fw vendor id read failed");
		else
		{
			if(0x51 == fw_vendor_id)
			{
				sprintf(ft6336s_vendor_name,"%s","ofilm-ft6336s");
			}
		}
	}	
     
	sprintf(ft6336s_fw_version,"-fw:%d.%d.%d",fw_ver[0],fw_ver[1],fw_ver[2]);
	strcat(ft6336s_vendor_name,ft6336s_fw_version);
	
	printk("fts Touch Panel Device Probe %s\n", (retval < TPD_OK) ? "FAIL" : "PASS");

	//REGISTER_AND_INIT_ONTIM_DEBUG_FOR_THIS_DEV();

#ifdef CONFIG_ONTIM_DSM	
	tp_probe_ok=1;//add by liuwei
	focaltech_dsm_client=dsm_register_client (&focaltech_dsm_dev);
	if (upgread_ret<0)
	{
		int error=OMTIM_DSM_TP_FW_UPGRAED_ERROR;
	 	if ( (focaltech_dsm_client ) && dsm_client_ocuppy(focaltech_dsm_client))
	 	{
	 		if ((focaltech_dsm_client->dump_buff) && (focaltech_dsm_client->buff_size)&&(focaltech_dsm_client->buff_flag == OMTIM_DSM_BUFF_OK))
	 		{
				focaltech_dsm_client->used_size = sprintf(focaltech_dsm_client->dump_buff,"Type=%d; ID=%d; error_id=%d; CTP info:%s; FW upgread error = %d\n",focaltech_dsm_client->client_type,focaltech_dsm_client->client_id,error,ft6336s_vendor_name,upgread_ret );
				dsm_client_notify(focaltech_dsm_client,error);
	 		}
	 	}
		else
		{
			printk(KERN_ERR "%s: dsm ocuppy error!!!",__func__);
		}
	}
	if (retval<0)
	{
		int error=OMTIM_DSM_TP_CREATE_THREAD_ERROR;
	 	if ( (focaltech_dsm_client ) && dsm_client_ocuppy(focaltech_dsm_client))
	 	{
	 		if ((focaltech_dsm_client->dump_buff) && (focaltech_dsm_client->buff_size)&&(focaltech_dsm_client->buff_flag == OMTIM_DSM_BUFF_OK))
	 		{
				focaltech_dsm_client->used_size = sprintf(focaltech_dsm_client->dump_buff,"Type=%d; ID=%d; error_id=%d; CTP info:%s; Create kernel thread error = %d\n",focaltech_dsm_client->client_type,focaltech_dsm_client->client_id,error,ft6336s_vendor_name,retval );
				dsm_client_notify(focaltech_dsm_client,error);
	 		}
	 	}
		else
		{
			printk(KERN_ERR "%s: dsm ocuppy error!!!",__func__);
		}
	}
#endif
	
   	return 0;
   
 }
/************************************************************************
* Name: tpd_remove
* Brief: remove driver/channel
* Input: i2c info
* Output: no
* Return: 0
***********************************************************************/
 static int __devexit tpd_remove(struct i2c_client *client)
 
 {
     int retval = TPD_OK;

     msg_dma_release();

     #ifdef FTS_CTL_IIC
     		fts_rw_iic_drv_exit();
     #endif
     #ifdef SYSFS_DEBUG
     		fts_remove_sysfs(client);
     #endif
     #if FT_ESD_PROTECT
    		destroy_workqueue(gtp_esd_check_workqueue);
     #endif

     #ifdef FTS_APK_DEBUG
     		fts_release_apk_debug_channel();
     #endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
     free_irq(touch_irq, NULL);
     
     retval = regulator_disable(tpd->reg);      //disable regulator
     if (retval)
     	TPD_DMESG("regulator_disable() failed!\n");
     regulator_put(tpd->reg);
#endif
	TPD_DMESG("TPD removed\n");
 
   return 0;
 }


 /************************************************************************
* Name: tpd_local_init
* Brief: add driver info
* Input: no
* Output: no
* Return: fail <0
***********************************************************************/
 static int tpd_local_init(void)
 {
   	if(i2c_add_driver(&tpd_i2c_driver)!=0)
   	{
        	TPD_DMESG("fts unable to add i2c driver.\n");
      		return -1;
    	}
    	if(tpd_load_status == 0) 
    	{
       	TPD_DMESG("fts add error touch panel driver.\n");
    		i2c_del_driver(&tpd_i2c_driver);
    		return -1;
    	}
	// TINNO_TOUCH_TRACK_IDS <--- finger number
	// TINNO_TOUCH_TRACK_IDS	5
	#ifdef MT_PROTOCOL_B
	#else
		#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 8, 0))
			// for linux 3.8
			input_set_abs_params(tpd->dev, ABS_MT_TRACKING_ID, 0, (TPD_MAX_POINTS_10-1), 0, 0);
		#endif
	#endif
	
	
   	#ifdef TPD_HAVE_BUTTON     
		// initialize tpd button data
    	 	tpd_button_setting(TPD_KEY_COUNT, tpd_keys_local, tpd_keys_dim_local);
	#endif   
  
	#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))    
    		TPD_DO_WARP = 1;
    		memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT*4);
    		memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT*4);
	#endif 

	#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
    		memcpy(tpd_calmat, tpd_def_calmat_local, 8*4);
    		memcpy(tpd_def_calmat, tpd_def_calmat_local, 8*4);	
	#endif  
	TPD_DMESG("end %s, %d\n", __FUNCTION__, __LINE__);  
	tpd_type_cap = 1;
    	return 0; 
 }
static void fts_release_all_finger ( void )
{
	unsigned int finger_count=0;

#ifndef MT_PROTOCOL_B
	input_mt_sync ( tpd->dev );
#else
	for(finger_count = 0; finger_count < fts_updateinfo_curr.TPD_MAX_POINTS; finger_count++)
	{
		input_mt_slot( tpd->dev, finger_count);
		input_mt_report_slot_state( tpd->dev, MT_TOOL_FINGER, false);
	}
	 input_report_key(tpd->dev, BTN_TOUCH, 0);
#endif
	input_sync ( tpd->dev );

}
 /************************************************************************
* Name: tpd_resume
* Brief: system wake up 
* Input: no use
* Output: no
* Return: no
***********************************************************************/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
static void tpd_resume(struct device *h )
#else
 static void tpd_resume( struct early_suspend *h )
#endif
 {
	//int i=0,ret = 0;
	
 	TPD_DMESG("TPD wake up\n");
	/*buf_addr[0]=0xC0;
	buf_addr[1]=0x8B;
				
	for(i=0;i<2;i++)
	{
		ret = fts_write_reg(fts_i2c_client, buf_addr[i], buf_value[i]);
		if (ret<0) 
		{
			printk("[Focal][Touch] write value fail");
			//return ret;
		}
	}*/
	
  	#ifdef TPD_PROXIMITY	
		if (tpd_proximity_flag == 1)
		{
			if(tpd_proximity_flag_one == 1)
			{
				tpd_proximity_flag_one = 0;	
				TPD_DMESG(TPD_DEVICE " tpd_proximity_flag_one \n"); 
				return;
			}
		}
	#endif	

 	#if FTS_GESTRUE_EN
	if(ft6336s_wakeup_enable){
 		//if (fts_updateinfo_curr.CHIP_ID!=0x86)
		//{
    			fts_write_reg(fts_i2c_client,0xD0,0x00);
    		//}
	}
	#endif
	#ifdef TPD_CLOSE_POWER_IN_SLEEP	
		hwPowerOn(TPD_POWER_SOURCE,VOL_3300,"TP");
	#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
		tpd_gpio_output(CTP_RST,0);
		msleep(1);
		tpd_gpio_output(CTP_RST,1);
#else
		mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    		mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    		mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);  
    		msleep(1);  
    		mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    		mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    		mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
#endif
    		
	#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
	enable_irq(touch_irq);
#else
	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);  
#endif
	msleep(30);
	fts_release_all_finger();
	tpd_halt = 0;
	
	#if FT_ESD_PROTECT
                count_irq = 0;
    		queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, TPD_ESD_CHECK_CIRCLE);
	#endif

	TPD_DMESG("TPD wake up done\n");

 }
 /************************************************************************
* Name: tpd_suspend
* Brief: system sleep
* Input: no use
* Output: no
* Return: no
***********************************************************************/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
static void tpd_suspend( struct device *h )
#else
 static void tpd_suspend( struct early_suspend *h )
#endif
 {
	static char data = 0x3;
	//int i=0,
	int ret = 0;
	/*buf_addr[0]=0xC0;
	buf_addr[1]=0x8B;
				
	for(i=0;i<2;i++)
	{
		ret = fts_read_reg(fts_i2c_client, buf_addr[i], (buf_value+i));
		if (ret<0) 
		{
			printk("[Focal][Touch] read value fail");
			//return ret;
		}
	}
	*/
			
	TPD_DMESG("TPD enter sleep\n");
//+add by hzb for dsm
#ifdef CONFIG_ONTIM_DSM	
 	if ( (focaltech_dsm_client ) && dsm_client_ocuppy(focaltech_dsm_client))
 	{
		int error=OMTIM_DSM_TP_INFO;
 		if ((focaltech_dsm_client->dump_buff) && (focaltech_dsm_client->buff_size)&&(focaltech_dsm_client->buff_flag == OMTIM_DSM_BUFF_OK))
 		{
			focaltech_dsm_client->used_size = sprintf(focaltech_dsm_client->dump_buff,"Type=%d; ID=%d; error_id=%d;  CTP info:%s; irq_count = %d; irq_run_count =%d\n",focaltech_dsm_client->client_type,focaltech_dsm_client->client_id,error,ft6336s_vendor_name,ft6336s_irq_count,ft6336s_irq_run_count );
			dsm_client_notify(focaltech_dsm_client,error);
 		}
 	}
	else
	{
		printk(KERN_ERR "%s: dsm ocuppy error!!!",__func__);
	}
#endif
//-add by hzb for dsm
	#ifdef TPD_PROXIMITY
	if (tpd_proximity_flag == 1)
	{
		tpd_proximity_flag_one = 1;	
		return;
	}
	#endif

	#if FTS_GESTRUE_EN
	if(ft6336s_wakeup_enable){
	
		if (fts_updateinfo_curr.CHIP_ID==0x54 || fts_updateinfo_curr.CHIP_ID==0x58 || fts_updateinfo_curr.CHIP_ID==0x86  || fts_updateinfo_curr.CHIP_ID==0x87)
		{
		  	fts_write_reg(fts_i2c_client, 0xd1, 0xff);
			fts_write_reg(fts_i2c_client, 0xd2, 0xff);
			fts_write_reg(fts_i2c_client, 0xd5, 0xff);
			fts_write_reg(fts_i2c_client, 0xd6, 0xff);
			fts_write_reg(fts_i2c_client, 0xd7, 0xff);
			fts_write_reg(fts_i2c_client, 0xd8, 0xff);
		}
		fts_write_reg(fts_i2c_client, 0xd0, 0x01);
        	return;
	}
	#endif
	#if FT_ESD_PROTECT
    		cancel_delayed_work_sync(&gtp_esd_check_work);
	#endif
 	 tpd_halt = 1;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
	disable_irq(touch_irq);
#else
	 mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
#endif
	 mutex_lock(&i2c_access);
	#ifdef TPD_CLOSE_POWER_IN_SLEEP	
		hwPowerDown(TPD_POWER_SOURCE,"TP");
	#else
		if ((fts_updateinfo_curr.CHIP_ID==0x59))
		{
			//i2c_smbus_write_i2c_block_data(fts_i2c_client, 0xA5, 1, &data);  //TP enter sleep mode
			data = 0x02;
			ret = fts_write_reg(fts_i2c_client, 0xA5,data);
			
			if (ret<0) 
			{
				printk("[Focal][Touch] write value fail");
				//return ret;
			}
		}
		else
		{
			
			//i2c_smbus_write_i2c_block_data(fts_i2c_client, 0xA5, 1, &data);  //TP enter sleep mode
			ret = fts_write_reg(fts_i2c_client, 0xA5,data);
			if (ret<0) 
			{
				printk("[Focal][Touch] write value fail");
				//return ret;
			}
		}
		msleep(10);
	#endif
	mutex_unlock(&i2c_access);
	fts_release_all_finger();
	//disable_irq_nosync(ts->pdata->intr_gpio);
	//

	//if ((fts_updateinfo_curr.CHIP_ID==0x59))
		//fts_write_reg(ts->client,0xa5,0x02);
	//else
		//fts_write_reg(ts->client,0xa5,0x03);	
	//msleep(10);
	/*release add touches*/
	/*		
	for (i = 0; i <CFG_MAX_TOUCH_POINTS; i++) 
	{
		input_mt_slot(fts_input_dev, i);
		input_mt_report_slot_state(fts_input_dev, MT_TOOL_FINGER, 0);
	}
	input_mt_report_pointer_emulation(fts_input_dev, false);
	input_sync(fts_input_dev);
	*/
	
    	TPD_DMESG("TPD enter sleep done\n");

 } 


 static struct tpd_driver_t tpd_device_driver = {
       	 .tpd_device_name 	= "fts",
		 .tpd_local_init 		= tpd_local_init,
		 .suspend 			= tpd_suspend,
		 .resume 				= tpd_resume,
	
	#ifdef TPD_HAVE_BUTTON
		 .tpd_have_button 		= 1,
	#else
		 .tpd_have_button 		= 0,
	#endif
	
 };

  /************************************************************************
* Name: tpd_suspend
* Brief:  called when loaded into kernel
* Input: no
* Output: no
* Return: 0
***********************************************************************/
 static int __init tpd_driver_init(void) {
        printk("MediaTek fts touch panel driver init\n");
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 19))
	tpd_get_dts_info();
#else
        i2c_register_board_info(IIC_PORT, &fts_i2c_tpd, 1);
#endif
	if(tpd_driver_add(&tpd_device_driver) < 0)
       	TPD_DMESG("add fts driver failed\n");
	 return 0;
 }
 
 
/************************************************************************
* Name: tpd_driver_exit
* Brief:  should never be called
* Input: no
* Output: no
* Return: 0
***********************************************************************/
 static void __exit tpd_driver_exit(void) 
 {
        TPD_DMESG("MediaTek fts touch panel driver exit\n");
	 //input_unregister_device(tpd->dev);
	 tpd_driver_remove(&tpd_device_driver);
 }
 
 module_init(tpd_driver_init);
 module_exit(tpd_driver_exit);
