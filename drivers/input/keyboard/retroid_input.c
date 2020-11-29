#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>

#include <linux/input.h>
#include <linux/input/retroid_input.h>

typedef struct {
	unsigned int pin;
	unsigned int state;
} retroid_gpio_t;

typedef struct {
	unsigned int channel;
	int value;
} retroid_adc_t;

static int retroid_input_open(struct inode *, struct file *);
static int retroid_input_release(struct inode *, struct file *);
static int retroid_input_ioctl(struct file *, unsigned int, unsigned long);
static ssize_t retroid_input_read(struct file *, char *, size_t, loff_t *);
static ssize_t retroid_input_write(struct file *, const char *, size_t, loff_t *);

static int device_open = 0;

struct cdev *retroid_input_dev;
static int major;
static dev_t dev = MKDEV(0, 0);
static struct class *retroid_input_class;
static struct file_operations fops = {
	.read	    = retroid_input_read,
	.write      = retroid_input_write,
	.open	    = retroid_input_open,
	.unlocked_ioctl	= retroid_input_ioctl,
	.release    = retroid_input_release,
};

static retroid_gpio_t retroid_gpios[RETROID_KEYS_SIZE];
static retroid_adc_t  retroid_adcs[RETROID_ADCS_SIZE];
static retroid_gpio_t *retroid_gpios_ptr;
static retroid_adc_t  *retroid_adcs_ptr;

extern unsigned int retroid_tl2_state;
extern unsigned int retroid_tr2_state;

static int retroid_input_open(struct inode *inodp, struct file *filp)
{
	if (device_open)
		return -EBUSY;

	device_open++;

	retroid_gpios_ptr = retroid_gpios;
	retroid_adcs_ptr  = retroid_adcs;
	memset(retroid_gpios, 0, RETROID_KEYS_SIZE * sizeof(retroid_gpio_t));
	memset(retroid_adcs, 0, RETROID_ADCS_SIZE * sizeof(retroid_adc_t));

	try_module_get(THIS_MODULE);

	return SUCCESS;
}

static int retroid_input_release(struct inode *inodp, struct file *filp)
{
	device_open--;
	module_put(THIS_MODULE);
	return SUCCESS;
}

static inline int get_adc_vol(unsigned int channel)
{
	int vol = -1;
	int data[4];
	IMM_GetOneChannelValue(channel, data, &vol);
	return (data[0] * 100 + data[1]);
}

static void retroid_input_update(void)
{
	retroid_gpios_ptr = retroid_gpios;
	retroid_adcs_ptr  = retroid_adcs;

	while (retroid_gpios_ptr->pin > 0) {
		switch (retroid_gpios_ptr->pin) {
			case BTN_VOLUME_DOWN_VIRTUAL_GPIO_PIN:
				retroid_gpios_ptr->state = retroid_tl2_state;
				break;
			case BTN_VOLUME_UP_VIRTUAL_GPIO_PIN:
				retroid_gpios_ptr->state = retroid_tr2_state;
				break;
			default:
				retroid_gpios_ptr->state = !!retroid_mt_get_gpio_in_base(retroid_gpios_ptr->pin);
				break;
		}

		retroid_gpios_ptr++;
	}
	while (retroid_adcs_ptr->channel > 0) {
		retroid_adcs_ptr->value = get_adc_vol(retroid_adcs_ptr->channel);
		retroid_adcs_ptr++;
	}
}

static ssize_t retroid_input_read(struct file *filp, char *buff, size_t count, loff_t *offp)
{
	ssize_t bytes_read = 0;

	retroid_input_update();

	retroid_gpios_ptr = retroid_gpios;
	retroid_adcs_ptr  = retroid_adcs;

	while (count && retroid_gpios_ptr->pin > 0) {
		put_user(retroid_gpios_ptr->state, buff++);
		
		count--;
		bytes_read++;
		retroid_gpios_ptr++;
	}

	while (count && retroid_adcs_ptr->channel > 0) {
		put_user(retroid_adcs_ptr->value, buff++);

		count--;
		bytes_read++;
		retroid_adcs_ptr++;
	}

	return bytes_read;
}

static ssize_t retroid_input_write(struct file *filp, const char *buff, size_t count, loff_t *offp)
{
	return 0;
}

static int retroid_input_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
		case RETROID_INPUT_SET_GPIO_PIN:
			retroid_gpios_ptr = retroid_gpios;
			while (retroid_gpios_ptr->pin > 0) retroid_gpios_ptr++;
			retroid_gpios_ptr->pin = (unsigned int)arg;
			break;
		case RETROID_INPUT_SET_ADC_CHANNEL:
			retroid_adcs_ptr  = retroid_adcs;
			while (retroid_adcs_ptr->channel > 0) retroid_adcs_ptr++;
			retroid_adcs_ptr->channel = (unsigned int)arg;
			break;	
		default:
			ALERT("Command unknown\n");
			return -EINVAL;
			break;
	}

	return 0;
}

static ssize_t retroid_getinfo_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	int count;
	count = sprintf(buf,"adc channel 1:%d ,channel 12:%d\n",get_adc_vol(1),get_adc_vol(12));
	return count;
}

static DEVICE_ATTR(getinfo, 0664, retroid_getinfo_show, NULL);

static struct device_attribute *retroid_attributes[] = {
	&dev_attr_getinfo,
	NULL
};

static int __init retroid_input_init(void)
{
	struct device_attribute **attrs = retroid_attributes;
	struct device_attribute *attr;
	 struct device * retroid_device;
	int err;
	
	retroid_input_dev = cdev_alloc();

	alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
	major = MAJOR(dev);

	cdev_init(retroid_input_dev, &fops);
	retroid_input_dev->ops = &fops;
	retroid_input_dev->owner = THIS_MODULE;
	cdev_add(retroid_input_dev, dev, 1);

	/* create a struct class structure */
	retroid_input_class = class_create(THIS_MODULE, DEVICE_NAME);
	/* creates a device and registers it with sysfs */
	retroid_device = device_create(retroid_input_class, NULL, MKDEV(major, 0), "%s", DEVICE_NAME);

	while ((attr = *attrs++)) {
		err = device_create_file(retroid_device, attr);
		if (err) {
			//device_destroy(cockroach_class, cockroachdrv_device->devt);
			return err;
		}
	}
	return 0;
}

static void __exit retroid_input_exit(void)
{
	cdev_del(retroid_input_dev);
	unregister_chrdev_region(dev, 1);
	device_destroy(retroid_input_class, MKDEV(major, 0));
	class_unregister(retroid_input_class);
	class_destroy(retroid_input_class);
}

module_init(retroid_input_init);
module_exit(retroid_input_exit);

MODULE_AUTHOR("retroid");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");