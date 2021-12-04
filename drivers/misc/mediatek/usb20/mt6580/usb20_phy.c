
#include <mach/mt_clkmgr.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <linux/spinlock.h>
#include "mtk_musb.h"
#include "musb_core.h"
#include "usb20.h"

#define FRA (48)
#define PARA (28)

#ifdef CONFIG_MTK_UART_USB_SWITCH
int usb_port_mode_temp = 0;
#endif

#ifdef FPGA_PLATFORM
#include <linux/i2c.h>


void USB_PHY_Write_Register8(UINT8 var,  UINT8 addr)
{
	char buffer[2];

	buffer[0] = addr;
	buffer[1] = var;
	i2c_master_send(usb_i2c_client, buffer, 2);
}

UINT8 USB_PHY_Read_Register8(UINT8 addr)
{
	UINT8 var;

	i2c_master_send(usb_i2c_client, &addr, 1);
	i2c_master_recv(usb_i2c_client, &var, 1);
	return var;
}

bool usb_enable_clock(bool enable)
{
	return true;
}

void usb_phy_poweron(void)
{
}

void usb_phy_savecurrent(void)
{
}

void usb_phy_recover(void)
{
	/* usb phy initial sequence */
	USB_PHY_Write_Register8(0x00, 0xFF);
	USB_PHY_Write_Register8(0x04, 0x61);
	USB_PHY_Write_Register8(0x00, 0x68);
	USB_PHY_Write_Register8(0x00, 0x6a);
	USB_PHY_Write_Register8(0x6e, 0x00);
	USB_PHY_Write_Register8(0x0c, 0x1b);
	USB_PHY_Write_Register8(0x44, 0x08);
	USB_PHY_Write_Register8(0x55, 0x11);
	USB_PHY_Write_Register8(0x68, 0x1a);
	USBPHY_CLR8(0x6a, 0xff);

	DBG(0, "[MUSB]addr: 0xFF, value: %x\n", USB_PHY_Read_Register8(0xFF));
	DBG(0, "[MUSB]addr: 0x61, value: %x\n", USB_PHY_Read_Register8(0x61));
	DBG(0, "[MUSB]addr: 0x68, value: %x\n", USB_PHY_Read_Register8(0x68));
	DBG(0, "[MUSB]addr: 0x6a, value: %x\n", USB_PHY_Read_Register8(0x6a));
	DBG(0, "[MUSB]addr: 0x00, value: %x\n", USB_PHY_Read_Register8(0x00));
	DBG(0, "[MUSB]addr: 0x1b, value: %x\n", USB_PHY_Read_Register8(0x1b));
	DBG(0, "[MUSB]addr: 0x08, value: %x\n", USB_PHY_Read_Register8(0x08));
	DBG(0, "[MUSB]addr: 0x11, value: %x\n", USB_PHY_Read_Register8(0x11));
	DBG(0, "[MUSB]addr: 0x1a, value: %x\n", USB_PHY_Read_Register8(0x1a));
	DBG(0, "[MUSB]usb_phy_recover\n");
}

/* BC1.2 */
void Charger_Detect_Init(void)
{
}

void Charger_Detect_Release(void)
{
}

void usb_phy_context_save(void)
{
}

void usb_phy_context_restore(void)
{
}

#ifdef CONFIG_MTK_UART_USB_SWITCH
bool usb_phy_check_in_uart_mode(void)
{
	UINT8 usb_port_mode;

	usb_enable_clock(true);
	udelay(50);

	usb_port_mode = USB_PHY_Read_Register8(0x6B);
	usb_enable_clock(false);

	if ((usb_port_mode == 0x5C) ||
		(usb_port_mode == 0x5E))
		return true;
	else
		return false;
}

void usb_phy_switch_to_uart(void)
{

	if (usb_phy_check_in_uart_mode())
		return;

	/* ALPS00775710 */
	DBG(0, "force to uart mode !!\n");
	/* ALPS00775710 */

	usb_enable_clock(true);
	udelay(50);

	/* RG_USB20_BC11_SW_EN = 1'b0 */
	USBPHY_CLR8(0x1a, 0x80);

	/* Set RG_SUSPENDM to 1 */
	USBPHY_SET8(0x68, 0x08);

	/* force suspendm = 1 */
	USBPHY_SET8(0x6a, 0x04);

	/* Set ru_uart_mode to 2'b01 */
	USBPHY_SET8(0x6B, 0x5C);

	/* Set RG_UART_EN to 1 */
	USBPHY_SET8(0x6E, 0x07);

	/* Set RG_USB20_DM_100K_EN to 1 */
	USBPHY_SET8(0x22, 0x02);
	usb_enable_clock(false);
}

