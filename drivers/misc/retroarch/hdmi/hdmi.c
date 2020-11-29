#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include <linux/pm.h>
#include <linux/wakelock.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/kobject.h>
#include <linux/timer.h>

#include "../fpga/fpga_gpio.h"

extern int  backlight_brightness_set(int level);
extern void speaker_power(int state);

struct hdmi_data {
	int init;
	int irq;
	int gpio;
	int gpio_debounce;
	struct timer_list async_check_timer;
	
	struct kobject          *k_obj;
	struct workqueue_struct *hdmi_eint_workqueue;
	struct pinctrl          *hdmi_gpio;
	struct pinctrl_state    *hdmi_hpd;
	
	struct work_struct hdmi_eint_work;
	wait_queue_head_t  wait_queue;
};

static struct file_operations hdmi_fops =
{
	.owner   = THIS_MODULE,
}; 

static struct miscdevice hdmi_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "retroarch_hdmi",
	.fops  = &hdmi_fops,
};

enum {
	EINT_PIN_PLUG_OUT = 0,
	EINT_PIN_PLUG_IN
};

static struct hdmi_data drv_data = {0};
static int lcd_on = 1;

static ssize_t sysfs_get_hpd(struct device *dev, struct device_attribute *attr, char * buf)
{
	return sprintf(buf, "%d\n", gpio_get_value(drv_data.gpio));
}

static ssize_t sysfs_set_voice(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	unsigned int value = 0;
	ret = kstrtouint(buf, 0, &value);
	//accdet_hdmi_enable(value);
	printk("[hdmi] sysfs_set_voice:%d\n",value);
	return size;
}

static ssize_t sysfs_get_lcd(struct device *dev, struct device_attribute *attr, char * buf)
{
	return sprintf(buf, "%d\n", lcd_on);
}

static ssize_t sysfs_set_lcd(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	unsigned int value = 0;
	ret = kstrtouint(buf, 0, &value);
	lcd_on = value;
	printk("[hdmi] sysfs_set_lcd:%d\n",value);
	return size;
}

static DEVICE_ATTR(hpd,   0644, sysfs_get_hpd,   NULL);
static DEVICE_ATTR(voice, 0644, NULL, sysfs_set_voice);
static DEVICE_ATTR(lcd, 0644, sysfs_get_lcd, sysfs_set_lcd);

static struct attribute *sysfs_attributes[] = {
    &dev_attr_hpd.attr,
	&dev_attr_voice.attr,
	&dev_attr_lcd.attr,
    NULL
};

static struct attribute_group hdmi_attr_group = {
    .attrs = sysfs_attributes,
};

static void hdmi_sysfs_init(struct kobject *k_obj)
{
	if ((k_obj = kobject_create_and_add("hdmi", NULL)) == NULL ) {
         printk("[hdmi] sys node create error \n");
         return;
    }

	if(sysfs_create_group(k_obj, &hdmi_attr_group) ) {
         printk("[hdmi] sysfs_create_group failed\n");
    }
}

static irqreturn_t hdmi_eint_func(int irq, void *data)
{
	disable_irq_nosync(drv_data.irq);
	queue_work(drv_data.hdmi_eint_workqueue, &(drv_data.hdmi_eint_work));
	return IRQ_HANDLED;
}

static void boot_check_hpd_once(unsigned long a)
{
	printk("[hdmi] boot_check_hpd_once \n");
	hdmi_eint_func(0,0);
}

// 0 is out, 1 is in
int get_hdmi_state(void)
{
	if(drv_data.gpio == 0)
		return -1;
	return gpio_get_value(drv_data.gpio);
}
EXPORT_SYMBOL(get_hdmi_state);

static void hdmi_eint_work_callback(struct work_struct *work)
{
	int  level = 0;
	//char *name = "DEV_NAME=retroarch_hdmi";
	//char *envp[2] = {name, NULL};
	level = gpio_get_value(drv_data.gpio);
	printk("[hdmi] hdmi_eint_work_callback gpio: %d, level: %d.\n", drv_data.gpio, level);
	if (level == EINT_PIN_PLUG_OUT)
	{
		if(lcd_on == 0)
			backlight_brightness_set(500);
		irq_set_irq_type(drv_data.irq, IRQ_TYPE_LEVEL_HIGH);
		printk("[hdmi] hdmi_eint_work_callback HDMI Plug Out.\n");
		speaker_power(1);
	}
	else
	{
		if(lcd_on == 0)
			backlight_brightness_set(0);
		irq_set_irq_type(drv_data.irq, IRQ_TYPE_LEVEL_LOW);
		printk("[hdmi] hdmi_eint_work_callback HDMI Plug In.\n");
		speaker_power(0);
	}
	gpio_set_debounce(drv_data.gpio, 1000 * 1000);
	enable_irq(drv_data.irq);
}

#define WAKE_UP 1

