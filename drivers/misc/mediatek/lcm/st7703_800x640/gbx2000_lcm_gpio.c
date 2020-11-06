
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>

static struct pinctrl_state *fpga_on, *fpga_off, *reset_on, *reset_off,*lcd_vs_on, *lcd_vs_off;;
static struct pinctrl *fpga_gpio; 

enum {
	FPGA_OFF = 0,
	FPGA_ON
};

void enable_fpga(int able)
{
	if(fpga_gpio == 0)
		return;
		
	if (able)
	{
		pinctrl_select_state(fpga_gpio, lcd_vs_on);
		pinctrl_select_state(fpga_gpio, fpga_on);
		pinctrl_select_state(fpga_gpio, reset_on);
	}
	else
	{
		pinctrl_select_state(fpga_gpio, lcd_vs_off);
		pinctrl_select_state(fpga_gpio, fpga_off);
		pinctrl_select_state(fpga_gpio, reset_off);
	}
	
	printk("[fpga] lcm enable_fpga:%d\n",able);
}

/*static void reset_fpga(void)
{
	pinctrl_select_state(fpga_gpio, reset_off);
	pinctrl_select_state(fpga_gpio, reset_on);
}*/

static int fpga_probe(struct platform_device *dev)
{
	int ret = 0;
		
	printk("[fpga] fpga_probe begin!\n");
	
	fpga_gpio = devm_pinctrl_get(&dev->dev);
	
	fpga_on = pinctrl_lookup_state(fpga_gpio, "fpga_on");
	if (IS_ERR(fpga_on)) {
		ret = PTR_ERR(fpga_on);
		printk("[fpga] dts can not find fpga_on!\n");
		return ret;
	}
	
	fpga_off = pinctrl_lookup_state(fpga_gpio, "fpga_off");
	if (IS_ERR(fpga_off)) {
		ret = PTR_ERR(fpga_on);
		printk("[fpga] dts can not find fpga_on!\n");
		return ret;
	}
	
	reset_on = pinctrl_lookup_state(fpga_gpio, "fpga_rst_on");
	if (IS_ERR(reset_on)) {
		ret = PTR_ERR(fpga_on);
		printk("[fpga] dts can not find fpga_on!\n");
		return ret;
	}
	
	reset_off = pinctrl_lookup_state(fpga_gpio, "fpga_rst_off");
	if (IS_ERR(reset_off)) {
		ret = PTR_ERR(fpga_on);
		printk("[fpga] dts can not find fpga_on!\n");
		return ret;
	}

	lcd_vs_on = pinctrl_lookup_state(fpga_gpio, "lcd_vs_enable");
	if (IS_ERR(lcd_vs_on)) {
		ret = PTR_ERR(fpga_on);
		printk("[fpga] dts can not find lcd_vs_on!\n");
		return ret;
	}
	
	lcd_vs_off = pinctrl_lookup_state(fpga_gpio, "lcd_vs_disable");
	if (IS_ERR(lcd_vs_off)) {
		ret = PTR_ERR(fpga_on);
		printk("[fpga] dts can not find lcd_vs_off!\n");
		return ret;
	}
	
	enable_fpga(FPGA_ON);
	return ret;
}


struct of_device_id fpga_of_match[] = {
	{ .compatible = "moorechip,gpio_fpga", },
	{},
};

static struct platform_driver fpga_gpio_driver = {
	.probe = fpga_probe,
	/* .suspend = accdet_suspend, */
	/* .resume = accdet_resume, */
	//.shutdown = fpga_shutdown,
	//.remove = fpga_remove,
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
	printk("[fpga] fpga_gpio_exit\n");
	platform_driver_unregister(&fpga_gpio_driver);

	printk("[fpga] fpga_gpio_exit Done!\n");
}

module_init(fpga_gpio_init);
module_exit(fpga_gpio_exit);

MODULE_LICENSE("GPL");