void usb_phy_switch_to_usb(void)
{
	usb_enable_clock(true);
	udelay(50);
	/* clear force_uart_en */
	USBPHY_WRITE8(0x6B, 0x00);
	usb_enable_clock(false);
	usb_phy_poweron();
	/* disable the USB clock turned on in usb_phy_poweron() */
	usb_enable_clock(false);
}
#endif

#else

#ifdef CONFIG_MTK_UART_USB_SWITCH
static bool in_uart_mode;
#endif

static DEFINE_SPINLOCK(musb_reg_clock_lock);

static void enable_phy_clock(bool enable)
{
	/* USB phy 48M clock , UNIVPLL_CON0[26] */
	if (enable) {
		enable_clock(MT_CG_UNIV_48M, "PERI_USB");
		enable_clock(MT_CG_USB_48M, "PERI_USB");
	} else {
		disable_clock(MT_CG_UNIV_48M, "PERI_USB");
		disable_clock(MT_CG_USB_48M, "PERI_USB");
	}
}

bool usb_enable_clock(bool enable)
{
	static int count;
	bool res = TRUE;
	unsigned long flags;

	spin_lock_irqsave(&musb_reg_clock_lock, flags);

	if (enable && count == 0) {
		enable_phy_clock(true);
		res = enable_clock(MT_CG_USB_SW_CG, "PERI_USB");
	} else if (!enable && count == 1) {
		res = disable_clock(MT_CG_USB_SW_CG, "PERI_USB");
		enable_phy_clock(false);
	}

	if (enable)
		count++;
	else
		count = (count == 0) ? 0 : (count-1);

	spin_unlock_irqrestore(&musb_reg_clock_lock, flags);

	DBG(0, "enable(%d), count(%d) res=%d\n", enable, count, res);
	return 1;
}

static void hs_slew_rate_cal(void)
{
	unsigned long data;
	unsigned long x;
	unsigned char value;
	unsigned long start_time, timeout;
	unsigned int timeout_flag = 0;
	/* 4 s1:enable usb ring oscillator. */
	USBPHY_WRITE8(0x15, 0x80);

	/* 4 s2:wait 1us. */
	udelay(1);

	/* 4 s3:enable free run clock */
	USBPHY_WRITE8(0xf00-0x800+0x11, 0x01);
	/* 4 s4:setting cyclecnt. */
	USBPHY_WRITE8(0xf00-0x800+0x01, 0x04);
	/* 4 s5:enable frequency meter */
	USBPHY_SET8(0xf00-0x800+0x03, 0x01);

	/* 4 s6:wait for frequency valid. */
	start_time = jiffies;
	timeout = jiffies + 3 * HZ;

	while (!(USBPHY_READ8(0xf00-0x800+0x10)&0x1)) {
		if (time_after(jiffies, timeout)) {
			timeout_flag = 1;
			break;
		}
	}

	/* 4 s7: read result. */
	if (timeout_flag) {
		DBG(0, "[USBPHY] Slew Rate Calibration: Timeout\n");
		value = 0x4;
	} else{
		data = USBPHY_READ32(0xf00-0x800+0x0c);
		x = ((1024*FRA*PARA)/data);
		value = (unsigned char)(x/1000);
	if ((x-value*1000)/100 >= 5)
		value += 1;
		DBG(0, "[USBPHY]slew calibration:FM_OUT =%lu,x=%lu,value=%d\n", data, x, value);
	}

	/* 4 s8: disable Frequency and run clock. */
	USBPHY_CLR8(0xf00-0x800+0x03, 0x01);
	USBPHY_CLR8(0xf00-0x800+0x11, 0x01);

	/* 4 s9: */
	USBPHY_WRITE8(0x15, value<<4);

	/* 4 s10:disable usb ring oscillator. */
	USBPHY_CLR8(0x15, 0x80);
}