static int hdmi_probe(struct platform_device *dev)
{
	struct device_node *node;
	int ret = 0;
	u32 ints[2] = { 0, 0 };
	node = dev->dev.of_node;
	//ç”³è¯·å”¤é†’ç³»ç»Ÿ
	device_init_wakeup(&dev->dev, WAKE_UP);
	//è®¾ç½®gpio
	drv_data.hdmi_gpio = devm_pinctrl_get(&dev->dev);
	drv_data.hdmi_hpd  = pinctrl_lookup_state(drv_data.hdmi_gpio, "hdmi_hpd");
	if (IS_ERR(drv_data.hdmi_hpd)) {
		ret = PTR_ERR(drv_data.hdmi_hpd);
		printk("[hdmi] dts can not find hdmi_hpd!\n");
		return ret;
	}
	
	ret = pinctrl_select_state(drv_data.hdmi_gpio, drv_data.hdmi_hpd);
	if (ret)
	{
		printk("[hdmi] pinctrl_select_state hdmi_hpd err!\n");
		return ret;
	}
	//è®¾ç½®ä¸­æ–­
	of_property_read_u32_array(node, "interrupts", ints, ARRAY_SIZE(ints));
	
	drv_data.gpio          = ints[0];
	drv_data.gpio_debounce = ints[1];
	
	gpio_set_debounce(drv_data.gpio, 1000 * 1000);
	
	drv_data.hdmi_eint_workqueue = create_singlethread_workqueue("hdmi_eint");
	INIT_WORK(&(drv_data.hdmi_eint_work), hdmi_eint_work_callback);
	init_waitqueue_head(&drv_data.wait_queue);
	drv_data.irq = irq_of_parse_and_map(node, 0);
	//èµ‹å?¼åˆå§‹çŠ¶æ€?
	if (gpio_get_value(drv_data.gpio))
	{
		irq_set_irq_type(drv_data.irq, IRQ_TYPE_LEVEL_LOW);
	}

	ret = request_irq(drv_data.irq, hdmi_eint_func, IRQF_TRIGGER_NONE, "retroarch_hdmi", NULL);
	if (ret != 0) 
	{
		printk("[hdmi] EINT IRQ LINE NOT AVAILABLE\n");
		drv_data.hdmi_eint_workqueue = NULL;
		return -1;
	} 
	else 
	{
		printk("[hdmi] set EINT finished.\n");
	}
	//åˆ›å»ºè°ƒè¯•æ–‡ä»¶
	hdmi_sysfs_init(drv_data.k_obj);
	//æ³¨å†Œè®¾å¤‡
	misc_register(&hdmi_dev);
		
	/*INIT the timer to check first status.*/
	init_timer(&(drv_data.async_check_timer));
	drv_data.async_check_timer.expires = jiffies +  (15 * HZ);
	drv_data.async_check_timer.function = &boot_check_hpd_once;
	drv_data.async_check_timer.data = ((unsigned long)0);
	add_timer(&(drv_data.async_check_timer));
	
	drv_data.init = 1;
	return 0;
}

static int hdmi_suspend(struct device *device)
{		
	printk("[hdmi] hdmi_suspend\n");
	return 0;
}

static int hdmi_resume(struct device *device)
{			
	printk("[hdmi] hdmi_resume\n");
	return 0;
}

static const struct dev_pm_ops hdmi_pm_ops = {
	.suspend = hdmi_suspend,
	.resume = hdmi_resume,
};

static int hdmi_remove(struct platform_device *dev)
{
	misc_deregister(&hdmi_dev);
	
	if (drv_data.hdmi_eint_workqueue)
	{
		destroy_workqueue(drv_data.hdmi_eint_workqueue);
	}
	
	return 0;
}

static void hdmi_shutdown(struct platform_device *device)
{
	
}

struct of_device_id hdmi_of_match[] = {
	{ .compatible = "eint_hdmi_hpd", },
	{},
};

static struct platform_driver hdmi_driver = {
	.probe    = hdmi_probe,
	.shutdown = hdmi_shutdown,
	.remove   = hdmi_remove,
	.driver   = {
			.name = "HDMI_Driver",
			.pm = &hdmi_pm_ops,
			.of_match_table = hdmi_of_match,
		   },
};

static int __init hdmi_init(void) 
{
	int ret = 0;

	printk("[hdmi] hdmi_init begin!\n");
	
	ret = platform_driver_register(&hdmi_driver);
	if (ret)
		printk("[hdmi] platform_driver_register error:(%d)\n", ret);
	else
		printk("[hdmi] platform_driver_register done!\n");

	printk("[hdmi] hdmi_init done!\n");
	
	return ret;

}

static void __exit hdmi_exit(void) 
{
	printk("[hdmi] hdmi_exit\n");
	
	platform_driver_unregister(&hdmi_driver);

	printk("[hdmi] hdmi_exit Done!\n"); 
}

rootfs_initcall(hdmi_init);
module_exit(hdmi_exit);

MODULE_LICENSE("GPL");
