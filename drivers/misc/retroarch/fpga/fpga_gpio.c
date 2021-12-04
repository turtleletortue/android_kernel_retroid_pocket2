#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/kobject.h>
#include "fpga_gpio.h"

static struct pinctrl_state *fpga_pwr2on, *fpga_pwr2off, 
                            *lcm_vs_enon, *lcm_vs_enoff, *uart_tx, *uart_rx,*hdmi_boost_enon, *hdmi_boost_enoff;
static struct pinctrl       *fpga_gpio = 0; 

void enable_fpga(int able,int module)
{
	if(fpga_gpio == 0)
		return;
	
	if (able)
	{
		if(module & LCM_VS)
			pinctrl_select_state(fpga_gpio, lcm_vs_enon);
		if(module & FPGA_PWR2)
			pinctrl_select_state(fpga_gpio, fpga_pwr2on);
		if(module & HDMI_BOOST)
			pinctrl_select_state(fpga_gpio, hdmi_boost_enon);
	}
	else
	{
		if(module & LCM_VS)
			pinctrl_select_state(fpga_gpio, lcm_vs_enoff);
		if(module & FPGA_PWR2)
			pinctrl_select_state(fpga_gpio, fpga_pwr2off);
		if(module & HDMI_BOOST)
			pinctrl_select_state(fpga_gpio, hdmi_boost_enoff);
	}
	//printk("enable_fpga able:%d  %d\n",able,module);
}
EXPORT_SYMBOL(enable_fpga);

static int fpga_probe(struct platform_device *dev)
{
	int ret = 0;
		
	printk("[fpga] fpga_probe begin!\n");
	
	fpga_gpio = devm_pinctrl_get(&dev->dev);
	
	fpga_pwr2on = pinctrl_lookup_state(fpga_gpio, "fpga_pwr2on");
	if (IS_ERR(fpga_pwr2on)) {
		ret = PTR_ERR(fpga_pwr2on);
		printk("[fpga] dts can not find fpga_pwr2on!\n");
		return ret;
	}
	
	fpga_pwr2off = pinctrl_lookup_state(fpga_gpio, "fpga_pwr2off");
	if (IS_ERR(fpga_pwr2off)) {
		ret = PTR_ERR(fpga_pwr2off);
		printk("[fpga] dts can not find fpga_pwr2off!\n");
		return ret;
	}
	
	lcm_vs_enon = pinctrl_lookup_state(fpga_gpio, "lcm_vs_enon");
	if (IS_ERR(lcm_vs_enon)) {
		ret = PTR_ERR(lcm_vs_enon);
		printk("[fpga] dts can not find lcm_vs_enon!\n");
		return ret;
	}
	
	lcm_vs_enoff = pinctrl_lookup_state(fpga_gpio, "lcm_vs_enoff");
	if (IS_ERR(lcm_vs_enoff)) {
		ret = PTR_ERR(lcm_vs_enoff);
		printk("[fpga] dts can not find lcm_vs_enoff!\n");
		return ret;
	}
	
	uart_tx = pinctrl_lookup_state(fpga_gpio, "uart_tx");
	if (IS_ERR(uart_tx)) {
		ret = PTR_ERR(uart_tx);
		printk("[fpga] dts can not find uart_tx!\n");
		return ret;
	}
	
	uart_rx = pinctrl_lookup_state(fpga_gpio, "uart_rx");
	if (IS_ERR(uart_rx)) {
		ret = PTR_ERR(uart_rx);
		printk("[fpga] dts can not find uart_rx!\n");
		return ret;
	}
	
	ret = pinctrl_select_state(fpga_gpio, uart_rx);
	if (ret)
	{
		printk("[fpga] pinctrl_select_state uart_rx err!\n");
	}
	
	ret = pinctrl_select_state(fpga_gpio, uart_tx);
	if (ret)
	{
		printk("[fpga] pinctrl_select_state uart_tx err!\n");
	}
	#if 0
	lcm_rst_on = pinctrl_lookup_state(fpga_gpio, "lcm_rst_on");
	if (IS_ERR(lcm_rst_on)) {
		ret = PTR_ERR(lcm_rst_on);
		printk("[fpga] dts can not find lcm_rst_on!\n");
		return ret;
	}
	
	lcm_rst_off = pinctrl_lookup_state(fpga_gpio, "lcm_rst_off");
	if (IS_ERR(lcm_rst_off)) {
		ret = PTR_ERR(lcm_rst_off);
		printk("[fpga] dts can not find lcm_rst_off!\n");
		return ret;
	}
	#endif	

	hdmi_boost_enon = pinctrl_lookup_state(fpga_gpio, "hdmi_boost_enon");
	if (IS_ERR(hdmi_boost_enon)) {
		ret = PTR_ERR(hdmi_boost_enon);
		printk("[fpga] dts can not find hdmi_boost_enon!\n");
		return ret;
	}
	
	hdmi_boost_enoff = pinctrl_lookup_state(fpga_gpio, "hdmi_boost_enoff");
	if (IS_ERR(hdmi_boost_enoff)) {
		ret = PTR_ERR(hdmi_boost_enoff);
		printk("[fpga] dts can not find hdmi_boost_enoff!\n");
		return ret;
	}
	return ret;
}

static int fpga_remove(struct platform_device *dev)
{
	return 0;
}

static void fpga_shutdown(struct platform_device *device)
{	/* wake up */
	printk("[fpga] fpga_shutdown begin!\n");
	enable_fpga(0,FPGA_PWR1);
	enable_fpga(0,FPGA_PWR2);
	enable_fpga(0,LCM_VS);
}

struct of_device_id fpga_of_match[] = {
	{ .compatible = "retroarch,gpio_fpga", },
	{},
};

static struct platform_driver fpga_gpio_driver = {
	.probe = fpga_probe,
	.shutdown = fpga_shutdown,
	.remove = fpga_remove,
	.driver = {
			.name = "FPGA_Driver",
			//.pm = &fpga_pm_ops,
			.of_match_table = fpga_of_match,
		   },
};

static int __init fpga_gpio_init(void) 
{
	int ret = 0;

	printk("[fpga] fpga_gpio_init begin!\n");
	
	ret = platform_driver_register(&fpga_gpio_driver);
	if (ret)
		printk("[fpga] platform_driver_register error:(%d)\n", ret);
	else
		printk("[fpga] platform_driver_register done!\n");
	printk("[fpga] fpga_gpio_init done!\n");
	return ret;
}

static void __exit fpga_gpio_exit(void) 
{
	platform_driver_unregister(&fpga_gpio_driver);
	printk("[fpga] fpga_gpio_exit Done!\n");
}

fs_initcall(fpga_gpio_init);
module_exit(fpga_gpio_exit);

MODULE_LICENSE("GPL");