#ifdef CONFIG_MTK_UART_USB_SWITCH
bool usb_phy_check_in_uart_mode(void)
{
	UINT8 usb_port_mode;

	usb_enable_clock(true);
	udelay(50);
	usb_port_mode = USBPHY_READ8(0x6B);
	/* usb_port_mode = 1; */
	usb_enable_clock(false);

	if ((usb_port_mode == 0x5C) ||
		(usb_port_mode == 0x5E) || (usb_port_mode_temp == 1)) {
		usb_port_mode_temp = 1;
		return true;
	}

	return false;
}

void usb_phy_switch_to_uart(void)
{
	if (usb_phy_check_in_uart_mode())
		return;

	usb_enable_clock(true);
	udelay(50);

	/* RG_USB20_BC11_SW_EN = 1'b0 */
	USBPHY_CLR8(0x1a, 0x80);

	/* Set RG_SUSPENDM to 1 */
	USBPHY_SET8(0x68, 0x08);

	/* force suspendm = 1 */
	USBPHY_SET8(0x6a, 0x04);

	/* Set ru_uart_mode to 2'b01 */
	USBPHY_SET8(0x6B, 0x5C);

	/* Set RG_UART_EN to 1 */
	USBPHY_SET8(0x6E, 0x07);

	/* Set RG_USB20_DM_100K_EN to 1 */
	USBPHY_SET8(0x22, 0x02);
	usb_enable_clock(false);

	/*set uart rx path for K2 use*/
	/* mtk_uart_usb_rx_sel(1, 1); */

	/* GPIO Selection set UART#1 */
	/*DRV_WriteReg32(GPIO_BASE_1 + 0x118, 0x02); */	/* set */
}


void usb_phy_switch_to_usb(void)
{
	/* DRV_WriteReg32(GPIO_BASE_1 + 0x11C, 0x02); */	/* clear */
	usb_enable_clock(true);
	udelay(50);
	/* clear force_uart_en */
	USBPHY_WRITE8(0x6B, 0x00);
	usb_enable_clock(false);
	usb_phy_poweron();
	/* disable the USB clock turned on in usb_phy_poweron() */
	usb_enable_clock(false);

	usb_port_mode_temp = 0;

	DBG(0, "usb port value in usb function:%d\n", usb_port_mode_temp);
}
#endif

void usb_phy_poweron(void)
{

	#ifdef CONFIG_MTK_UART_USB_SWITCH
	if (usb_phy_check_in_uart_mode())
		return;
	#endif

	/* 4 s1: enable USB MAC clock. */
	usb_enable_clock(true);

	/* 4 s2: wait 50 usec for PHY3.3v/1.8v stable. */
	udelay(50);

	/* 4 s3: swtich to USB function. (system register, force ip into usb mode. */
	USBPHY_CLR8(0x6b, 0x04);
	USBPHY_CLR8(0x6e, 0x01);
	USBPHY_CLR8(0x21, 0x03);
	/* USBPHY_CLR8(0x21, 0x01); */

	/* 4 s4: RG_USB20_BC11_SW_EN 1'b0 */
	USBPHY_CLR8(0x1a, 0x80);

	/* 5 s5: RG_USB20_DP_100K_EN 1'b0, RG_USB20_DM_100K_EN 1'b0 */
	USBPHY_SET8(0x22, 0x04);
	USBPHY_CLR8(0x22, 0x03);

	/*OTG enable*/
	USBPHY_SET8(0x1a, 0x10);
	/* 6 s6: release force suspendm. */
	USBPHY_CLR8(0x6a, 0x04);

	/* 6-1. PASS RX sensitivity HQA requirement */
	USBPHY_SET8(0x18, 0x06);

	/* 7 s7: wait for 800 usec. */
	udelay(800);

	/* 8 s8: enter device mode */
	USBPHY_CLR8(0x6c, 0x10);
	USBPHY_SET8(0x6c, 0x2F);
	USBPHY_SET8(0x6d, 0x3F);

	DBG(0, "usb power on success\n");
}

#ifdef CONFIG_MTK_UART_USB_SWITCH
static bool skipDisableUartMode;
#endif

