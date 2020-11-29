#ifndef __FPGA_GPIO__
#define __FPGA_GPIO__

#define FPGA_PWR1  (1<<1)
#define FPGA_PWR2  (1<<2)
#define LCM_VS     (1<<3)
#define LCM_RST    (1<<4)
#define HDMI_BOOST (1<<5)


extern void enable_fpga(int able,int module);

#endif
