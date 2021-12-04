#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/time.h>
#include "kd_flashlight.h"
#include <asm/io.h>
#include <asm/uaccess.h>
#include "kd_camera_typedef.h"
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/leds.h>
#include "upmu_common.h"
//#include "upmu_hw.h"
#include "leds_sw.h"


/******************************************************************************
 * Debug configuration
******************************************************************************/
/* availible parameter */
/* ANDROID_LOG_ASSERT */
/* ANDROID_LOG_ERROR */
/* ANDROID_LOG_WARNING */
/* ANDROID_LOG_INFO */
/* ANDROID_LOG_DEBUG */
/* ANDROID_LOG_VERBOSE */

#define TAG_NAME "[leds_strobe_sub.c]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    pr_debug(TAG_NAME "%s: " fmt, __func__ , ##arg)

/*#define DEBUG_LEDS_STROBE*/
#ifdef DEBUG_LEDS_STROBE
#define PK_DBG PK_DBG_FUNC
#else
#define PK_DBG(a, ...)
#endif


/******************************************************************************
 * local variables
******************************************************************************/

static DEFINE_SPINLOCK(g_strobeSMPLock);	/* cotta-- SMP proection */


static u32 strobe_Res;
static u32 strobe_Timeus;
static BOOL g_strobe_On;

static int g_timeOutTimeMs;

static DEFINE_MUTEX(g_strobeSem);



static struct work_struct workTimeOut;
/*****************************************************************************
Functions
*****************************************************************************/
static void work_timeOutFunc(struct work_struct *data);

static int FL_Enable(void)
{
    //pmic_set_register_value(PMIC_RG_DRV_32K_CK_PDN,0x0); // Disable power down  
    //pmic_set_register_value(PMIC_RG_ISINK0_CK_PDN,0);
    //pmic_set_register_value(PMIC_RG_ISINK0_CK_SEL,0);
    //pmic_set_register_value(PMIC_ISINK_CH0_MODE,ISINK_REGISTER_MODE);
	//pmic_set_register_value(PMIC_RG_ISINK0_DOUBLE_EN, 0x0);	/* Enable double current */
    //pmic_set_register_value(PMIC_ISINK_CH0_STEP,ISINK_5);//24mA
    
    //pmic_set_register_value(PMIC_RG_ISINK1_CK_PDN,0);
    //pmic_set_register_value(PMIC_RG_ISINK1_CK_SEL,0);
    //pmic_set_register_value(PMIC_ISINK_CH1_MODE,ISINK_REGISTER_MODE);
	//pmic_set_register_value(PMIC_RG_ISINK1_DOUBLE_EN, 0x0);	/* Enable double current */
    //pmic_set_register_value(PMIC_ISINK_CH1_STEP,ISINK_5);//24mA
    
    //pmic_set_register_value(PMIC_RG_ISINK3_CK_PDN,0);
    //pmic_set_register_value(PMIC_RG_ISINK3_CK_SEL,0);
    //pmic_set_register_value(PMIC_ISINK_CH3_MODE,ISINK_REGISTER_MODE);
	//pmic_set_register_value(PMIC_RG_ISINK3_DOUBLE_EN, 0x0);	/* Enable double current */
    //pmic_set_register_value(PMIC_ISINK_CH3_STEP,ISINK_5);//24mA

    //pmic_set_register_value(PMIC_ISINK_CH0_EN,NLED_ON);
    //pmic_set_register_value(PMIC_ISINK_CH1_EN,NLED_ON);
    pmic_set_register_value(PMIC_ISINK_CH3_EN,NLED_ON);
    printk("Isink FL_enable");        
            
    return 0;
}

static int FL_Disable(void)
{
    //pmic_set_register_value(PMIC_ISINK_CH0_EN,NLED_OFF);
    //pmic_set_register_value(PMIC_ISINK_CH1_EN,NLED_OFF);
    pmic_set_register_value(PMIC_ISINK_CH3_EN,NLED_OFF);

    printk("Isink FL_disable");

    return 0;
}

static int FL_dim_duty(kal_uint32 duty)
{
    return 0;
}

// isink0, isink1, isink3
static int FL_Init(void)
{
    pmic_set_register_value(PMIC_RG_DRV_32K_CK_PDN,0x0); // Disable power down  
    //pmic_set_register_value(PMIC_RG_ISINK0_CK_PDN,0);
    //pmic_set_register_value(PMIC_RG_ISINK0_CK_SEL,0);
    //pmic_set_register_value(PMIC_ISINK_CH0_MODE,ISINK_REGISTER_MODE);
	//pmic_set_register_value(PMIC_RG_ISINK0_DOUBLE_EN, 0x0);	/* Enable double current */
    //pmic_set_register_value(PMIC_ISINK_CH0_STEP,ISINK_5);//24mA
    
    //pmic_set_register_value(PMIC_RG_ISINK1_CK_PDN,0);
    //pmic_set_register_value(PMIC_RG_ISINK1_CK_SEL,0);
    //pmic_set_register_value(PMIC_ISINK_CH1_MODE,ISINK_REGISTER_MODE);
	//pmic_set_register_value(PMIC_RG_ISINK1_DOUBLE_EN, 0x0);	/* Enable double current */
    //pmic_set_register_value(PMIC_ISINK_CH1_STEP,ISINK_5);//24mA
    
    pmic_set_register_value(PMIC_RG_ISINK3_CK_PDN,0);
    pmic_set_register_value(PMIC_RG_ISINK3_CK_SEL,0);
    pmic_set_register_value(PMIC_ISINK_CH3_MODE,ISINK_REGISTER_MODE);
	pmic_set_register_value(PMIC_RG_ISINK3_DOUBLE_EN, 0x0);	/* Enable double current */
    pmic_set_register_value(PMIC_ISINK_CH3_STEP,ISINK_5);//24mA

    //pmic_set_register_value(PMIC_ISINK_CH0_EN,NLED_OFF);
    //pmic_set_register_value(PMIC_ISINK_CH1_EN,NLED_OFF);
    pmic_set_register_value(PMIC_ISINK_CH3_EN,NLED_OFF);
    
    printk("Isink FL_init");
            
    return 0;
}