static void usb_phy_savecurrent_internal(void)
{

	/* 4 1. swtich to USB function. (system register, force ip into usb mode. */

	#ifdef CONFIG_MTK_UART_USB_SWITCH
	if (!usb_phy_check_in_uart_mode()) {
		/* 4 s1: enable USB MAC clock. */
		usb_enable_clock(true);

		/* 4 s2: wait 50 usec for PHY3.3v/1.8v stable. */
		udelay(50);

		USBPHY_CLR8(0x6b, 0x04);
		USBPHY_CLR8(0x6e, 0x01);

		USBPHY_CLR8(0x21, 0x03);

		/* 4 2. force suspendm. */
		/* USBPHY_CLR8(0x6a, 0x04); */
		USBPHY_SET8(0x6a, 0x04);
		USBPHY_SET8(0x68, 0x08);
		usb_enable_clock(false);
	} else {
		if (skipDisableUartMode)
			skipDisableUartMode = false;
		else
			return;
	}
	#else
	USBPHY_CLR8(0x6b, 0x04);
	USBPHY_CLR8(0x6e, 0x01);
	USBPHY_CLR8(0x21, 0x03);

	/* 4 2. release force suspendm. */
	/* USBPHY_CLR8(0x6a, 0x04); */
	USBPHY_SET8(0x6a, 0x04);
	USBPHY_SET8(0x68, 0x08);
	#endif

	/* 4 3. RG_DPPULLDOWN./RG_DMPULLDOWN. */
	USBPHY_SET8(0x68, 0xc0);
	/* 4 4. RG_XCVRSEL[1:0] =2'b01. */
	USBPHY_CLR8(0x68, 0x30);
	USBPHY_SET8(0x68, 0x10);
	/* 4 5. RG_TERMSEL = 1'b1 */
	USBPHY_SET8(0x68, 0x04);
	/* 4 6. RG_DATAIN[3:0]=4'b0000 */
	USBPHY_CLR8(0x69, 0x3c);
	/* 4 7.force_dp_pulldown, force_dm_pulldown, force_xcversel,force_termsel. */
	USBPHY_SET8(0x6a, 0xba);

	/* 4 8.RG_USB20_BC11_SW_EN 1'b0 */
	USBPHY_CLR8(0x1a, 0x80);
	/* 4 9.RG_USB20_OTG_VBUSSCMP_EN 1'b0 */
	USBPHY_CLR8(0x1a, 0x10);
	/* 4 10. delay 800us. */
	udelay(800);
	/* 4 11. rg_usb20_pll_stable = 1 */
	USBPHY_CLR8(0x68, 0x08);
	/* USBPHY_SET8(0x63, 0x02); */

/* ALPS00427972, implement the analog register formula */
	/* printk("%s: USBPHY_READ8(0x05) = 0x%x\n", __func__, USBPHY_READ8(0x05)); */
	/* printk("%s: USBPHY_READ8(0x07) = 0x%x\n", __func__, USBPHY_READ8(0x07)); */
/* ALPS00427972, implement the analog register formula */

	udelay(1);
	/* 4 12.  force suspendm = 1. */
	/* USBPHY_SET8(0x6a, 0x04); */
	/* 4 13.  wait 1us */
	udelay(1);

	/* force enter device mode */
	/* USBPHY_CLR8(0x6c, 0x10); */
	/* USBPHY_SET8(0x6c, 0x2E); */
	/* USBPHY_SET8(0x6d, 0x3E); */
}

void usb_phy_savecurrent(void)
{
	usb_phy_savecurrent_internal();
	/* 4 14. turn off internal 48Mhz PLL. */
	usb_enable_clock(false);
	DBG(0, "usb save current success\n");
}

