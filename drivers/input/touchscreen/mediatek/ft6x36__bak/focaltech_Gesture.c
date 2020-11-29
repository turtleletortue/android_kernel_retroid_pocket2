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
* File Name: Focaltech_Gestrue.c
*
* Author: Xu YongFeng
*
* Created: 2015-01-29
*   
* Modify by mshl on 2015-03-20
*
* Abstract:
*
* Reference:
*
*******************************************************************************/

/*******************************************************************************
* 1.Included header files
*******************************************************************************/
#include "focaltech_core.h"
#if FTS_GESTRUE_EN
#include "ft_gesture_lib.h"
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include<linux/wakelock.h>
#define  TPD_PACKET_LENGTH                     128
#define PROC_READ_DRAWDATA			10


/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/
#define GESTURE_LEFT								0x20
#define GESTURE_RIGHT								0x21
#define GESTURE_UP		    							0x22
#define GESTURE_DOWN								0x23
#define GESTURE_DOUBLECLICK						0x24
#define GESTURE_O		    							0x30
#define GESTURE_W		    							0x31
#define GESTURE_M		   	 						0x32
#define GESTURE_E		    							0x33
#define GESTURE_L		    							0x44
#define GESTURE_S		    							0x46
#define GESTURE_V		    							0x54
#define GESTURE_Z		    							0x65
#define GESTURE_C		    					              0x34
#define FTS_GESTRUE_POINTS 						255
#define FTS_GESTRUE_POINTS_ONETIME  				62
#define FTS_GESTRUE_POINTS_HEADER 				8
#define FTS_GESTURE_OUTPUT_ADRESS 				0xD3
#define FTS_GESTURE_OUTPUT_UNIT_LENGTH 			4
 #define PROC_UPGRADE							0
#define TS_WAKE_LOCK_TIMEOUT		(2 * HZ)
unsigned short gesture_id;

unsigned short Drawdata[15] = {0};
extern struct wake_lock gesture_chrg_lock;

//static unsigned char proc_operate_mode = PROC_UPGRADE;

unsigned char buf[FTS_GESTRUE_POINTS * 2] ;
static unsigned char gesture_point_readbuf[TPD_PACKET_LENGTH];
static struct i2c_client *client01;
/*******************************************************************************
* Private enumerations, structures and unions using typedef
*******************************************************************************/


/*******************************************************************************
* Static variables
*******************************************************************************/
short pointnum 					= 0;
unsigned short coordinate_x[150] 	= {0};
unsigned short coordinate_y[150] 	= {0};

/*******************************************************************************
* Global variable or extern global variabls/functions
*******************************************************************************/

/*******************************************************************************
* Static function prototypes
*******************************************************************************/


/*******************************************************************************
*   Name: fts_Gesture_init
*  Brief:
*  Input:
* Output: None
* Return: None
*******************************************************************************/
 struct fts_gesture_entry {
	 struct attribute attr;
	 ssize_t (*show)(struct kobject *kobj, char *page);
	 ssize_t (*store)(struct kobject *kobj, const char *page, size_t size);
 };
 struct fts_gesture_sysobj {
	 struct kobject kobj;
	 atomic_t enable;
 } gesture_sysobj = {
	 .enable = ATOMIC_INIT(0),
 };
int fts_Gesture_init(struct input_dev *input_dev)
{
        //init_para(480,854,60,0,0);
	input_set_capability(input_dev, EV_KEY, KEY_POWER);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_U); 
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_UP); 
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_DOWN);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_LEFT); 
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_RIGHT); 
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_O);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_E); 
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_M); 
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_L);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_W);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_S); 
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_V);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_Z);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_C);
		
	__set_bit(KEY_GESTURE_RIGHT, input_dev->keybit);
	__set_bit(KEY_GESTURE_LEFT, input_dev->keybit);
	__set_bit(KEY_GESTURE_UP, input_dev->keybit);
	__set_bit(KEY_GESTURE_DOWN, input_dev->keybit);
	__set_bit(KEY_GESTURE_U, input_dev->keybit);
	__set_bit(KEY_GESTURE_O, input_dev->keybit);
	__set_bit(KEY_GESTURE_E, input_dev->keybit);
	__set_bit(KEY_GESTURE_M, input_dev->keybit);
	__set_bit(KEY_GESTURE_W, input_dev->keybit);
	__set_bit(KEY_GESTURE_L, input_dev->keybit);
	__set_bit(KEY_GESTURE_S, input_dev->keybit);
	__set_bit(KEY_GESTURE_V, input_dev->keybit);
	__set_bit(KEY_GESTURE_Z, input_dev->keybit);
	__set_bit(KEY_GESTURE_C, input_dev->keybit);
	return 0;
}