static int FL_Uninit(void)
{
    FL_Disable();
    return 0;
}

/*****************************************************************************
User interface
*****************************************************************************/

static void work_timeOutFunc(struct work_struct *data)
{
	FL_Disable();
	PK_DBG("ledTimeOut_callback\n");
}

static enum hrtimer_restart ledTimeOutCallback(struct hrtimer *timer)
{
	schedule_work(&workTimeOut);
	return HRTIMER_NORESTART;
}

static struct hrtimer g_timeOutTimer;
static void timerInit(void)
{
	INIT_WORK(&workTimeOut, work_timeOutFunc);
	g_timeOutTimeMs = 1000;
	hrtimer_init(&g_timeOutTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	g_timeOutTimer.function = ledTimeOutCallback;
}



static int sub_strobe_ioctl(unsigned int cmd, unsigned long arg)
{
	int i4RetValue = 0;
	int ior_shift;
	int iow_shift;
	int iowr_shift;

	ior_shift = cmd - (_IOR(FLASHLIGHT_MAGIC, 0, int));
	iow_shift = cmd - (_IOW(FLASHLIGHT_MAGIC, 0, int));
	iowr_shift = cmd - (_IOWR(FLASHLIGHT_MAGIC, 0, int));
/*	PK_DBG
	    ("LM3642 constant_flashlight_ioctl() line=%d ior_shift=%d, iow_shift=%d iowr_shift=%d arg=%d\n",
	     __LINE__, ior_shift, iow_shift, iowr_shift, (int)arg);
*/
	switch (cmd) {

	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		PK_DBG("FLASH_IOC_SET_TIME_OUT_TIME_MS: %d\n", (int)arg);
		g_timeOutTimeMs = arg;
		break;


	case FLASH_IOC_SET_DUTY:
		PK_DBG("FLASHLIGHT_DUTY: %d\n", (int)arg);
		FL_dim_duty(arg);
		break;


	case FLASH_IOC_SET_STEP:
		PK_DBG("FLASH_IOC_SET_STEP: %d\n", (int)arg);

		break;

	case FLASH_IOC_SET_ONOFF:
		PK_DBG("FLASHLIGHT_ONOFF: %d\n", (int)arg);
		if (arg == 1) {

			int s;
			int ms;

			if (g_timeOutTimeMs > 1000) {
				s = g_timeOutTimeMs / 1000;
				ms = g_timeOutTimeMs - s * 1000;
			} else {
				s = 0;
				ms = g_timeOutTimeMs;
			}

			if (g_timeOutTimeMs != 0) {
				ktime_t ktime;

				ktime = ktime_set(s, ms * 1000000);
				hrtimer_start(&g_timeOutTimer, ktime, HRTIMER_MODE_REL);
			}
			FL_Enable();
		} else {
			FL_Disable();
			hrtimer_cancel(&g_timeOutTimer);
		}
		break;
	default:
		PK_DBG(" No such command\n");
		i4RetValue = -EPERM;
		break;
	}
	return i4RetValue;
}




static int sub_strobe_open(void *pArg)
{
	int i4RetValue = 0;

	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);

	if (0 == strobe_Res) {
		FL_Init();
		timerInit();
	}
	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);
	spin_lock_irq(&g_strobeSMPLock);


	if (strobe_Res) {
		PK_DBG(" busy!\n");
		i4RetValue = -EBUSY;
	} else {
		strobe_Res += 1;
	}


	spin_unlock_irq(&g_strobeSMPLock);
	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);

	return i4RetValue;

}


static int sub_strobe_release(void *pArg)
{
	PK_DBG(" constant_flashlight_release\n");

	if (strobe_Res) {
		spin_lock_irq(&g_strobeSMPLock);

		strobe_Res = 0;
		strobe_Timeus = 0;

		/* LED On Status */
		g_strobe_On = FALSE;

		spin_unlock_irq(&g_strobeSMPLock);

		FL_Uninit();
	}

	PK_DBG(" Done\n");

	return 0;

}


FLASHLIGHT_FUNCTION_STRUCT subStrobeFunc = {
	sub_strobe_open,
	sub_strobe_release,
	sub_strobe_ioctl
};


MUINT32 subStrobeInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc)
{
	if (pfFunc != NULL)
		*pfFunc = &subStrobeFunc;
	return 0;
}
