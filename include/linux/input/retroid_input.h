#ifndef _RETROID_INPUT_H
#define _RETROID_INPUT_H

#define SUCCESS	0
#define FAILURE	-1

#define IOC_MAGIC 'k'

/* ioctl commands */
#define RETROID_INPUT_NONE	            _IO(IOC_MAGIC, 0)
#define RETROID_INPUT_SET_GPIO_PIN	    _IOW(IOC_MAGIC, 1, int)
#define RETROID_INPUT_SET_ADC_CHANNEL	_IOW(IOC_MAGIC, 2, int)

#define BTN_VOLUME_UP_VIRTUAL_GPIO_PIN 1000
#define BTN_VOLUME_DOWN_VIRTUAL_GPIO_PIN 1001

#define DEVICE_NAME "retroid_input"
#define DRIVER_DESC "retroid input"

#define INFO(...)   printk(KERN_INFO __VA_ARGS__);
#define ALERT(...)  printk(KERN_ALERT __VA_ARGS__);

#define RETROID_KEYS_SIZE  32
#define RETROID_ADCS_SIZE   4

extern int retroid_mt_get_gpio_in_base(unsigned int pin);
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int *rawdata);

#endif