/*******************************************************************************
*   Name: fts_check_gesture
*  Brief:
*  Input:
* Output: None
* Return: None
*******************************************************************************/
static void fts_check_gesture(struct input_dev *input_dev,int gesture_id)
{
	printk("fts gesture_id==0x%x\n ",gesture_id);
       wake_lock_timeout(&gesture_chrg_lock,TS_WAKE_LOCK_TIMEOUT);
	switch(gesture_id)
	{
	        case GESTURE_LEFT:
	                input_report_key(input_dev, KEY_POWER, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_POWER, 0);
	                input_sync(input_dev);
	                break;
	        case GESTURE_RIGHT:
	                input_report_key(input_dev, KEY_POWER, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_POWER, 0);
	                input_sync(input_dev);
			    break;
	        case GESTURE_UP:
	                input_report_key(input_dev, KEY_POWER, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_POWER, 0);
	                input_sync(input_dev);    
	                break;
	        case GESTURE_DOWN:
	                input_report_key(input_dev, KEY_POWER, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_POWER, 0);
	                input_sync(input_dev);
	                break;
	        case GESTURE_DOUBLECLICK:
	                input_report_key(input_dev, KEY_POWER, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_POWER, 0);
	                input_sync(input_dev);
	                break;
	        case GESTURE_O:
	                input_report_key(input_dev, KEY_POWER, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_POWER, 0);
	                input_sync(input_dev);
	                break;
	        case GESTURE_W:
	                input_report_key(input_dev, KEY_POWER, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_POWER, 0);
	                input_sync(input_dev);
	                break;
	        case GESTURE_M:
	                input_report_key(input_dev, KEY_POWER, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_POWER, 0);
	                input_sync(input_dev);
	                break;
	        case GESTURE_E:
	                input_report_key(input_dev, KEY_POWER, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_POWER, 0);
	                input_sync(input_dev);
	                break;
	        case GESTURE_L:
	                input_report_key(input_dev, KEY_POWER, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_POWER, 0);
	                input_sync(input_dev);
	                break;
	        case GESTURE_S:
	                input_report_key(input_dev, KEY_POWER, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_POWER, 0);
	                input_sync(input_dev);
	                break;
	        case GESTURE_V:
	                input_report_key(input_dev, KEY_POWER, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_POWER, 0);
	                input_sync(input_dev);
	                break;
	        case GESTURE_Z:
	                input_report_key(input_dev, KEY_POWER, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_POWER, 0);
	                input_sync(input_dev);
	                break;
		case GESTURE_C:
	                input_report_key(input_dev, KEY_POWER, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_POWER, 0);
	                input_sync(input_dev);
	                break;
	        default:
                       
	                break;
	}

}

 /************************************************************************
*   Name: fts_read_Gestruedata
* Brief: read data from TP register
* Input: no
* Output: no
* Return: fail <0
***********************************************************************/
int fts_read_Gestruedata(void)        //read data from fw.Drawdata[i] sent to mobile************************************************
{
    //unsigned char buf[FTS_GESTRUE_POINTS * 3] = { 0 };
    int ret = -1;
    //int i = 0;
    int gesture_id = 0;

    buf[0] = 0xd3;
    pointnum = 0;

    ret = fts_i2c_read(fts_i2c_client, buf, 1, buf, FTS_GESTRUE_POINTS_HEADER);
        //printk( "tpd read FTS_GESTRUE_POINTS_HEADER.\n");

    if (ret < 0)
    {
        printk( "%s read touchdata failed.\n", __func__);
        return ret;
    }
#if 0
    /* FW */
     if (fts_updateinfo_curr.CHIP_ID==0x54 || fts_updateinfo_curr.CHIP_ID==0x58 || fts_updateinfo_curr.CHIP_ID==0x86 || fts_updateinfo_curr.CHIP_ID==0x87  || fts_updateinfo_curr.CHIP_ID == 0x64)
     {
	 gesture_id = buf[0];
	 pointnum = (short)(buf[1]) & 0xff;
	 buf[0] = 0xd3;

	 if((pointnum * 4 + 2)<255)
	 {
	    	 ret = fts_i2c_read(fts_i2c_client, buf, 1, buf, (pointnum * 4 + 2));
	 }
	 else
	 {
	        ret = fts_i2c_read(fts_i2c_client, buf, 1, buf, 255);
	        ret = fts_i2c_read(fts_i2c_client, buf, 0, buf+255, (pointnum * 4 + 2) -255);
	 }
	 if (ret < 0)
	 {
	       printk( "%s read touchdata failed.\n", __func__);
	       return ret;
	 }

	 fts_check_gesture(fts_input_dev,gesture_id);
	 for(i = 0;i < pointnum;i++)
	 {
	    	coordinate_x[i] =  (((s16) buf[0 + (4 * i+2)]) & 0x0F) <<
	        	8 | (((s16) buf[1 + (4 * i+2)])& 0xFF);
	    	coordinate_y[i] = (((s16) buf[2 + (4 * i+2)]) & 0x0F) <<
	        	8 | (((s16) buf[3 + (4 * i+2)]) & 0xFF);
	 }
	 return -1;
   }
	 #endif
	// other IC's gestrue in driver
	if (0x24 == buf[0])                                      //check is double click or not*********************************************************
	{
	    	gesture_id = 0x24;
	    	fts_check_gesture(fts_input_dev,gesture_id);
		printk( "%d check_gesture gesture_id.\n", gesture_id);
	    return -1;
	}
	gesture_id = buf[0];
	//pointnum = (short)(buf[1]) & 0xff;
	pointnum = 7;
	buf[0] = 0xd3;
	/*if((pointnum * 4 + 8)<255)
	{
		ret = fts_i2c_read(fts_i2c_client, buf, 1, buf, (pointnum * 4 + 8));
	}
	else
	{
	     ret = fts_i2c_read(fts_i2c_client, buf, 1, buf, 255);
	     ret = fts_i2c_read(fts_i2c_client, buf, 0, buf+255, (pointnum * 4 + 8) -255);
	}
	if (ret < 0)
	{
	    printk( "%s read touchdata failed.\n", __func__);
	    return ret;
	}

	//gesture_id = fetch_object_sample(buf, pointnum);//need gesture lib.a
	gesture_id = 0x24;
	*/
	ret = fts_i2c_read(fts_i2c_client, buf, 1, buf, (pointnum * 4 + 2 +6));
	if (ret < 0)
	{
		printk( "%s read touchdata failed.\n", __func__);
		return ret;
	}

	fts_check_gesture(fts_input_dev,gesture_id);
#if 0
	{
	printk( "%d read gesture_id.\n", gesture_id);
			// add debug info
		
	
		printk("Debug gestrue_id = %d \n",gesture_id);
		
		for (i = 0; i < 14; i++)
		{
                    Drawdata[i] = (unsigned short)(((buf[1 + i * 2] & 0xFF) << 8) | (buf[1+1 + i * 2] & 0xFF));
		}

		for (i = 0; i < 14; i++)
		{
                    printk("%d ,", Drawdata[i]);
		}

		printk("Debug end \n");
	
	/*
	for(i = 0;i < pointnum;i++)
	{
	    coordinate_x[i] =  (((s16) buf[0 + (4 * i+8)]) & 0x0F) <<
	        8 | (((s16) buf[1 + (4 * i+8)])& 0xFF);
	    coordinate_y[i] = (((s16) buf[2 + (4 * i+8)]) & 0x0F) <<
	        8 | (((s16) buf[3 + (4 * i+8)]) & 0xFF);
	}
	*/
#endif
	return -1;
}
 /**********************************************************
????
*********************************************************/

 static ssize_t fts_gesture_attr_show(struct kobject *kobj, struct attribute *attr, char *buffer)
 {
	 struct fts_gesture_entry *entry = container_of(attr, struct fts_gesture_entry, attr);
	 return entry->show(kobj, buffer);
 }
 static ssize_t fts_gesture_attr_store(struct kobject *kobj, struct attribute *attr, const char *buffer, size_t size)
 {
	 struct fts_gesture_entry *entry = container_of(attr, struct fts_gesture_entry, attr);
	 return entry->store(kobj, buffer, size);
 }
 static ssize_t fts_gesture_enable_show(struct kobject *kobj, char *buffer)
 {
	 int buf_len=0;
	  buf_len += sprintf(buffer, "%d",tpd->gesture_enable);
	 return buf_len;
 }
 static ssize_t fts_gesture_enable_store(struct kobject *kobj, const char *buffer, size_t size)
 {
	 int res = sscanf(buffer, "%d", &tpd->gesture_enable);
	 if (res != 1) {
		 printk("fts_gesture_enable_store input string :'%s' \n",buffer);
	 } else {
		 printk("[fts_gesture_enable_store] tpd->gesture_enable %d\n", tpd->gesture_enable);
	 }
	 return size;
 }
 
 static ssize_t fts_gesture_debug_show(struct kobject *kobj, char *buffer)
 {
	 ssize_t len = 0;
	 //struct i2c_client *client = (struct i2c_client *)client01;
	 int num_read_chars = 0;
	 int i=0;
	 //unsigned char gesture_point_readbuf_temp[TPD_PACKET_LENGTH]={0};
	// printk("fts_gesture_debug_show start %d\n", proc_operate_mode);
	 //switch (proc_operate_mode)
	 //{
		 //case PROC_READ_DRAWDATA:
		 
		 	 //memcpy(buffer,gesture_point_readbuf_temp,29);
			 gesture_point_readbuf[0] = gesture_id;
			 for(i = 0;i < 28;i++)
			 {
				 gesture_point_readbuf[1 + i] = buf[i+1];
			 }
			 len = num_read_chars = 29;
			 memcpy(buffer,gesture_point_readbuf,num_read_chars);
		 	 //memcpy(gesture_point_readbuf,gesture_point_readbuf_temp,128);
			 //break;
		//default:
			// break;
	 //}
	 printk("fts_gesture_debug_show--end %d\n", num_read_chars);
	
	 return len;
 }
 static ssize_t fts_gesture_debug_store(struct kobject *kobj, const char *buffer, size_t size)
 {
	/* if(sscanf(buffer, "%c", &proc_operate_mode) == 1)
	 {
		 printk("fts_gesture_debug_store success %d size=%d\n",proc_operate_mode,size);
		 printk("fts_gesture_debug_store input string :'%s' \n",buffer);
	 }
	 else
	 {
		 printk("fts_gesture_debug_store error format :'%s'\n",buffer);
		 proc_operate_mode=0xff;
		 return -EFAULT;
	 }
	 switch (proc_operate_mode) {
	 default:
		 break;
	 }*/
	
	 return size;
 }
 static void fts_gesture_sysfs_release(struct kobject *kobj)
 {
	 struct fts_gesture_sysobj		 *ge_sysfs;

	 ge_sysfs=container_of(kobj, struct fts_gesture_sysobj, kobj);

	 kfree(ge_sysfs);
 }

 /*---------------------------------------------------------------------------*/
 struct sysfs_ops fts_gesture_sysfs_ops = {
	 .show	 = fts_gesture_attr_show,
	 .store  = fts_gesture_attr_store,
 };
 /*---------------------------------------------------------------------------*/
 /*---------------------------------------------------------------------------*/
 struct fts_gesture_entry gesture_enable_entry = {
	 { .name = "gesture_enable", .mode = 0664 },
	 fts_gesture_enable_show,
	 fts_gesture_enable_store,
 };
 /*---------------------------------------------------------------------------*/
 struct fts_gesture_entry gesture_debug_entry = {
	 { .name = "debug", .mode = 0664},
	 fts_gesture_debug_show,
	 fts_gesture_debug_store,
 };
 /*---------------------------------------------------------------------------*/
 struct attribute *fts_gesture_attributes[] = {
	 &gesture_enable_entry.attr,  /*enable setting*/
	 &gesture_debug_entry.attr,
	 NULL,
 };
 /*---------------------------------------------------------------------------*/
 struct kobj_type fts_gesture_ktype = {
	 .sysfs_ops = &fts_gesture_sysfs_ops,
	 .release =fts_gesture_sysfs_release,
	 .default_attrs = fts_gesture_attributes,
 };
 /*---------------------------------------------------------------------------*/

 /*---------------------------------------------------------------------------*/
 int fts_gesture_sysfs(struct i2c_client * client)
 {
	 struct fts_gesture_sysobj *obj = &gesture_sysobj;

	 memset(&obj->kobj, 0x00, sizeof(obj->kobj));

	 atomic_set(&obj->enable, 0);

	 obj->kobj.parent = kernel_kobj;
	 if (kobject_init_and_add(&obj->kobj, &fts_gesture_ktype, NULL, "gesture")) {
		 kobject_put(&obj->kobj);
		 printk("error fts_gesture_sysfs ");
		 return -ENOMEM;
	 }
	 kobject_uevent(&obj->kobj, KOBJ_ADD);
	 client01 = client;
	 return 0;
 }

#endif