void usb_phy_recover(void)
{

	/* 4 1. turn on USB reference clock. */
	usb_enable_clock(true);
	/* 4 2. wait 50 usec. */
	udelay(50);

	#ifdef CONFIG_MTK_UART_USB_SWITCH
	if (!usb_phy_check_in_uart_mode()) {
		/* clean PUPD_BIST_EN */
		/* PUPD_BIST_EN = 1'b0 */
		/* PMIC will use it to detect charger type */
	USBPHY_CLR8(0x1d, 0x10);

	/* 4 3. force_uart_en = 1'b0 */
	USBPHY_CLR8(0x6b, 0x04);
	/* 4 4. RG_UART_EN = 1'b0 */
	USBPHY_CLR8(0x6e, 0x1);
	/* 4 5. release force suspendm. */
	USBPHY_CLR8(0x6a, 0x04);
	USBPHY_CLR8(0x22, 0x02);

	skipDisableUartMode = false;
	} else {
		/* if (!skipDisableUartMode) */
			return;
	}
	#else
	/* clean PUPD_BIST_EN */
	/* PUPD_BIST_EN = 1'b0 */
	/* PMIC will use it to detect charger type */
	USBPHY_CLR8(0x1d, 0x10);

	/* 4 3. force_uart_en = 1'b0 */
	USBPHY_CLR8(0x6b, 0x04);
	/* 4 4. RG_UART_EN = 1'b0 */
	USBPHY_CLR8(0x6e, 0x1);
	/* 4 5. force_uart_en = 1'b0 */
	USBPHY_CLR8(0x6a, 0x04);

	USBPHY_CLR8(0x21, 0x03);
	#endif

	/* 4 6. RG_DPPULLDOWN = 1'b0 */
	USBPHY_CLR8(0x68, 0x40);
	/* 4 7. RG_DMPULLDOWN = 1'b0 */
	USBPHY_CLR8(0x68, 0x80);
	/* 4 8. RG_XCVRSEL = 2'b00 */
	USBPHY_CLR8(0x68, 0x30);
	/* 4 9. RG_TERMSEL = 1'b0 */
	USBPHY_CLR8(0x68, 0x04);
	/* 4 10. RG_DATAIN[3:0] = 4'b0000 */
	USBPHY_CLR8(0x69, 0x3c);

	/* 4 11. force_dp_pulldown = 1b'0 */
	USBPHY_CLR8(0x6a, 0x10);
	/* 4 12. force_dm_pulldown = 1b'0 */
	USBPHY_CLR8(0x6a, 0x20);
	/* 4 13. force_xcversel = 1b'0 */
	USBPHY_CLR8(0x6a, 0x08);
	/* 4 14. force_termsel = 1b'0 */
	USBPHY_CLR8(0x6a, 0x02);
	/* 4 15. force_datain = 1b'0 */
	USBPHY_CLR8(0x6a, 0x80);

	/* 4 16. RG_USB20_BC11_SW_EN 1'b0 */
	USBPHY_CLR8(0x1a, 0x80);
	/* 4 17. RG_USB20_OTG_VBUSSCMP_EN 1'b1 */
	USBPHY_SET8(0x1a, 0x10);
	/* 18. PASS RX sensitivity HQA requirement */
	USBPHY_CLR8(0x18, 0x08);
	USBPHY_SET8(0x18, 0x06);

	/* 4 18. wait 800 usec. */
	udelay(800);

	/* force enter device mode, k2 need? */
	USBPHY_CLR8(0x6c, 0x10);
	USBPHY_SET8(0x6c, 0x2F);
	USBPHY_SET8(0x6d, 0x3F);

	#if defined(MTK_HDMI_SUPPORT)
	USBPHY_SET8(0x05, 0x05);
	USBPHY_SET8(0x05, 0x50);
	#endif
	hs_slew_rate_cal();

	DBG(0, "usb recovery success\n");
}

/* BC1.2 */
void Charger_Detect_Init(void)
{
	/* turn on USB reference clock. */
	usb_enable_clock(true);
	/* wait 50 usec. */
	udelay(50);
	/* RG_USB20_BC11_SW_EN = 1'b1 */
	USBPHY_SET8(0x1a, 0x80);
	DBG(0, "Charger_Detect_Init\n");
}

void Charger_Detect_Release(void)
{
	/* RG_USB20_BC11_SW_EN = 1'b0 */
	USBPHY_CLR8(0x1a, 0x80);
	udelay(1);
	/* 4 14. turn off internal 48Mhz PLL. */
	usb_enable_clock(false);
	DBG(0, "Charger_Detect_Release\n");
}

void usb_phy_context_save(void)
{
#ifdef CONFIG_MTK_UART_USB_SWITCH
	in_uart_mode = usb_phy_check_in_uart_mode();
#endif
}

void usb_phy_context_restore(void)
{
#ifdef CONFIG_MTK_UART_USB_SWITCH
	if (in_uart_mode)
		usb_phy_switch_to_uart();
#endif
	usb_phy_savecurrent_internal();
}

#endif
