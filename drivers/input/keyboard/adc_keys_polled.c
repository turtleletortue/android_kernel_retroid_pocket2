
/*
 *  Driver for buttons on GPIO lines not capable of generating interrupts
 *
 *  Copyright (C) 2007-2010 Gabor Juhos <juhosg@openwrt.org>
 *  Copyright (C) 2010 Nuno Goncalves <nunojpg@gmail.com>
 *
 *  This file was based on: /drivers/input/misc/cobalt_btns.c
 *	Copyright (C) 2007 Yoichi Yuasa <yoichi_yuasa@tripeaks.co.jp>
 *
 *  also was based on: /drivers/input/keyboard/gpio_keys.c
 *	Copyright 2005 Phil Blundell
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/device.h>
#include <linux/kobject.h>
#include <linux/jiffies.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/time.h>

#define DRV_NAME	"adc-keys-polled"
#define SLOW_MODE    500
#define FAST_MODE    2

//#define __ADC_DEBUG__

#ifdef __ADC_DEBUG__
	#define ADCLOG(fmt, arg...) printk(fmt, ##arg);
#else
	#define ADCLOG(fmt, arg...)
#endif

enum key_positon{
	POSITION_RIGHT = 1,
	POSITION_RIGHT_UP,
	POSITION_UP,
	POSITION_LEFT_UP,
	POSITION_LEFT,
	POSITION_LEFT_DOWN,
	POSITION_DOWN,
	POSITION_RIGHT_DOWN,
};

enum key_index{
	INDEX_RIGHT = 0,
	INDEX_UP,
	INDEX_LEFT,
	INDEX_DOWN,
};

struct adc_key_state{
	enum key_index index;
	unsigned int key;
	int press;
};

struct adc_key{
	unsigned int max_radius;   // 最大半径
	unsigned int threshold_radius;   // 阈值半径
	unsigned int config_radius;   // 配置比例，单位百分比
	unsigned int sine_min;            // 相对于45度线的偏离角
	unsigned int sine_max;
	struct adc_key_state key_state[4];
};

struct adc_channel {
    unsigned int channel;
	unsigned int min;
	unsigned int max;
};

struct adc_keys_platform_data{
	int nchannels;
	unsigned int rep:1;
	int  (*enable)(struct device *dev);
	void (*disable)(struct device *dev);
	const char *name;
	struct adc_key key;
	struct adc_channel *channels;
};

struct adc_keys_polled_dev {
	struct input_polled_dev *poll_dev;
	struct device *dev;
	struct adc_keys_platform_data *pdata;
	struct kobject *k_obj;
};

static struct adc_keys_polled_dev *g_dev = NULL;

extern int IMM_GetOneChannelValue_Cali(int Channel, int *voltage);
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int *rawdata);

int my_abs(int x) 
{
    if(x<0) x=0-x;
    return x;
}

static int get_adc_vol(int channel)
{
	int vol = -1;
	int data[4];
	int result = -1;
	
	IMM_GetOneChannelValue(channel, data, &vol);
	result = data[0] * 100 + data[1];
	ADCLOG("get_adc_status get channel[%d]: %d.\n", channel, result);
	
	return result;
}

static ssize_t sysfs_get_radius(struct device *dev, struct device_attribute *attr, char * buf)
{
	struct adc_keys_platform_data *pdata = g_dev->pdata;
	return sprintf(buf, "%d\n", pdata->key.config_radius);
}

static ssize_t sysfs_set_radius(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
	struct adc_keys_platform_data *pdata = g_dev->pdata;
	int ret;
	ret = kstrtouint(buf, 0, &(pdata->key.config_radius));
	return n;
}

static ssize_t sysfs_get_sine_min(struct device *dev, struct device_attribute *attr, char * buf)
{
	struct adc_keys_platform_data *pdata = g_dev->pdata;
	return sprintf(buf, "%d\n", pdata->key.sine_min);
}

static ssize_t sysfs_set_sine_min(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
	struct adc_keys_platform_data *pdata = g_dev->pdata;
	int ret;
	ret = kstrtouint(buf, 0, &(pdata->key.sine_min));
	return n;
}

static ssize_t sysfs_get_sine_max(struct device *dev, struct device_attribute *attr, char * buf)
{
	struct adc_keys_platform_data *pdata = g_dev->pdata;
	return sprintf(buf, "%d\n", pdata->key.sine_max);
}

static ssize_t sysfs_set_sine_max(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
	struct adc_keys_platform_data *pdata = g_dev->pdata;
	int ret;
	ret = kstrtouint(buf, 0, &(pdata->key.sine_max));
	return n;
}

static void adc_keys_get_vol(struct adc_keys_platform_data *pdata,int *x,int *y)
{
	int i;
	int vol;

	for (i = 0; i < pdata->nchannels; i++) {
		vol = get_adc_vol(pdata->channels[i].channel);
		switch(pdata->channels[i].channel){
			case 1:
				*x = vol;
				break;
			case 12:
				*y = vol;
				break;
		}
	}
}

static void adc_auto_calibration_radius(struct adc_keys_platform_data *pdata)
{
	int y = (pdata->channels[0].max - pdata->channels[0].min)/2;
	int x = (pdata->channels[1].max - pdata->channels[1].min)/2;
	pdata->key.max_radius = (x+y)/2;
	pdata->key.threshold_radius = (pdata->key.config_radius*pdata->key.max_radius)/100;
}

static void adc_auto_calibration(struct adc_keys_platform_data *pdata,int i,int vol)
{
	if(vol > pdata->channels[i].max)
	{
		pdata->channels[i].max = vol;
		adc_auto_calibration_radius(pdata);
	}
	
	if(vol < pdata->channels[i].min)
	{
		pdata->channels[i].min = vol;
		adc_auto_calibration_radius(pdata);
	}
}

static void adc_keys_get_xy(struct adc_keys_platform_data *pdata,int *x,int *y)
{
	int i;
	int vol;
	
	for (i = 0; i < pdata->nchannels; i++) {
		vol = get_adc_vol(pdata->channels[i].channel);
		adc_auto_calibration(pdata,i,vol);
		switch(pdata->channels[i].channel){
			case 1:
				*x = vol - (pdata->channels[i].min + pdata->channels[i].max)/2;
				break;
			case 12:
				*y = 0 -(vol - (pdata->channels[i].min + pdata->channels[i].max)/2);
				break;
		}
	}
}

static ssize_t sysfs_get_xy(struct device *dev, struct device_attribute *attr, char * buf)
{
	int x = 0,y = 0;
	int org_x = 0, org_y = 0;
	struct adc_keys_platform_data *pdata = g_dev->pdata;
	adc_keys_get_xy(pdata,&x,&y);
	adc_keys_get_vol(pdata,&org_x,&org_y);
	return sprintf(buf, "x=%d y=%d. channel x=%d y=%d\n", x, y,org_x,org_y);
}

static ssize_t sysfs_get_info(struct device *dev, struct device_attribute *attr, char * buf)
{
	struct adc_keys_platform_data *pdata = g_dev->pdata;
	return sprintf(buf, "threshold_radius=%d config_radius=%d max_radius=%d\n",
					    pdata->key.threshold_radius,pdata->key.config_radius,pdata->key.max_radius);
}

static ssize_t sysfs_get_min_max(struct device *dev, struct device_attribute *attr, char * buf)
{
	struct adc_keys_platform_data *pdata = g_dev->pdata;
	return sprintf(buf, "calibration channel %d min=%d max=%d; channel %d min=%d max=%d\n", 
		                pdata->channels[0].channel,pdata->channels[0].min,pdata->channels[0].max,
		                pdata->channels[1].channel,pdata->channels[1].min,pdata->channels[1].max);
}

static ssize_t sysfs_get_interval(struct device *dev, struct device_attribute *attr, char * buf)
{
	struct input_polled_dev *polldev = g_dev->poll_dev;
	return sprintf(buf, "%d\n ",polldev->poll_interval);
}

static void adc_polldev_queue_work(struct input_polled_dev *dev)
{
	unsigned long delay;

	delay = msecs_to_jiffies(dev->poll_interval);
	if (delay >= HZ)
		delay = round_jiffies_relative(delay);

	queue_delayed_work(system_freezable_wq, &dev->work, delay);
}

static ssize_t sysfs_set_interval(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
	struct input_polled_dev *polldev = g_dev->poll_dev;
	struct input_dev *input = polldev->input;
	unsigned int poll_interval = 50;
	int ret;
	ret = kstrtouint(buf, 0, &poll_interval);
	mutex_lock(&input->mutex);
	//polldev->poll_interval = poll_interval;
	if (input->users) {
		cancel_delayed_work_sync(&polldev->work);
		adc_polldev_queue_work(polldev);
	}
	mutex_unlock(&input->mutex);
	return n;
}

static DEVICE_ATTR(radius, 0644, sysfs_get_radius, sysfs_set_radius);
static DEVICE_ATTR(sine_min, 0644, sysfs_get_sine_min, sysfs_set_sine_min);
static DEVICE_ATTR(sine_max, 0644, sysfs_get_sine_max, sysfs_set_sine_max);
static DEVICE_ATTR(xy, 0644, sysfs_get_xy, NULL);
static DEVICE_ATTR(info, 0644, sysfs_get_info, NULL);
static DEVICE_ATTR(min_max, 0644, sysfs_get_min_max, NULL);
static DEVICE_ATTR(interval, 0644, sysfs_get_interval, sysfs_set_interval);

static struct attribute *sysfs_attributes[] = {
    &dev_attr_radius.attr,
	&dev_attr_sine_min.attr,
	&dev_attr_sine_max.attr,
	&dev_attr_xy.attr,
	&dev_attr_info.attr,
	&dev_attr_min_max.attr,
	&dev_attr_interval.attr,
    NULL
};

static struct attribute_group adc_key_attr_group = {
    .attrs = sysfs_attributes,
};

static void adc_sysfs_init(struct kobject *k_obj)
{
	if ((k_obj = kobject_create_and_add("adc_key", NULL)) == NULL ) {
         printk("sys node create error \n");
         return;
    }

	if(sysfs_create_group(k_obj, &adc_key_attr_group) ) {
         printk("sysfs_create_group failed\n");
    }
}

// only report current position
static int cal_key_position(int x, int y, struct adc_key *key)
{
	unsigned int cur_radius;
	unsigned int sine_abs;
	cur_radius = (unsigned int)int_sqrt(x*x + y*y);
	if(cur_radius < key->threshold_radius)
		return 0;

	sine_abs = (my_abs(y)*1000000)/cur_radius;  // 因内核不支持float类型的除法。
	
	// quadrant 1
	if(x >= 0 && y >= 0)
	{
		if(sine_abs < key->sine_min)
			return POSITION_RIGHT;
		else if(sine_abs > key->sine_min && sine_abs < key->sine_max)
			return POSITION_RIGHT_UP;
		else if(sine_abs > key->sine_max)
			return POSITION_UP;
	}

	// quadrant 2
	if(x <= 0 && y >= 0)
	{
		if(sine_abs < key->sine_min)
			return POSITION_LEFT;
		else if(sine_abs > key->sine_min && sine_abs < key->sine_max)
			return POSITION_LEFT_UP;
		else if(sine_abs > key->sine_max)
			return POSITION_UP;
	}

	// quadrant 3
	if(x <= 0 && y <= 0)
	{
		if(sine_abs < key->sine_min)
			return POSITION_LEFT;
		else if(sine_abs > key->sine_min && sine_abs < key->sine_max)
			return POSITION_LEFT_DOWN;
		else if(sine_abs > key->sine_max)
			return POSITION_DOWN;
	}

	// quadrant 4
	if(x >= 0 && y <= 0)
	{
		if(sine_abs < key->sine_min)
			return POSITION_RIGHT;
		else if(sine_abs > key->sine_min && sine_abs < key->sine_max)
			return POSITION_RIGHT_DOWN;
		else if(sine_abs > key->sine_max)
			return POSITION_DOWN;
	}

	return 0;
}

static void position_to_key_status(char *key_buffer, int position)
{
	switch(position)
	{
		case POSITION_RIGHT:
			key_buffer[INDEX_RIGHT] = 1;
			break;
		case POSITION_RIGHT_UP:
			key_buffer[INDEX_RIGHT] = 1;
			key_buffer[INDEX_UP] = 1;
			break;
		case POSITION_UP:
			key_buffer[INDEX_UP] = 1;
			break;
		case POSITION_LEFT_UP:
			key_buffer[INDEX_UP] = 1;
			key_buffer[INDEX_LEFT] = 1;
			break;
		case POSITION_LEFT:
			key_buffer[INDEX_LEFT] = 1;
			break;
		case POSITION_LEFT_DOWN:
			key_buffer[INDEX_LEFT] = 1;
			key_buffer[INDEX_DOWN] = 1;
			break;
		case POSITION_DOWN:
			key_buffer[INDEX_DOWN] = 1;
			break;
		case POSITION_RIGHT_DOWN:
			key_buffer[INDEX_DOWN] = 1;
			key_buffer[INDEX_RIGHT] = 1;
			break;
	}
}

static void adc_keys_polled_poll(struct input_polled_dev *dev)
{
	struct adc_keys_polled_dev *bdev = dev->private;
	struct adc_keys_platform_data *pdata = bdev->pdata;
	struct adc_key *key = &pdata->key;
	struct input_dev *input = dev->input;
	char new_press_status[4];
	int change = 0;
	int positon;
	int i;
	int x = 0,y = 0;

	adc_keys_get_xy(pdata,&x,&y);
	//ADCLOG("adc_keys_polled_poll x=%d  y=%d\n",x,y);
	positon = cal_key_position(x,y,key);
	memset(new_press_status,0,4);
	position_to_key_status(new_press_status,positon);
	for(i = 0; i < 4; i++){
		if(key->key_state[i].press != new_press_status[i]){
			key->key_state[i].press = new_press_status[i];
			//unsigned int radius = (unsigned int)int_sqrt(x*x + y*y);
			//ADCLOG("adc_keys key:%d press:%d x:%d y:%d r:%d sine:%d\n", key->key_state[i].key, 
			//	key->key_state[i].press, x, y, radius, (my_abs(y)*1000000)/radius)
			input_event(input, EV_KEY, key->key_state[i].key, key->key_state[i].press);
			change = 1;
		}
	}
	if(change)
		input_sync(input);
}

static void adc_keys_polled_open(struct input_polled_dev *dev)
{
	struct adc_keys_polled_dev *bdev = dev->private;
	struct adc_keys_platform_data *pdata = bdev->pdata;

	if (pdata->enable)
		pdata->enable(bdev->dev);
}

static void adc_keys_polled_close(struct input_polled_dev *dev)
{
	struct adc_keys_polled_dev *bdev = dev->private;
	struct adc_keys_platform_data *pdata = bdev->pdata;

	if (pdata->disable)
		pdata->disable(bdev->dev);
}

#ifdef CONFIG_OF
static struct adc_keys_platform_data *adc_keys_polled_get_devtree_pdata(struct device *dev)
{
	struct device_node *node, *pp, *bd;
	struct adc_keys_platform_data *pdata;
	struct adc_channel *channel;
	int nchannels;
	int i, j;
	unsigned int keyboard_value;
	unsigned int index;

	node = dev->of_node;
	if (!node)
		return NULL;

	nchannels = of_get_child_count(node);
	if (nchannels == 0)
		return NULL;

	pdata = devm_kzalloc(dev, sizeof(*pdata) + nchannels * sizeof(*channel), GFP_KERNEL);
	if (!pdata)
		return ERR_PTR(-ENOMEM);

	pdata->channels  = (struct adc_channel *)(pdata + 1);
	pdata->nchannels = nchannels;

	pdata->rep = !!of_get_property(node, "autorepeat", NULL);
	of_property_read_u32(node, "radius", &pdata->key.config_radius);
	of_property_read_u32(node, "sine_min", &pdata->key.sine_min);
	of_property_read_u32(node, "sine_max", &pdata->key.sine_max);

	i = 0;
	j = 0;
	for_each_child_of_node(node, pp) {
		if (!of_find_property(pp, "channel", NULL)) {
			pdata->channels--;
			dev_warn(dev, "Found button without channel\n");
			continue;
		}

		channel = &pdata->channels[i++];
		
		if (of_property_read_u32(pp, "channel", &channel->channel)) {
			dev_err(dev, "Button without channel: 0x%x\n",
				channel->channel);
			return ERR_PTR(-EINVAL);
		}

		ADCLOG("adc_keys_polled_get_devtree_pdata channel: %d.\n", channel->channel);

		if (of_property_read_u32(pp, "min", &channel->min)) {
			dev_err(dev, "Button without min: 0x%x\n",
				channel->min);
			return ERR_PTR(-EINVAL);
		}

		if (of_property_read_u32(pp, "max", &channel->max)) {
			dev_err(dev, "Button without max: 0x%x\n",
				channel->max);
			return ERR_PTR(-EINVAL);
		}

		for_each_child_of_node(pp, bd) {
			if (of_property_read_u32(bd, "index", &index)) {
				dev_err(dev, "Button without index: 0x%x\n", index);
				return ERR_PTR(-EINVAL);
			}
			if (of_property_read_u32(bd, "key", &keyboard_value)) {
				dev_err(dev, "Button without key: 0x%x\n", keyboard_value);
				return ERR_PTR(-EINVAL);
			}
			pdata->key.key_state[index].key = keyboard_value;
			ADCLOG("adc_keys_polled_get_devtree_pdata key: %d.\n",keyboard_value);
		}
	}

	adc_auto_calibration_radius(pdata);
	return pdata;
}

static const struct of_device_id adc_keys_polled_of_match[] = {
	{ .compatible = "adc-keys-polled", },
	{ },
};
MODULE_DEVICE_TABLE(of, adc_keys_polled_of_match);

#else

static inline struct adc_keys_platform_data *
adc_keys_polled_get_devtree_pdata(struct device *dev)
{
	return NULL;
}
#endif

static int adc_keys_polled_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct adc_keys_platform_data *pdata = dev_get_platdata(dev);
	struct adc_keys_polled_dev *bdev;
	struct input_polled_dev    *poll_dev;
	struct input_dev           *input;
	
	size_t size;
	int    error;
	int    i;

	if (!pdata) {
		pdata = adc_keys_polled_get_devtree_pdata(dev);
		if (IS_ERR(pdata))
			return PTR_ERR(pdata);
		
		if (!pdata) {
			dev_err(dev, "missing platform data\n");
			return -EINVAL;
		}
	}

	size = sizeof(struct adc_keys_polled_dev);
	bdev = devm_kzalloc(&pdev->dev, size, GFP_KERNEL);
	if (!bdev) {
		dev_err(dev, "no memory for private data\n");
		return -ENOMEM;
	}
	
	g_dev = bdev;

	poll_dev = devm_input_allocate_polled_device(&pdev->dev);
	if (!poll_dev) {
		dev_err(dev, "no memory for polled device\n");
		return -ENOMEM;
	}

	poll_dev->private       = bdev;
	poll_dev->poll          = adc_keys_polled_poll;
	poll_dev->poll_interval = SLOW_MODE; //ms
	poll_dev->open          = adc_keys_polled_open;
	poll_dev->close         = adc_keys_polled_close;

	input = poll_dev->input;
	input->name = pdev->name;
	input->phys = DRV_NAME"/input0";
	input->id.bustype = BUS_HOST;
	input->id.vendor  = 0xde15;
	input->id.product = 0x1234;
	input->id.version = 0x0100;

	__set_bit(EV_KEY, input->evbit);
	
	if (pdata->rep)
		__set_bit(EV_REP, input->evbit);

	for (i = 0; i < 4; i++) {
		input_set_capability(input, EV_KEY, pdata->key.key_state[i].key);
	}

	bdev->poll_dev = poll_dev;
	bdev->dev      = dev;
	bdev->pdata    = pdata;
	
	platform_set_drvdata(pdev, bdev);

	error = input_register_polled_device(poll_dev);
	if (error) {
		dev_err(dev, "unable to register polled device, err=%d\n",
			error);
		return error;
	}

	adc_sysfs_init(bdev->k_obj);

	return 0;
}

static struct platform_driver adc_keys_polled_driver = {
	.probe	= adc_keys_polled_probe,
	.driver	= {
		.name	= DRV_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(adc_keys_polled_of_match),
	},
};
module_platform_driver(adc_keys_polled_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("dywu <1260339597@qq.com>");
MODULE_DESCRIPTION("Polled ADC Buttons driver");
MODULE_ALIAS("platform:" DRV_NAME);

