/* For MTK android platform.
 *
 * msa.c - Linux kernel modules for 3-Axis Accelerometer
 *
 * Copyright (C) 2007-2016 MEMS Sensing Technology Co., Ltd.
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

#include <cust_acc.h>
#include "msa_core.h"
#include "msa_cust.h"

#if MTK_ANDROID_M
#include <accel.h>
#include "mt_boot.h"
#else
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <linux/hwmsen_helper.h>


#if  MSA_SUPPORT_FAST_AUTO_CALI
#include <mach/mt_boot.h>
#endif

#if defined(MT6516)
#include <mach/mt6516_devs.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_gpio.h>
#include <mach/mt6516_pll.h>
#elif defined(MT6573)
#include <mach/mt6573_devs.h>
#include <mach/mt6573_typedefs.h>
#include <mach/mt6573_gpio.h>
#include <mach/mt6573_pll.h>
#elif defined(MT6575)
#include <mach/mt6575_devs.h>
#include <mach/mt6575_typedefs.h>
#include <mach/mt6575_gpio.h>
#include <mach/mt6575_pm_ldo.h>
#else
#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_boot.h>
#endif

#if defined(MT6516)
#define POWER_NONE_MACRO MT6516_POWER_NONE
#else
#define POWER_NONE_MACRO MT65XX_POWER_NONE
#endif
#endif

#define MSA_DRV_NAME                 	"msa"
#define MSA_MISC_NAME                	"gsensor"
#define MSA_PLATFORM_NAME         "gsensor"



#define MSA_AXIS_X          			0
#define MSA_AXIS_Y         		 	1
#define MSA_AXIS_Z          			2
#define MSA_AXES_NUM        			3

#define MTK_AUTO_MODE           		1 

struct scale_factor{
    u8  whole;
    u8  fraction;
};

struct data_resolution {
    struct scale_factor scalefactor;
    int                 sensitivity;
};

struct msa_i2c_data {
    struct i2c_client 		*client;
    struct acc_hw 			*hw;
    struct hwmsen_convert   	cvt;
    
    struct data_resolution 	*reso;
    atomic_t                		trace;
    atomic_t                		suspend;
    atomic_t                		selftest;
    s16                     		cali_sw[MSA_AXES_NUM+1];

    s8                      		offset[MSA_AXES_NUM+1]; 
    s16                     		data[MSA_AXES_NUM+1];

#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    	early_drv;
#endif     
};

static struct data_resolution msa_data_resolution[] = {
    {{ 1, 0}, 1024},// 1024 表示1g
};

//I2C型号信息
#if !MTK_ANDROID_M
static struct i2c_board_info      msa_i2c_boardinfo = { I2C_BOARD_INFO(MSA_DRV_NAME, MSA_I2C_ADDR>>1) };
#endif

static bool sensor_power = false;
static struct GSENSOR_VECTOR3D gsensor_gain, gsensor_offset;
static MSA_HANDLE              msa_handle;
static int msa_init_flag =0;

#if MTK_ANDROID_M
static struct acc_hw accel_cust;
static struct acc_hw *hw = &accel_cust;
#endif
//static MSA_HANDLE              msa_handle;
extern int Log_level;
/*----------------------------------------------------------------------------*/
#define MI_DATA(format, ...)            if(DEBUG_DATA&Log_level){printk(KERN_ERR MI_TAG format "\n", ## __VA_ARGS__);}
#define MI_MSG(format, ...)             if(DEBUG_MSG&Log_level){printk(KERN_ERR MI_TAG format "\n", ## __VA_ARGS__);}
#define MI_ERR(format, ...)             if(DEBUG_ERR&Log_level){printk(KERN_ERR MI_TAG format "\n", ## __VA_ARGS__);}
#define MI_FUN                          if(DEBUG_FUNC&Log_level){printk(KERN_ERR MI_TAG "%s is called, line: %d\n", __FUNCTION__,__LINE__);}
#define MI_ASSERT(expr)                 \
	if (!(expr)) {\
		printk(KERN_ERR "Assertion failed! %s,%d,%s,%s\n",\
			__FILE__, __LINE__, __func__, #expr);\
	}
/*----------------------------------------------------------------------------*/
//MSA 初始化结果信息
#if MTK_AUTO_MODE
static int msa_local_init(void);
static int msa_local_remove(void);
#if MTK_ANDROID_M
static struct acc_init_info msa_init_info = {
#else
static struct sensor_init_info msa_init_info = {
#endif
    .name = "msa",
    .init = msa_local_init,
    .uninit = msa_local_remove,
};
extern struct acc_hw *msa_get_cust_acc_hw(void);
#else
static int msa_platform_probe(struct platform_device *pdev); 
static int msa_platform_remove(struct platform_device *pdev);
static struct platform_driver msa_gsensor_driver = {
    .driver     = {
        .name  = MSA_PLATFORM_NAME,
        .owner = THIS_MODULE,
    },	
    .probe      = msa_platform_probe,
    .remove     = msa_platform_remove,    
};
#endif
/*----------------------------------------------------------------------------*/
#if MSA_OFFSET_TEMP_SOLUTION
/* 校准文件存储*/
static char OffsetFileName[] = "/data/misc/msaGSensorOffset.txt";
static char OffsetFolerName[] = "/data/misc/msaGSensor/";
static int bCaliResult = -1; 
#define OFFSET_STRING_LEN               26
struct work_info
{
    char        tst1[20];
    char        tst2[20];
    char        buffer[OFFSET_STRING_LEN];
    struct      workqueue_struct *wq;
    struct      delayed_work read_work;
    struct      delayed_work write_work;
    struct      completion completion;
    int         len;
    int         rst; 
};

static struct work_info m_work_info = {{0}};
/*************************************************
Function: sensor_write_work
Description: MSA 在PROBE时写SENSOR工作数据
Input: 
Output: 
Return: 无
*************************************************/
static void sensor_write_work( struct work_struct *work )
{
    struct work_info*   pWorkInfo;
    struct file         *filep;
    mm_segment_t        orgfs;
    int                 ret;   

    orgfs = get_fs();
    set_fs(KERNEL_DS);

    pWorkInfo = container_of((struct delayed_work*)work, struct work_info, write_work);
    if (pWorkInfo == NULL){            
            MI_ERR("get pWorkInfo failed!");       
            return;
    }
    
    filep = filp_open(OffsetFileName, O_RDWR|O_CREAT, 0600);
    if (IS_ERR(filep)){
        MI_ERR("write, sys_open %s error!!.\n", OffsetFileName);
        ret =  -1;
    }
    else
    {   
        filep->f_op->write(filep, pWorkInfo->buffer, pWorkInfo->len, &filep->f_pos);
        filp_close(filep, NULL);
        ret = 0;        
    }
    
    set_fs(orgfs);   
    pWorkInfo->rst = ret;
    complete( &pWorkInfo->completion );
}

/*************************************************
Function: sensor_read_work
Description: MSA 在PROBE时读SENSOR工作数据
Input: 
Output: 
Return: 无
*************************************************/
static void sensor_read_work( struct work_struct *work )
{
    mm_segment_t orgfs;
    struct file *filep;
    int ret; 
    struct work_info* pWorkInfo;
        
    orgfs = get_fs();
    set_fs(KERNEL_DS);
    
    pWorkInfo = container_of((struct delayed_work*)work, struct work_info, read_work);
    if (pWorkInfo == NULL){            
        MI_ERR("get pWorkInfo failed!");       
        return;
    }
 
    filep = filp_open(OffsetFileName, O_RDONLY, 0600);
    if (IS_ERR(filep)){
        MI_MSG("read, sys_open %s error!!.\n",OffsetFileName);
        set_fs(orgfs);
        ret =  -1;
    }
    else{
        filep->f_op->read(filep, pWorkInfo->buffer,  sizeof(pWorkInfo->buffer), &filep->f_pos);
        filp_close(filep, NULL);    
        set_fs(orgfs);
        ret = 0;
    }

    pWorkInfo->rst = ret;
    complete( &(pWorkInfo->completion) );
}

/*************************************************
Function: sensor_sync_read
Description: MSA 同步读SENSOR 补偿工作数据
Input: 
Output: 
Return: 无
*************************************************/
static int sensor_sync_read(u8* offset)
{
    int     err;
    int     off[MSA_OFFSET_LEN] = {0};
    struct work_info* pWorkInfo = &m_work_info;
     
    init_completion( &pWorkInfo->completion );
    queue_delayed_work( pWorkInfo->wq, &pWorkInfo->read_work, msecs_to_jiffies(0) );
    err = wait_for_completion_timeout( &pWorkInfo->completion, msecs_to_jiffies( 2000 ) );
    if ( err == 0 ){
        MI_ERR("wait_for_completion_timeout TIMEOUT");
        return -1;
    }

    if (pWorkInfo->rst != 0){
        MI_ERR("work_info.rst  not equal 0");
        return pWorkInfo->rst;
    }
    
    sscanf(m_work_info.buffer, "%x,%x,%x,%x,%x,%x,%x,%x,%x", &off[0], &off[1], &off[2], &off[3], &off[4], &off[5],&off[6], &off[7], &off[8]);

    offset[0] = (u8)off[0];
    offset[1] = (u8)off[1];
    offset[2] = (u8)off[2];
    offset[3] = (u8)off[3];
    offset[4] = (u8)off[4];
    offset[5] = (u8)off[5];
    offset[6] = (u8)off[6];
    offset[7] = (u8)off[7];
    offset[8] = (u8)off[8];
    
    return 0;
}

/*************************************************
Function: sensor_sync_write
Description: MSA 同步写SENSOR 补偿工作数据
Input: 
Output: 
Return: 无
*************************************************/
static int sensor_sync_write(u8* off)
{
    int err = 0;
    struct work_info* pWorkInfo = &m_work_info;
       
    init_completion( &pWorkInfo->completion );
    
    sprintf(m_work_info.buffer, "%x,%x,%x,%x,%x,%x,%x,%x,%x\n", off[0],off[1],off[2],off[3],off[4],off[5],off[6],off[7],off[8]);
    
    pWorkInfo->len = sizeof(m_work_info.buffer);
        
    queue_delayed_work( pWorkInfo->wq, &pWorkInfo->write_work, msecs_to_jiffies(0) );
    err = wait_for_completion_timeout( &pWorkInfo->completion, msecs_to_jiffies( 2000 ) );
    if ( err == 0 ){
        MI_ERR("wait_for_completion_timeout TIMEOUT");
        return -1;
    }

    if (pWorkInfo->rst != 0){
        MI_ERR("work_info.rst  not equal 0");
        return pWorkInfo->rst;
    }
    
    return 0;
}

/*************************************************
Function: check_califolder_exist
Description: MSA判断校准文件是否存在
Input: 
Output: 
Return: 1 TURE, 0 FALSE
*************************************************/
static int check_califolder_exist(void)
{
    mm_segment_t     orgfs;
    struct  file *filep;
        
    orgfs = get_fs();
    set_fs(KERNEL_DS);

    filep = filp_open(OffsetFolerName, O_RDONLY, 0600);
    if (IS_ERR(filep)) {
        MI_ERR("%s read, sys_open %s error!!.\n",__func__,OffsetFolerName);
        set_fs(orgfs);
        return 0;
    }

    filp_close(filep, NULL);    
    set_fs(orgfs); 

    return 1;
}

/*************************************************
Function: support_fast_auto_cali
Description: MSA是否支持快速校准
Input: 
Output: 
Return: 1 TURE, 0 FALSE
*************************************************/
static int support_fast_auto_cali(void)
{
#if MSA_SUPPORT_FAST_AUTO_CALI
    return (FACTORY_BOOT == get_boot_mode());
#else
    return 0;
#endif
}
#endif

/*************************************************
Function: get_address
Description: MSA获取片选地址
Input: 
Output: 
Return: 地址 初始化成功, -1 初始化失败
*************************************************/
static int get_address(PLAT_HANDLE handle)
{
    if(NULL == handle){
        MI_ERR("chip init failed !\n");
		    return -1;
    }
			
	return ((struct i2c_client *)handle)->addr; 		
}

/*************************************************
Function: msa_resetCalibration
Description: MSA复位校准数据为空
Input: 
Output: 
Return: 0
*************************************************/
static int msa_resetCalibration(struct i2c_client *client)
{
    struct msa_i2c_data *obj = i2c_get_clientdata(client);    

    MI_FUN;
  
    memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
    return 0;     
}

/*************************************************
Function: msa_readCalibration
Description: MSA读取校准数据
Input: 
Output: 
Return: 0
*************************************************/
static int msa_readCalibration(struct i2c_client *client, int *dat)
{
    struct msa_i2c_data *obj = i2c_get_clientdata(client);

    MI_FUN;

    dat[obj->cvt.map[MSA_AXIS_X]] = obj->cvt.sign[MSA_AXIS_X]*obj->cali_sw[MSA_AXIS_X];
    dat[obj->cvt.map[MSA_AXIS_Y]] = obj->cvt.sign[MSA_AXIS_Y]*obj->cali_sw[MSA_AXIS_Y];
    dat[obj->cvt.map[MSA_AXIS_Z]] = obj->cvt.sign[MSA_AXIS_Z]*obj->cali_sw[MSA_AXIS_Z];                        
                                       
    return 0;
}

/*************************************************
Function: msa_writeCalibration
Description: MSA写校准数据
Input: 
Output: 
Return: 0 TURE, -1 FALSE
*************************************************/
static int msa_writeCalibration(struct i2c_client *client, int dat[MSA_AXES_NUM])
{
    struct msa_i2c_data *obj = i2c_get_clientdata(client);
    int err = 0;
    int cali[MSA_AXES_NUM];


    MI_FUN;
    if(!obj || ! dat)
    {
        MI_ERR("null ptr!!\n");
        return -EINVAL;
    }
    else
    {
        cali[obj->cvt.map[MSA_AXIS_X]] = obj->cvt.sign[MSA_AXIS_X]*obj->cali_sw[MSA_AXIS_X];
        cali[obj->cvt.map[MSA_AXIS_Y]] = obj->cvt.sign[MSA_AXIS_Y]*obj->cali_sw[MSA_AXIS_Y];
        cali[obj->cvt.map[MSA_AXIS_Z]] = obj->cvt.sign[MSA_AXIS_Z]*obj->cali_sw[MSA_AXIS_Z]; 
        cali[MSA_AXIS_X] += dat[MSA_AXIS_X];
        cali[MSA_AXIS_Y] += dat[MSA_AXIS_Y];
        cali[MSA_AXIS_Z] += dat[MSA_AXIS_Z];

        obj->cali_sw[MSA_AXIS_X] += obj->cvt.sign[MSA_AXIS_X]*dat[obj->cvt.map[MSA_AXIS_X]];
        obj->cali_sw[MSA_AXIS_Y] += obj->cvt.sign[MSA_AXIS_Y]*dat[obj->cvt.map[MSA_AXIS_Y]];
        obj->cali_sw[MSA_AXIS_Z] += obj->cvt.sign[MSA_AXIS_Z]*dat[obj->cvt.map[MSA_AXIS_Z]];
    } 
	
	mdelay(1);
	
    return err;
}

/*************************************************
Function: msa_readChipInfo
Description: MSA获取芯片信息
Input: 
Output: 
Return: 0 TURE, -1,-2 FALSE
*************************************************/
static int msa_readChipInfo(struct i2c_client *client, char *buf, int bufsize)
{
    if((NULL == buf)||(bufsize<=30)) {
        return -1;
    }

    if(NULL == client) {
        *buf = 0;
        return -2;
    }

    sprintf(buf, "%s\n", "msa");
		
    return 0;
}

/*************************************************
Function: msa_setPowerMode
Description: MSA设置工作模式
Input: 
Output: 
Return: 0 enable, 1 disable
*************************************************/
static int msa_setPowerMode(struct i2c_client *client, bool enable)
{
    int ret;
    
    MI_MSG ("msa_setPowerMode(), enable = %d", enable);
    ret = msa_set_enable(client,enable);  
    if (ret == 0){
        sensor_power = enable;
    }
    return ret;
}

/*************************************************
Function: msa_readSensorData
Description: MSA读取ACC值
Input: 
Output: 
Return: 0 TURE, -1,-2,-3 FALSE
*************************************************/
static int msa_readSensorData(struct i2c_client *client, char *buf)
{    
    struct msa_i2c_data *obj = (struct msa_i2c_data*)i2c_get_clientdata(client);
    unsigned char databuf[20];
    int acc[MSA_AXES_NUM];
    int res = 0;
    memset(databuf, 0, sizeof(unsigned char)*10);

    if(NULL == buf)
    {
        return -1;
    }
    if(NULL == client)
    {
        *buf = 0;
        return -2;
    }

    if(sensor_power == false)
    {
        res = msa_setPowerMode(client, true);
        if(res)
        {
            MI_ERR("Power on msa error %d!\n", res);
        }
        msleep(20);
    }
    
	res = msa_read_data(client, &(obj->data[MSA_AXIS_X]),&(obj->data[MSA_AXIS_Y]),&(obj->data[MSA_AXIS_Z]));
    if(res) 
    {        
        MI_ERR("I2C error: ret value=%d", res);
        return -3;
    }
    else
    {
	#if MSA_OFFSET_TEMP_SOLUTION
        if( bLoad != FILE_EXIST) 
	#endif    
        {
            obj->data[MSA_AXIS_X] += obj->cali_sw[MSA_AXIS_X];
            obj->data[MSA_AXIS_Y] += obj->cali_sw[MSA_AXIS_Y];
            obj->data[MSA_AXIS_Z] += obj->cali_sw[MSA_AXIS_Z];
        }
        
        acc[obj->cvt.map[MSA_AXIS_X]] = obj->cvt.sign[MSA_AXIS_X]*obj->data[MSA_AXIS_X];
        acc[obj->cvt.map[MSA_AXIS_Y]] = obj->cvt.sign[MSA_AXIS_Y]*obj->data[MSA_AXIS_Y];
        acc[obj->cvt.map[MSA_AXIS_Z]] = obj->cvt.sign[MSA_AXIS_Z]*obj->data[MSA_AXIS_Z];
        
	      #if MSA_OFFSET_TEMP_SOLUTION
        if( bLoad != FILE_EXIST) 
	      #endif    
        {				
                 if(abs(obj->cali_sw[MSA_AXIS_Z])> 1300)
                 acc[obj->cvt.map[MSA_AXIS_Z]] = acc[obj->cvt.map[MSA_AXIS_Z]] - 2048;
		    }
		            
#if MSA_STK_TEMP_SOLUTION
      if(bzstk)
			   acc[MSA_AXIS_Z] =squareRoot(1024*1024 - acc[MSA_AXIS_X]*acc[MSA_AXIS_X] - acc[MSA_AXIS_Y]*acc[MSA_AXIS_Y]); 	
#endif

        MI_DATA("msa data map: %d, %d, %d!\n", acc[MSA_AXIS_X], acc[MSA_AXIS_Y], acc[MSA_AXIS_Z]);
        
        acc[MSA_AXIS_X] = acc[MSA_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
        acc[MSA_AXIS_Y] = acc[MSA_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
        acc[MSA_AXIS_Z] = acc[MSA_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;        

        sprintf(buf, "%04x %04x %04x", acc[MSA_AXIS_X], acc[MSA_AXIS_Y], acc[MSA_AXIS_Z]);
        
        MI_DATA( "msa data mg: x= %d, y=%d, z=%d\n",  acc[MSA_AXIS_X],acc[MSA_AXIS_Y],acc[MSA_AXIS_Z]); 
    }
    
    return 0;
}

/*************************************************
Function: msa_readSensorData
Description: MSA读取ACC原始数据
Input: 
Output: 
Return: 0 TURE,其他FALSE
*************************************************/
static int msa_readRawData(struct i2c_client *client, char *buf)
{
	struct msa_i2c_data *obj = (struct msa_i2c_data*)i2c_get_clientdata(client);
	int res = 0;

	if (!buf || !client) {
		return EINVAL;
	}
	
       res = msa_read_data(client, &(obj->data[MSA_AXIS_X]),&(obj->data[MSA_AXIS_Y]),&(obj->data[MSA_AXIS_Z])); 
       if(res) {        
		MI_ERR("I2C error: ret value=%d", res);
		return EIO;
	}
	else {
		sprintf(buf, "%04x %04x %04x", obj->data[MSA_AXIS_X], obj->data[MSA_AXIS_Y], obj->data[MSA_AXIS_Z]);
	
	}
	
	return 0;
}

/*************************************************
Function: msa_misc_ioctl
Description: MSA与MTK上层的各种IOCTL通信
Input: 
Output: 
Return: 0 TURE,其他FALSE
*************************************************/
static long msa_misc_ioctl( struct file *file,unsigned int cmd, unsigned long arg)
{
    struct i2c_client *client = msa_handle;
    struct msa_i2c_data *obj = (struct msa_i2c_data*)i2c_get_clientdata(client);    
    char strbuf[MSA_BUFSIZE] = {0};
    void __user *data;
    struct SENSOR_DATA sensor_data;
    int err = 0;
    int cali[3]={0};

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
        MI_ERR("access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
        return -EFAULT;
    }
    
    switch (cmd) {  
 
    case GSENSOR_IOCTL_INIT:
        MI_MSG("IOCTRL --- GSENSOR_IOCTL_INIT\n");

	 err = msa_chip_resume(client);
	 if(err) {
		MI_ERR("chip resume fail!!\n");
		return -EFAULT;
	 }        
        break;

    case GSENSOR_IOCTL_READ_CHIPINFO:
        MI_MSG("IOCTRL --- GSENSOR_IOCTL_READ_CHIPINFO\n");
        data = (void __user *) arg;
        if(data == NULL)
        {
            err = -EINVAL;
            break;      
        }

	msa_readChipInfo(client,strbuf,MSA_BUFSIZE);
		
        if(copy_to_user(data, strbuf, strlen(strbuf)+1))
        {
            err = -EFAULT;
            break;
        }                 
        break;      

    case GSENSOR_IOCTL_READ_SENSORDATA:
        MI_MSG("IOCTRL --- GSENSOR_IOCTL_READ_SENSORDATA\n");
        data = (void __user *) arg;
        if(data == NULL)
        {
            err = -EINVAL;
            break;      
        }
        
        msa_readSensorData(client, strbuf);
        if(copy_to_user(data, strbuf, strlen(strbuf)+1))
        {
            err = -EFAULT;
            break;      
        }                 
        break;

    case GSENSOR_IOCTL_READ_GAIN:
        MI_MSG("IOCTRL --- GSENSOR_IOCTL_READ_GAIN\n");
        data = (void __user *) arg;
        if(data == NULL)
        {
            err = -EINVAL;
            break;      
        }            
        
        if(copy_to_user(data, &gsensor_gain, sizeof(struct GSENSOR_VECTOR3D)))
        {
            err = -EFAULT;
            break;
        }                 
        break;

    case GSENSOR_IOCTL_READ_OFFSET:
        MI_MSG("IOCTRL --- GSENSOR_IOCTL_READ_OFFSET\n");
        data = (void __user *) arg;
        if(data == NULL)
        {
            err = -EINVAL;
            break;      
        }
        
        if(copy_to_user(data, &gsensor_offset, sizeof(struct GSENSOR_VECTOR3D)))
        {
            err = -EFAULT;
            break;
        }                 
        break;

    case GSENSOR_IOCTL_READ_RAW_DATA:
        MI_MSG("IOCTRL --- GSENSOR_IOCTL_READ_RAW_DATA\n");
        data = (void __user *) arg;
        if(data == NULL)
        {
            err = -EINVAL;
            break;      
        }
        msa_readRawData(client, strbuf);
        if(copy_to_user(data, strbuf, strlen(strbuf)+1))
        {
            err = -EFAULT;
            break;      
        }
        break;      

    case GSENSOR_IOCTL_SET_CALI:
        MI_MSG("IOCTRL --- GSENSOR_IOCTL_SET_CALI\n");
        data = (void __user*)arg;
        if(data == NULL)
        {
            err = -EINVAL;
            break;      
        }
        
        if(copy_from_user(&sensor_data, data, sizeof(sensor_data)))
        {
            err = -EFAULT;
            break;      
        }

        if(atomic_read(&obj->suspend))
        {
            MI_ERR("Perform calibration in suspend state!!\n");
            err = -EINVAL;
        }
        else
        {
            cali[MSA_AXIS_X] = sensor_data.x * obj->reso->sensitivity / GRAVITY_EARTH_1000;
            cali[MSA_AXIS_Y] = sensor_data.y * obj->reso->sensitivity / GRAVITY_EARTH_1000;
            cali[MSA_AXIS_Z] = sensor_data.z * obj->reso->sensitivity / GRAVITY_EARTH_1000;              
            err = msa_writeCalibration(client, cali);
        }
        break;

    case GSENSOR_IOCTL_CLR_CALI:
        MI_MSG("IOCTRL --- GSENSOR_IOCTL_CLR_CALI\n");
        err = msa_resetCalibration(client);
        break;

    case GSENSOR_IOCTL_GET_CALI:
        MI_MSG("IOCTRL --- GSENSOR_IOCTL_GET_CALI\n");
        data = (void __user*)arg;
        if(data == NULL)
        {
            err = -EINVAL;
            break;      
        }
        err = msa_readCalibration(client, cali);
        if(err)
        {
            break;
        }
        
        sensor_data.x = cali[MSA_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
        sensor_data.y = cali[MSA_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
        sensor_data.z = cali[MSA_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
        if(copy_to_user(data, &sensor_data, sizeof(sensor_data)))
        {
            err = -EFAULT;
            break;
        }        
        break;  
           
    default:
        return -EINVAL;
    }

    return err;
}

#ifdef CONFIG_COMPAT
static long msa_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long err = 0;

	void __user *arg32 = compat_ptr(arg);

	if (!file->f_op || !file->f_op->unlocked_ioctl)
		return -ENOTTY;

	switch (cmd) {
	case COMPAT_GSENSOR_IOCTL_READ_SENSORDATA:
		if (arg32 == NULL) {
			err = -EINVAL;
			break;
		}

		err =
		    file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_READ_SENSORDATA,
					       (unsigned long)arg32);
		if (err) {
			MI_ERR("GSENSOR_IOCTL_READ_SENSORDATA unlocked_ioctl failed.");
			return err;
		}
		break;
	case COMPAT_GSENSOR_IOCTL_SET_CALI:
		if (arg32 == NULL) {
			err = -EINVAL;
			break;
		}

		err =
		    file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_SET_CALI, (unsigned long)arg32);
		if (err) {
			MI_ERR("GSENSOR_IOCTL_SET_CALI unlocked_ioctl failed.");
			return err;
		}
		break;
	case COMPAT_GSENSOR_IOCTL_GET_CALI:
		if (arg32 == NULL) {
			err = -EINVAL;
			break;
		}

		err =
		    file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_GET_CALI, (unsigned long)arg32);
		if (err) {
			MI_ERR("GSENSOR_IOCTL_GET_CALI unlocked_ioctl failed.");
			return err;
		}
		break;
	case COMPAT_GSENSOR_IOCTL_CLR_CALI:
		if (arg32 == NULL) {
			err = -EINVAL;
			break;
		}

		err =
		    file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_CLR_CALI, (unsigned long)arg32);
		if (err) {
			MI_ERR("GSENSOR_IOCTL_CLR_CALI unlocked_ioctl failed.");
			return err;
		}
		break;

	default:
		MI_ERR("unknown IOCTL: 0x%08x\n", cmd);
		err = -ENOIOCTLCMD;
		break;

	}

	return err;
}
#endif

/*----------------------------------------------------------------------------*/
static const struct file_operations msa_misc_fops = {
        .owner = THIS_MODULE,
        .unlocked_ioctl = msa_misc_ioctl,
		#ifdef CONFIG_COMPAT
		.compat_ioctl = msa_compat_ioctl,
		#endif
};

static struct miscdevice misc_msa = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = MSA_MISC_NAME,
        .fops = &msa_misc_fops,
};


/*************************************************
Function: msa_enable_show
Description: MSA使能标志
Input: 
Output: 
Return: 0 TURE, !0 FALSE
*************************************************/
static ssize_t msa_enable_show(struct device_driver *ddri, char *buf)
{
    int ret;
    char bEnable;
    struct i2c_client *client = msa_handle;

    MI_FUN;
	
    ret = msa_get_enable(client, &bEnable);   
    if (ret < 0){
        ret = -EINVAL;
    }
    else{
        ret = sprintf(buf, "%d\n", bEnable);
    }

    return ret;
}

/*************************************************
Function: msa_enable_store
Description: MSA使能存储
Input: 
Output: 
Return: 0 TURE, !0 FALSE
*************************************************/
static ssize_t msa_enable_store(struct device_driver *ddri, const char *buf, size_t count)
{
    int ret;
    char bEnable;
    unsigned long enable;
    struct i2c_client *client = msa_handle;

    if (buf == NULL){
        return -1;
    }

    enable = simple_strtoul(buf, NULL, 10);    
    bEnable = (enable > 0) ? true : false;

    ret = msa_set_enable (client, bEnable);
    if (ret < 0){
        ret = -EINVAL;
    }
    else{
        ret = count;
    }

    return ret;
}


/*************************************************
Function: msa_axis_data_show
Description: MSA数据信息显示
Input: 
Output: 
Return: 0 TURE, !0 FALSE
*************************************************/
static ssize_t msa_axis_data_show(struct device_driver *ddri, char *buf)
{
    int result;
    short x,y,z;
    int count = 0;

    result = msa_read_data(msa_handle, &x, &y, &z);
    if (result == 0)
        count += sprintf(buf+count, "x= %d;y=%d;z=%d\n", x,y,z);
    else
        count += sprintf(buf+count, "reading failed!");

    return count;
}

/*************************************************
Function: msa_reg_data_show
Description: MSA寄存器信息显示
Input: 
Output: 
Return: 寄存器信息
*************************************************/
static ssize_t msa_reg_data_show(struct device_driver *ddri, char *buf)
{
    MSA_HANDLE          handle = msa_handle;
        
    return msa_get_reg_data(handle, buf);
}

/*************************************************
Function: msa_reg_data_store
Description: MSA寄存器信息存储
Input: 
Output: 
Return: 寄存器总数显示
*************************************************/
static ssize_t msa_reg_data_store(struct device_driver *ddri, const char *buf, size_t count)
{
    int                 addr, data;
    int                 result;

    sscanf(buf, "0x%x, 0x%x\n", &addr, &data);
    
    result = msa_register_write(msa_handle, addr, data);
    
    MI_ASSERT(result==0);

    MI_MSG("set[0x%x]->[0x%x]\n",addr,data);	

    return count;
}
/*************************************************
Function: msa_log_level_show
Description: MSA平均数据信息显示
Input: 
Output: 
Return: 
*************************************************/
static ssize_t msa_log_level_show(struct device_driver *ddri, char *buf)
{
    int ret;

    ret = sprintf(buf, "%d\n", Log_level);

    return ret;
}
/*************************************************
Function: msa_log_levelmsa_log_level_store_show
Description: MSA平均数据信息存储
Input: 
Output: 
Return: 
*************************************************/
static ssize_t msa_log_level_store(struct device_driver *ddri, const char *buf, size_t count)
{
    Log_level = simple_strtoul(buf, NULL, 10);
    return count;
}


/*************************************************
Function: msa_primary_offset_show
Description: MSA主要的偏移数据显示
Input: 
Output: 
Return: 
*************************************************/
static ssize_t msa_primary_offset_show(struct device_driver *ddri, char *buf){    
    int x=0,y=0,z=0;
   
    msa_get_primary_offset(msa_handle,&x,&y,&z);

	  return sprintf(buf, "x=%d ,y=%d ,z=%d\n",x,y,z);
}

/*************************************************
Function: msa_version_show
Description: MSA客户版本信息显示
Input: 
Output: 
Return: 
*************************************************/
static ssize_t msa_version_show(struct device_driver *ddri, char *buf)
{
	return sprintf(buf, "%s_%s\n", DRI_VER, CORE_VER);
}
/*----------------------------------------------------------------------------*/
static ssize_t msa_vendor_show(struct device_driver *ddri, char *buf)
{
    return sprintf(buf, "%s\n", "MEMSING");
}

#if MSA_OFFSET_TEMP_SOLUTION
/*************************************************
Function: msa_offset_show
Description: MSA校准数据信息显示
Input: 
Output: 
Return: 
*************************************************/
static ssize_t msa_offset_show(struct device_driver *ddri, char *buf)
{
    ssize_t count = 0;
    
    if(bLoad==FILE_EXIST)
    	count += sprintf(buf,"%s",m_work_info.buffer);   
    else
    	count += sprintf(buf,"%s","Calibration file not exist!\n");

    return count;
}
/*************************************************
Function: msa_calibrate_show
Description: MSA校准结果显示
Input: 
Output: 
Return: 
*************************************************/
static ssize_t msa_calibrate_show(struct device_driver *ddri, char *buf)
{
    int ret;       

    ret = sprintf(buf, "%d\n", bCaliResult);   
    return ret;
}
/*************************************************
Function: msa_calibrate_store
Description: MSA校准存储
Input: 
Output: 
Return: 
*************************************************/
static ssize_t msa_calibrate_store(struct device_driver *ddri, const char *buf, size_t count)
{
    signed char     z_dir = 0;
   
    z_dir = simple_strtol(buf, NULL, 10);
    bCaliResult = msa_calibrate(msa_handle,z_dir);
    
    return count;
}
#endif
/*----------------------------------------------------------------------------*/
#if FILTER_AVERAGE_ENHANCE
/*************************************************
Function: msa_average_enhance_show
Description: MSA平均过滤数据提高显示
Input: 
Output: 
Return: 
*************************************************/
static ssize_t msa_average_enhance_show(struct device_driver *ddri, char *buf)
{
    int       ret = 0;
    struct msa_filter_param_s    param = {0};

    ret = msa_get_filter_param(&param);
    ret |= sprintf(buf, "%d %d %d\n", param.filter_param_l, param.filter_param_h, param.filter_threhold);

    return ret;
}
/*************************************************
Function: msa_average_enhance_store
Description: MSA平均过滤数据提高存储
Input: 
Output: 
Return: 
*************************************************/
static ssize_t msa_average_enhance_store(struct device_driver *ddri, const char *buf, size_t count)
{ 
    int       ret = 0;
    struct msa_filter_param_s    param = {0};
    
    sscanf(buf, "%d %d %d\n", &param.filter_param_l, &param.filter_param_h, &param.filter_threhold);
    
    ret = msa_set_filter_param(&param);
    
    return count;
}
#endif 
/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(enable,          S_IRUGO | S_IWUSR,  msa_enable_show,             msa_enable_store);
static DRIVER_ATTR(axis_data,       S_IRUGO,  msa_axis_data_show,          NULL);
static DRIVER_ATTR(reg_data,        S_IWUSR | S_IRUGO,  msa_reg_data_show,           msa_reg_data_store);
static DRIVER_ATTR(log_level,       S_IWUSR | S_IRUGO,  msa_log_level_show,          msa_log_level_store);
static DRIVER_ATTR(primary_offset,  S_IRUGO,  msa_primary_offset_show,     NULL);
static DRIVER_ATTR(vendor,          S_IRUGO,  msa_vendor_show,             NULL);
static DRIVER_ATTR(version,         S_IRUGO,  msa_version_show,            NULL); 
#if MSA_OFFSET_TEMP_SOLUTION
static DRIVER_ATTR(offset,          S_IRUGO,  msa_offset_show,             NULL);
static DRIVER_ATTR(calibrate_msaGSensor,       S_IWUSR | S_IRUGO,  msa_calibrate_show,          msa_calibrate_store);
#endif
#if FILTER_AVERAGE_ENHANCE
static DRIVER_ATTR(average_enhance, S_IWUGO | S_IRUGO,  msa_average_enhance_show,    msa_average_enhance_store);
#endif 
/*----------------------------------------------------------------------------*/
static struct driver_attribute *msa_attributes[] = { 
    &driver_attr_enable,
    &driver_attr_axis_data,
    &driver_attr_reg_data,
    &driver_attr_log_level,
    &driver_attr_primary_offset,    
    &driver_attr_vendor,
    &driver_attr_version,
#if MSA_OFFSET_TEMP_SOLUTION
    &driver_attr_offset,    
    &driver_attr_calibrate_msaGSensor,
#endif
#if FILTER_AVERAGE_ENHANCE
    &driver_attr_average_enhance,
#endif     
};

/*************************************************
Function: msa_create_attr
Description: MSA创建标志
Input: 
Output: 
Return: 
*************************************************/
static int msa_create_attr(struct device_driver *driver) 
{
    int idx, err = 0;
    int num = (int)(sizeof(msa_attributes)/sizeof(msa_attributes[0]));
    if (driver == NULL)
    {
        return -EINVAL;
    }

    for(idx = 0; idx < num; idx++)
    {
		err = driver_create_file(driver, msa_attributes[idx]);
        if(err)
        {            
            MI_MSG("driver_create_file (%s) = %d\n", msa_attributes[idx]->attr.name, err);
            break;
        }
    }    
    return err;
}
/*************************************************
Function: msa_delete_attr
Description: MSA注销标志
Input: 
Output: 
Return: 
*************************************************/
static int msa_delete_attr(struct device_driver *driver)
{
    int idx ,err = 0;
    int num = (int)(sizeof(msa_attributes)/sizeof(msa_attributes[0]));

    if(driver == NULL)
    {
        return -EINVAL;
    }

    for(idx = 0; idx < num; idx++)
    {
        driver_remove_file(driver, msa_attributes[idx]);
    }

    return err;
}

/*************************************************
Function: msa_power
Description: MSA工作模式
Input: 
Output: 
Return: 
*************************************************/
static void msa_power(struct acc_hw *hw, unsigned int on) 
{
    static unsigned int power_on = 0;

    MI_MSG("power %s\n", on ? "on" : "off");

#if !MTK_ANDROID_M
    if(hw->power_id != POWER_NONE_MACRO)       
    {        
        MI_MSG("power %s\n", on ? "on" : "off");
        if(power_on == on)    
        {
            MI_MSG("ignore power control: %d\n", on);
        }
        else if(on)   
        {
            if(!hwPowerOn(hw->power_id, hw->power_vol, "msa"))
            {
                MI_ERR("power on fails!!\n");
            }
        }
        else    
        {
            if (!hwPowerDown(hw->power_id, "msa"))
            {
                MI_ERR("power off fail!!\n");
            }              
        }
    }
#endif
    power_on = on;    
}

#ifndef CONFIG_HAS_EARLYSUSPEND
/*************************************************
Function: msa_suspend
Description: MSA挂起模式
Input: 
Output: 
Return: 
*************************************************/
static int msa_suspend(struct i2c_client *client, pm_message_t msg) 
{
	struct msa_i2c_data *obj = i2c_get_clientdata(client);    
	int err = 0;

	MI_FUN;    

	if(msg.event == PM_EVENT_SUSPEND)
	{   
		if(obj == NULL)
		{
			MI_ERR("null pointer!!\n");
			return -EINVAL;
		}
		
		err = msa_set_enable(obj->client, false);
        if(err) {
            return err;
        }
                
		msa_power(obj->hw, 0);
	}
	return err;
}
/*************************************************
Function: msa_resume
Description: MSA恢复模式
Input: 
Output: 
Return: 
*************************************************/
static int msa_resume(struct i2c_client *client)
{
	struct msa_i2c_data *obj = i2c_get_clientdata(client);        
	int err;
	MI_FUN;

	if(obj == NULL)
	{
		MI_ERR("null pointer!!\n");
		return -EINVAL;
	}
	
       err = msa_chip_resume(obj->client);
	if(err) {
		MI_ERR("chip resume fail!!\n");
		return err;
	}

        err = msa_setPowerMode(obj->client, true);
        if(err != 0) {
		return err;
        }		

	msa_power(obj->hw, 1);
    
	atomic_set(&obj->suspend, 0);

	return 0;
}
#else 
/*************************************************
Function: msa_early_suspend
Description: MSA初期挂起
Input: 
Output: 
Return: 
*************************************************/
static void msa_early_suspend(struct early_suspend *h) 
{
	struct msa_i2c_data *obj = container_of(h, struct msa_i2c_data, early_drv);   
	int err;
	MI_FUN;    

	if(obj == NULL)
	{
		MI_ERR("null pointer!!\n");
		return;
	}
	atomic_set(&obj->suspend, 1); 

	err = msa_setPowerMode(obj->client, false);
	if(err) {
		MI_ERR("write power control fail!!\n");
		return;
	}

	sensor_power = false;
	
	msa_power(obj->hw, 0);
}

/*************************************************
Function: msa_late_resume
Description: MSA后期恢复
Input: 
Output: 
Return: 
*************************************************/
static void msa_late_resume(struct early_suspend *h)
{
	struct msa_i2c_data  *obj = container_of(h, struct msa_i2c_data, early_drv);         
	int                     err;

	MI_FUN;

	if(obj == NULL) {
		MI_ERR("null pointer!!\n");
		return;
	}

        err = msa_chip_resume(obj->client);
	if(err) {
		MI_ERR("chip resume fail!!\n");
		return;
	}


        err = msa_setPowerMode(obj->client, true);
        if(err != 0) {
		return err;
        }		

	msa_power(obj->hw, 1);

	atomic_set(&obj->suspend, 0);    
}
#endif 

/*************************************************
Function: msa_operate
Description: MSA操作命令
Input: 
Output: 
Return: 
*************************************************/
static int msa_operate(void* self, uint32_t command, void* buff_in, int size_in,
        void* buff_out, int size_out, int* actualout)
{
    int err = 0;
    int value;    
    struct msa_i2c_data *priv = (struct msa_i2c_data*)self;
    struct hwm_sensor_data* gsensor_data;
    char buff[MSA_BUFSIZE];
    
    switch (command)
    {
        case SENSOR_DELAY:
            MI_MSG("msa_operate, command is SENSOR_DELAY");

			  if((buff_in == NULL) || (size_in < sizeof(int)))
			  {
				 MI_ERR("Set delay parameter error!\n");
				 err = -EINVAL;
			  }
			  else
			  {
				 value = *(int *)buff_in;

				 err = msa_set_odr(priv->client,value); 
				 if(err) {
					 MI_ERR("msa_set_odr failed !");
					 err = -EINVAL;
				 }								
			 	}						
            break;

        case SENSOR_ENABLE:
            MI_MSG("msa_operate enable gsensor\n");
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                MI_ERR("msa_operate Enable sensor parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                value = *(int *)buff_in;
                
                MI_MSG("msa_operate, command is SENSOR_ENABLE, value = %d\n", value);
                
                if(((value == 0) && (sensor_power == false)) ||((value == 1) && (sensor_power == true)))
                {
                    MI_MSG("Gsensor device have updated!\n");
                }
                else
                {
                    err = msa_setPowerMode( priv->client, !sensor_power);
                }
            }
            break;

        case SENSOR_GET_DATA:
            MI_MSG("msa_operate, command is SENSOR_GET_DATA\n");
            if((buff_out == NULL) || (size_out< sizeof(struct hwm_sensor_data)))
            {
                MI_MSG("get sensor data parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                gsensor_data = (struct hwm_sensor_data *)buff_out;
               
                msa_readSensorData(priv->client, buff);  
                sscanf(buff, "%x %x %x", &gsensor_data->values[0], 
                &gsensor_data->values[1], &gsensor_data->values[2]);
                
                gsensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;                
                gsensor_data->value_divide = 1000;                
            }
            break;
        default:
                MI_MSG("gsensor operate function no this parameter %d!\n", command);
                err = -1;
            break;
    }
    
    return err;
}
/*************************************************
Function: i2c_smbus_read
Description: I2C总线读取
Input: 
Output: 
Return: 
*************************************************/
int i2c_smbus_read(PLAT_HANDLE handle, u8 addr, u8 *data)
{
    int                 res = 0;
    struct i2c_client   *client = (struct i2c_client*)handle;
    
    *data = i2c_smbus_read_byte_data(client, addr);
    
    return res;
}
/*************************************************
Function: i2c_smbus_read_block
Description: I2C总线块读取
Input: 
Output: 
Return: 
*************************************************/
int i2c_smbus_read_block(PLAT_HANDLE handle, u8 addr, u8 count, u8 *data)
{
    int                 res = 0;
    struct i2c_client   *client = (struct i2c_client*)handle;
    
    res = i2c_smbus_read_i2c_block_data(client, addr, count, data);
    
    return res;
}
/*************************************************
Function: i2c_smbus_write
Description: I2C总线写
Input: 
Output: 
Return: 
*************************************************/
int i2c_smbus_write(PLAT_HANDLE handle, u8 addr, u8 data)
{
    int                 res = 0;
    struct i2c_client   *client = (struct i2c_client*)handle;
    
    res = i2c_smbus_write_byte_data(client, addr, data);
    
    return res;
}
/********************************************
Function: msdelay
Description: 延时函数
Input: 
Output: 
Return: 
*************************************************/
void msdelay(int ms)
{
    mdelay(ms);
}

#if MTK_ANDROID_M
/* if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL */
static int msa_open_report_data(int open)
{
	/* should queuq work to report event if  is_report_input_direct=true */
	return 0;
}

/* if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL */
static int msa_enable_nodata(int en)
{
	int res = 0;
	int retry = 0;
	char bEnable = false;

	if (1 == en)
		bEnable = true;
	if (0 == en)
		bEnable = false;

	for (retry = 0; retry < 3; retry++) {
		res = msa_setPowerMode(msa_handle, bEnable);
		if (res == 0) {
			MI_ERR("msa_SetPowerMode done\n");
			break;
		}
		MI_ERR("msa_SetPowerMode fail\n");
	}

	if (res != 0) {
		MI_ERR("msa_SetPowerMode fail!\n");
		return -1;
	}
	MI_MSG("msa_enable_nodata OK!\n");
	return 0;
}

static int msa_set_delay(u64 ns)
{
	int value = 0;

	value = (int)ns/1000/1000;
	MI_MSG("msaset_delay (%d), chip only use 1024HZ\n", value);
	return 0;
}

static int msa_get_data(int *x , int *y, int *z, int *status)
{
	char buff[MSA_BUFSIZE];
	int ret;

	msa_readSensorData(msa_handle, buff);
	ret = sscanf(buff, "%x %x %x", x, y, z);
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;

	return 0;
}
#endif

#if MSA_OFFSET_TEMP_SOLUTION
MSA_GENERAL_OPS_DECLARE(ops_handle, i2c_smbus_read, i2c_smbus_read_block, i2c_smbus_write, sensor_sync_write, sensor_sync_read, check_califolder_exist,get_address,support_fast_auto_cali,msdelay, printk, sprintf);
#else
MSA_GENERAL_OPS_DECLARE(ops_handle, i2c_smbus_read, i2c_smbus_read_block, i2c_smbus_write, NULL, NULL, NULL,get_address,NULL,msdelay, printk, sprintf);
#endif

/********************************************
Function: msa_probe
Description: MSA取样函数
Input: 
Output: 
Return: 
*************************************************/
static int msa_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int                 result;
    struct msa_i2c_data *obj;
    struct hwmsen_object sobj;    
	unsigned char chip_id=0;
	unsigned char i=0;	
#if MTK_ANDROID_M
	struct acc_control_path ctl = {0};
	struct acc_data_path data = {0};
#endif 

    MI_FUN;
    
	printk("lipeiyang 555\n");
	client->addr = 0x26;
    
    if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
    {
        MI_ERR("kzalloc failed!");
        result = -ENOMEM;
        goto exit;
    }   
    
	printk("lipeiyang 666\n");
    memset(obj, 0, sizeof(struct msa_i2c_data));
	
    obj->client = client;
    i2c_set_clientdata(client,obj);	
#if MTK_ANDROID_M
    obj->hw = hw;
#else
#if MTK_AUTO_MODE
    obj->hw = msa_get_cust_acc_hw();	
#else
    obj->hw = get_cust_acc_hw();	
#endif
#endif 
	result = hwmsen_get_convert(obj->hw->direction, &obj->cvt);
    if(result)
    {
        MI_ERR("invalid direction: %d\n", obj->hw->direction);
        goto exit;
    }   
	
	printk("lipeiyang 777\n");
    obj->reso = &msa_data_resolution[0];
    gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = obj->reso->sensitivity;

    result = msa_resetCalibration(client);
    if(result != 0) {
	   return result;
    }	

    atomic_set(&obj->trace, 0);
    atomic_set(&obj->suspend, 0);

    if(msa_install_general_ops(&ops_handle)){
        MI_ERR("Install ops failed !\n");
        goto exit_init_failed;
    }

#if MSA_OFFSET_TEMP_SOLUTION
    m_work_info.wq = create_singlethread_workqueue( "oo" );
    if(NULL==m_work_info.wq) {
        MI_ERR("Failed to create workqueue !");
        goto exit_init_failed;
    }
    
    INIT_DELAYED_WORK( &m_work_info.read_work, sensor_read_work );
    INIT_DELAYED_WORK( &m_work_info.write_work, sensor_write_work );
#endif

	i2c_smbus_read((PLAT_HANDLE) client, NSA_REG_WHO_AM_I, &chip_id);	
	if(chip_id != 0x13){
        for(i=0;i<5;i++){
			mdelay(5); 
		    i2c_smbus_read((PLAT_HANDLE) client, NSA_REG_WHO_AM_I, &chip_id);
            if(chip_id == 0x13)
                break;				
		}
		if(i == 5)
	        client->addr = 0x27;
	}
    
    msa_handle = msa_core_init(client);
    if(NULL == msa_handle){
        MI_ERR("chip init failed !\n");
        goto exit_init_failed;        
    }

    result = misc_register(&misc_msa);
    if (result) {
        MI_ERR("%s: misc register failed !\n", __func__);
        goto exit_misc_device_register_failed;
    }

#if MTK_AUTO_MODE    
    result = msa_create_attr(&(msa_init_info.platform_diver_addr->driver));
    if(result)
    {
        MI_ERR("create attribute result = %d\n", result);
        result = -EINVAL;
	 goto exit_create_attr_failed;
    }
#else
    result = msa_create_attr(&msa_gsensor_driver.driver);
    if(result)
    {
        MI_ERR("create attribute result = %d\n", result);
        result = -EINVAL;		
        goto exit_create_attr_failed;
    }
#endif    
  
    sobj.self = obj;
    sobj.polling = 1;
    sobj.sensor_operate =msa_operate;
    result = hwmsen_attach(ID_ACCELEROMETER, &sobj);
    if(result)
    {
        MI_ERR("attach fail = %d\n", result);
        goto exit_kfree;
    }
    
#ifdef CONFIG_HAS_EARLYSUSPEND
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
	obj->early_drv.suspend  = msa_early_suspend,
	obj->early_drv.resume   = msa_late_resume,    
	register_early_suspend(&obj->early_drv);
#endif

#if MTK_ANDROID_M
    ctl.is_use_common_factory = false;
	ctl.open_report_data = msa_open_report_data;
	ctl.enable_nodata = msa_enable_nodata;
	ctl.set_delay  = msa_set_delay;
	ctl.is_report_input_direct = false;
	ctl.is_support_batch = obj->hw->is_batch_supported;
	result = acc_register_control_path(&ctl);
	if (result) {
		MI_ERR("register acc control path err\n");
		goto exit_kfree;
	}
	data.get_data = msa_get_data;
	data.vender_div = 1000;
	result = acc_register_data_path(&data);
	if (result) {
		MI_ERR("register acc data path err= %d\n", result);
		goto exit_kfree;
	}
#endif
    msa_init_flag = 0;

	printk("lipeiyang 123456\n");
    return result;
exit_create_attr_failed:
	misc_deregister(&misc_msa);
	exit_misc_device_register_failed:
	exit_init_failed:
	msa_init_flag = -1;	
	exit_kfree:
	kfree(obj);
	exit:
	MI_ERR("%s: err = %d\n", __func__, result);        
	return result;
}
/********************************************
Function: msa_remove
Description: MSA注销函数
Input: 
Output: 
Return: 
*************************************************/
static int  msa_remove(struct i2c_client *client)
{
    int err = 0;	

#if MTK_AUTO_MODE    
    err = msa_delete_attr(&(msa_init_info.platform_diver_addr->driver));
    if(err)
    {
        MI_ERR("msa_delete_attr fail: %d\n", err);
    }
#else
    err = msa_delete_attr(&msa_gsensor_driver.driver);
    if(err)
    {
        MI_ERR("msa_delete_attr fail: %d\n", err);
    }
#endif 
    misc_deregister(&misc_msa);
    msa_handle = NULL;
    i2c_unregister_device(client);
    kfree(i2c_get_clientdata(client));    
    
    return 0;
}
#if !MTK_ANDROID_M
/********************************************
Function: msa_detect
Description: MSA检测函数
Input: 
Output: 
Return: 
*************************************************/
static int msa_detect(struct i2c_client *new_client,int kind,struct i2c_board_info *info)
{
      struct i2c_adapter *adapter = new_client->adapter;

      MI_MSG("msa_detect, bus[%d] addr[0x%x]\n", adapter->nr,new_client->addr);

      if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;

       strcpy(info->type, MSA_DRV_NAME);

       return 0;
}
/*----------------------------------------------------------------------------*/
#endif
static const struct i2c_device_id msa_id[] = {
    { MSA_DRV_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, msa_id);

//#if MTK_ANDROID_23
//static unsigned short msa_force[] = {0x00, MSA_I2C_ADDR, I2C_CLIENT_END, I2C_CLIENT_END};
//static const unsigned short *const msa_forces[] = { msa_force, NULL };
//static struct i2c_client_address_data msa_addr_data = { .forces = msa_forces,};
//#endif

#ifdef CONFIG_OF
static const struct of_device_id accel_of_match[] = {
	#if 0	// gsensor 兼容
	{.compatible = "mediatek,gsensor_msa"},	
	#else
	{.compatible = "mediatek,gsensor"},
	#endif
	{},
};
#endif
static struct i2c_driver msa_driver = {
    .driver = {
        .name    = MSA_DRV_NAME,
        .owner   = THIS_MODULE,
#ifdef CONFIG_OF
	    .of_match_table = accel_of_match,
#endif
    },    
    .probe       = msa_probe,
    .remove      = msa_remove,
#if !MTK_ANDROID_M	
    .detect	     = msa_detect,
#endif
#if !defined(CONFIG_HAS_EARLYSUSPEND)        
    .suspend     = msa_suspend,
    .resume      = msa_resume,
#endif
    .id_table    = msa_id,
//#if MTK_ANDROID_23
//    .address_data = &msa_addr_data,    
//#endif
};

/*----------------------------------------------------------------------------*/
#if MTK_AUTO_MODE
/********************************************
Function: msa_local_init
Description: MSA本地初始化
Input: 
Output: 
Return: 
*************************************************/
static int msa_local_init(void) 
{
#if !MTK_ANDROID_M
#if MTK_AUTO_MODE
    struct acc_hw *hw = msa_get_cust_acc_hw();
#else
    struct acc_hw *hw = get_cust_acc_hw();
#endif	
#endif	
    MI_FUN;

    msa_power(hw, 1);

	printk("lipeiyang 333\n");
    if(i2c_add_driver(&msa_driver))
    {
        MI_ERR("add driver error\n");
        return -1;
    }

	printk("lipeiyang 444 msa_init_flag = %d\n",msa_init_flag);
	if(-1 == msa_init_flag)
	{
	   return -1;
	}		

    return 0;
}
/*----------------------------------------------------------------------------*/
/********************************************
Function: msa_local_init
Description: MSA本地注销
Input: 
Output: 
Return: 
*************************************************/
static int msa_local_remove(void)
{
#if !MTK_ANDROID_M
#if MTK_AUTO_MODE
    struct acc_hw *hw =msa_get_cust_acc_hw();
#else
    struct acc_hw *hw =get_cust_acc_hw();
#endif
#endif
    MI_FUN;    
	
    msa_power(hw, 0);    
	
    i2c_del_driver(&msa_driver);
	
    return 0;
}
/*----------------------------------------------------------------------------*/
#else
/********************************************
Function: msa_local_init
Description: MSA平台检测
Input: 
Output: 
Return: 
*************************************************/
static int msa_platform_probe(struct platform_device *pdev) 
{
#if !MTK_ANDROID_M	
    struct acc_hw *hw = get_cust_acc_hw();
#endif
    MI_FUN;

    msa_power(hw, 1);

    if(i2c_add_driver(&msa_driver))
    {
        MI_ERR("add driver error\n");
        return -1;
    }

    return 0;
}
/********************************************
Function: msa_platform_remove
Description: MSA平台注销
Input: 
Output: 
Return: 
*************************************************/
static int msa_platform_remove(struct platform_device *pdev)
{
#if !MTK_ANDROID_M
    struct acc_hw *hw = get_cust_acc_hw();
#endif
    MI_FUN;    

    msa_power(hw, 0);
	
    i2c_del_driver(&msa_driver);

    return 0;
}
#endif
/********************************************
Function: msa_init
Description: MSA初始化
Input: 
Output: 
Return: 
*************************************************/
static int __init msa_init(void)
{    
#if MTK_ANDROID_M
	const char *name = "mediatek,msa"; 

	printk("lipeiyang 111\n");
	hw = get_accel_dts_func(name, hw);
	if (!hw)
		MI_ERR("get dts info fail\n");

	printk("lipeiyang 222\n");
	acc_driver_add(&msa_init_info);
#else
#if MTK_AUTO_MODE
    struct acc_hw *hw = msa_get_cust_acc_hw();
#else
    struct acc_hw *hw = get_cust_acc_hw();
#endif 
    
    MI_FUN;

//#if !MTK_ANDROID_23
    i2c_register_board_info(hw->i2c_num, &msa_i2c_boardinfo, 1);
//#endif

#if MTK_AUTO_MODE
    	hwmsen_gsensor_add(&msa_init_info);
#else
    if(platform_driver_register(&msa_gsensor_driver))
    {
        MI_ERR("failed to register driver");
        return -ENODEV;
    }
#endif
#endif

    return 0;
}
/********************************************
Function: msa_init
Description: MSA退出
Input: 
Output: 
Return: 
*************************************************/
static void __exit msa_exit(void)
{    
    MI_FUN;
	
#if MTK_ANDROID_M	
    MI_MSG("msa_exit\n");
#else
#if !MTK_AUTO_MODE
    platform_driver_unregister(&msa_gsensor_driver);
#endif
#endif
}
/*----------------------------------------------------------------------------*/

module_init(msa_init);
module_exit(msa_exit);
MODULE_AUTHOR("MEMSING <lctang@memsing.com>");
MODULE_DESCRIPTION("MEMSING 3-Axis Accelerometer driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

