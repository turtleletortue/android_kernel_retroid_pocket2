#include "accdet.h"
#ifdef CONFIG_ACCDET_EINT
#include <linux/gpio.h>
#endif
#include <upmu_common.h>
#include <linux/timer.h>
#include <linux/of.h>
#include <linux/of_irq.h>

#define DEBUG_THREAD 1

/*----------------------------------------------------------------------
static variable defination
----------------------------------------------------------------------*/

#define REGISTER_VALUE(x)   (x - 1)
static int button_press_debounce = 0x400;
int cur_key = 0;
struct head_dts_data accdet_dts_data;
s8 accdet_auxadc_offset;
int accdet_irq;
unsigned int gpiopin, headsetdebounce;
unsigned int accdet_eint_type;
struct headset_mode_settings *cust_headset_settings;
#define ACCDET_DEBUG(format, args...) pr_warn(format, ##args)
#define ACCDET_INFO(format, args...) pr_warn(format, ##args)
#define ACCDET_ERROR(format, args...) pr_err(format, ##args)
static struct switch_dev accdet_data;
static struct input_dev *kpd_accdet_dev;
static struct cdev *accdet_cdev;
static struct class *accdet_class;
static struct device *accdet_nor_device;
static dev_t accdet_devno;
static int pre_status;
static int pre_state_swctrl;
static int accdet_status = PLUG_OUT;
static int cable_type;
#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
/*add for new feature PIN recognition*/
static int cable_pin_recognition;
static int show_icon_delay;
#endif
static int eint_accdet_sync_flag;
static int g_accdet_first = 1;
static bool IRQ_CLR_FLAG;
static char call_status;
static int button_status;
struct wake_lock accdet_suspend_lock;
struct wake_lock accdet_irq_lock;
struct wake_lock accdet_key_lock;
struct wake_lock accdet_timer_lock;
static struct work_struct accdet_work;
static struct workqueue_struct *accdet_workqueue;
static DEFINE_MUTEX(accdet_eint_irq_sync_mutex);
static inline void clear_accdet_interrupt(void);
static void send_key_event(int keycode, int flag);
#if defined CONFIG_ACCDET_EINT
static struct work_struct accdet_eint_work;
static struct workqueue_struct *accdet_eint_workqueue;
static struct work_struct accdet_speaker_work;
static struct workqueue_struct *accdet_speaker_workqueue;
static inline void accdet_init(void);
#define MICBIAS_DISABLE_TIMER   (6 * HZ)	/*6 seconds*/
struct timer_list micbias_timer;
static void disable_micbias(unsigned long a);
/* Used to let accdet know if the pin has been fully plugged-in */
#define EINT_PIN_PLUG_IN        (1)
#define EINT_PIN_PLUG_OUT       (0)
int cur_eint_state = EINT_PIN_PLUG_OUT;
int cur_eint_state2 = EINT_PIN_PLUG_OUT;
struct pinctrl *accdet_pinctrl1;
struct pinctrl_state *pins_eint_int;
static struct work_struct accdet_disable_work;
static struct workqueue_struct *accdet_disable_workqueue;
#else
/*static int g_accdet_working_in_suspend =0;*/
#endif/*end CONFIG_ACCDET_EINT*/
#ifdef DEBUG_THREAD
#endif


static u32 pmic_pwrap_read(u32 addr);
static void pmic_pwrap_write(u32 addr, unsigned int wdata);
char *accdet_status_string[5] = {
	"Plug_out",
	"Headset_plug_in",
	/*"Double_check",*/
	"Hook_switch",
	/*"Tvout_plug_in",*/
	"Stand_by"
};
char *accdet_report_string[4] = {
	"No_device",
	"Headset_mic",
	"Headset_no_mic",
	/*"HEADSET_illegal",*/
	/* "Double_check"*/
};
/****************************************************************/
/***        export function                                                                        **/
/****************************************************************/
struct pinctrl_state *pinctrlio63_output0 = 0, *pinctrlio63_output1 = 0;
struct pinctrl *pinctrlio63;

void accdet_speaker_enable(int state)
{
	if(pinctrlio63_output1 == 0 || pinctrlio63_output0 == 0)
		return;
	if (state == EINT_PIN_PLUG_OUT){
		pinctrl_select_state(pinctrlio63, pinctrlio63_output1);
	}
	else{
		pinctrl_select_state(pinctrlio63, pinctrlio63_output0);
	}
}

extern int get_hdmi_state(void);

void speaker_power(unsigned int on)
{
	if(on == 0)
	{
		accdet_speaker_enable(EINT_PIN_PLUG_IN);
		//printk("[accdet] speaker_power off.\n");
	}else{
		if(cur_eint_state2 == EINT_PIN_PLUG_OUT && get_hdmi_state() != 1)
		{
			accdet_speaker_enable(EINT_PIN_PLUG_OUT);
			//printk("[accdet] speaker_power on.\n");
		}
	}
}
EXPORT_SYMBOL(speaker_power);

int accdet_get_cable_type(void)
{
	return cable_type;
}

void accdet_auxadc_switch(int enable)
{
	if (enable) {
		pmic_pwrap_write(ACCDET_RSV, ACCDET_1V9_MODE_ON);
		/*ACCDET_DEBUG("ACCDET enable switch\n");*/
	} else {
		pmic_pwrap_write(ACCDET_RSV, ACCDET_1V9_MODE_OFF);
		/*ACCDET_DEBUG("ACCDET disable switch\n");*/
	}
}

/****************************************************************/
/*******static function defination                             **/
/****************************************************************/
static u64 accdet_get_current_time(void)
{
	return sched_clock();
}

static bool accdet_timeout_ns(u64 start_time_ns, u64 timeout_time_ns)
{
	u64 cur_time = 0;
	u64 elapse_time = 0;

	/*get current tick*/
	cur_time = accdet_get_current_time();	/*ns*/
	if (cur_time < start_time_ns) {
		ACCDET_DEBUG("@@@@Timer overflow! start%lld cur timer%lld\n", start_time_ns, cur_time);
		start_time_ns = cur_time;
		timeout_time_ns = 400 * 1000;	/*400us*/
		ACCDET_DEBUG("@@@@reset timer! start%lld setting%lld\n", start_time_ns, timeout_time_ns);
	}
	elapse_time = cur_time - start_time_ns;

	/*check if timeout*/
	if (timeout_time_ns <= elapse_time) {
		/*timeout*/
		ACCDET_DEBUG("@@@@ACCDET IRQ clear Timeout\n");
		return false;
	}
	return true;
}

/*pmic wrap read and write func*/
static u32 pmic_pwrap_read(u32 addr)
{
	u32 val = 0;

	pwrap_read(addr, &val);
	return val;
}

static void pmic_pwrap_write(unsigned int addr, unsigned int wdata)
{
	pwrap_write(addr, wdata);
}


#ifdef CONFIG_ACCDET_PIN_SWAP

static void accdet_FSA8049_enable(void)
{
	mt_set_gpio_mode(GPIO_FSA8049_PIN, GPIO_FSA8049_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_FSA8049_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_FSA8049_PIN, GPIO_OUT_ONE);
}

static void accdet_FSA8049_disable(void)
{
	mt_set_gpio_mode(GPIO_FSA8049_PIN, GPIO_FSA8049_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_FSA8049_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_FSA8049_PIN, GPIO_OUT_ZERO);
}

#endif
static inline void headset_plug_out(void)
{
	accdet_status = PLUG_OUT;
	cable_type = NO_DEVICE;
	/*update the cable_type*/
	if (cur_key != 0) {
		send_key_event(cur_key, 0);
		ACCDET_DEBUG(" [accdet] headset_plug_out send key = %d release\n", cur_key);
		cur_key = 0;
	}
	switch_set_state((struct switch_dev *)&accdet_data, cable_type);
	ACCDET_DEBUG(" [accdet] set state in cable_type = NO_DEVICE\n");

}

/*Accdet only need this func*/
static inline void enable_accdet(u32 state_swctrl)
{
	/*enable ACCDET unit*/
	ACCDET_DEBUG("accdet: enable_accdet\n");
	/*enable clock*/
	pmic_pwrap_write(TOP_CKPDN_CLR, RG_ACCDET_CLK_CLR);
	pmic_pwrap_write(ACCDET_STATE_SWCTRL, pmic_pwrap_read(ACCDET_STATE_SWCTRL) | state_swctrl);
	pmic_pwrap_write(ACCDET_CTRL, pmic_pwrap_read(ACCDET_CTRL) | ACCDET_ENABLE);

}

static inline void disable_accdet(void)
{
	int irq_temp = 0;

	/*sync with accdet_irq_handler set clear accdet irq bit to avoid  set clear accdet irq bit after disable accdet
	disable accdet irq*/
	pmic_pwrap_write(INT_CON_ACCDET_CLR, RG_ACCDET_IRQ_CLR);
	clear_accdet_interrupt();
	udelay(200);
	mutex_lock(&accdet_eint_irq_sync_mutex);
	while (pmic_pwrap_read(ACCDET_IRQ_STS) & IRQ_STATUS_BIT) {
		ACCDET_DEBUG("[Accdet]check_cable_type: Clear interrupt on-going....\n");
		msleep(20);
	}
	irq_temp = pmic_pwrap_read(ACCDET_IRQ_STS);
	irq_temp = irq_temp & (~IRQ_CLR_BIT);
	pmic_pwrap_write(ACCDET_IRQ_STS, irq_temp);
	mutex_unlock(&accdet_eint_irq_sync_mutex);
	/*disable ACCDET unit*/
	ACCDET_DEBUG("accdet: disable_accdet\n");
	pre_state_swctrl = pmic_pwrap_read(ACCDET_STATE_SWCTRL);
	pmic_pwrap_write(ACCDET_STATE_SWCTRL, 0);
	pmic_pwrap_write(ACCDET_CTRL, ACCDET_DISABLE);
	/*disable clock*/
	pmic_pwrap_write(TOP_CKPDN_SET, RG_ACCDET_CLK_SET);
}

static void disable_micbias(unsigned long a)
{
	int ret = 0;

	ret = queue_work(accdet_disable_workqueue, &accdet_disable_work);
	if (!ret)
		ACCDET_DEBUG("[Accdet]disable_micbias:accdet_work return:%d!\n", ret);
}

static void disable_micbias_callback(struct work_struct *work)
{

	if (cable_type == HEADSET_NO_MIC) {
#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
		show_icon_delay = 0;
		cable_pin_recognition = 0;
		ACCDET_DEBUG("[Accdet] cable_pin_recognition = %d\n", cable_pin_recognition);
		pmic_pwrap_write(ACCDET_PWM_WIDTH, cust_headset_settings->pwm_width);
		pmic_pwrap_write(ACCDET_PWM_THRESH, cust_headset_settings->pwm_thresh);
#endif
		/*setting pwm idle;*/
		pmic_pwrap_write(ACCDET_STATE_SWCTRL, pmic_pwrap_read(ACCDET_STATE_SWCTRL) & ~ACCDET_SWCTRL_IDLE_EN);
#ifdef CONFIG_ACCDET_PIN_SWAP
		/*accdet_FSA8049_disable(); disable GPIOxxx for PIN swap */
		/*ACCDET_DEBUG("[Accdet] FSA8049 disable!\n");*/
#endif
		disable_accdet();
		ACCDET_DEBUG("[Accdet] more than 5s MICBIAS : Disabled\n");
	}
#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
	else if (cable_type == HEADSET_MIC) {
		pmic_pwrap_write(ACCDET_PWM_WIDTH, cust_headset_settings->pwm_width);
		pmic_pwrap_write(ACCDET_PWM_THRESH, cust_headset_settings->pwm_thresh);
		ACCDET_DEBUG("[Accdet]pin recog after 5s recover micbias polling!\n");
	}
#endif
}

int accdet_irq_handler(void);

static void accdet_eint_work_callback(struct work_struct *work)
{
	/*KE under fastly plug in and plug out*/
	if (cur_eint_state == EINT_PIN_PLUG_IN) {
		ACCDET_DEBUG("[Accdet]ACC EINT func :plug-in, cur_eint_state = %d\n", cur_eint_state);
		mutex_lock(&accdet_eint_irq_sync_mutex);
		eint_accdet_sync_flag = 1;
		mutex_unlock(&accdet_eint_irq_sync_mutex);
		wake_lock_timeout(&accdet_timer_lock, 7 * HZ);
		accdet_init();	/* do set pwm_idle on in accdet_init*/

		/*set PWM IDLE  on*/
		pmic_pwrap_write(ACCDET_STATE_SWCTRL, (pmic_pwrap_read(ACCDET_STATE_SWCTRL) | ACCDET_SWCTRL_IDLE_EN));
		/*enable ACCDET unit*/
		enable_accdet(ACCDET_SWCTRL_EN);
	} else {  /*EINT_PIN_PLUG_OUT*/
        /*Disable ACCDET*/
		ACCDET_DEBUG("[Accdet]EINT func :plug-out, cur_eint_state = %d\n", cur_eint_state);
		mutex_lock(&accdet_eint_irq_sync_mutex);
		eint_accdet_sync_flag = 0;
		mutex_unlock(&accdet_eint_irq_sync_mutex);
		del_timer_sync(&micbias_timer);
		accdet_auxadc_switch(0);
		disable_accdet();
		headset_plug_out();
	}
	enable_irq(accdet_irq);
	ACCDET_INFO("[Accdet]enable_irq  !!!!!!\n");
	accdet_irq_handler();
}

static void accdet_speaker_work_callback(struct work_struct *work)
{
	if(cur_eint_state2 == EINT_PIN_PLUG_OUT)
		speaker_power(1);
	else
		speaker_power(0);
	enable_irq(accdet_irq);
}

static irqreturn_t accdet_eint_func(int irq, void *data)
{
	int ret = 0;

	ACCDET_INFO("[Accdet]Enter accdet_eint_func, accdet_eint_type=%d !!!!!!\n", accdet_eint_type);
	if (cur_eint_state2 == EINT_PIN_PLUG_IN) {
		/*
		   To trigger EINT when the headset was plugged in
		   We set the polarity back as we initialed.
		 */
		if (accdet_eint_type == IRQ_TYPE_LEVEL_HIGH)
			irq_set_irq_type(accdet_irq, IRQ_TYPE_LEVEL_HIGH);
		else
			irq_set_irq_type(accdet_irq, IRQ_TYPE_LEVEL_LOW);
		gpio_set_debounce(gpiopin, headsetdebounce);
		/* update the eint status */
		cur_eint_state2 = EINT_PIN_PLUG_OUT;
	} else {
		/*
		   To trigger EINT when the headset was plugged out
		   We set the opposite polarity to what we initialed.
		 */
		if (accdet_eint_type == IRQ_TYPE_LEVEL_HIGH)
			irq_set_irq_type(accdet_irq, IRQ_TYPE_LEVEL_LOW);
		else
			irq_set_irq_type(accdet_irq, IRQ_TYPE_LEVEL_HIGH);

		gpio_set_debounce(gpiopin, accdet_dts_data.accdet_plugout_debounce * 1000);
		/* update the eint status */
		cur_eint_state2 = EINT_PIN_PLUG_IN;

		mod_timer(&micbias_timer, jiffies + MICBIAS_DISABLE_TIMER);
	}
	disable_irq_nosync(accdet_irq);
	ACCDET_DEBUG("[Accdet]accdet_eint_func after cur_eint_state2=%d\n", cur_eint_state2);

	ret = queue_work(accdet_speaker_workqueue, &accdet_speaker_work);
	return IRQ_HANDLED;
}
static DEFINE_MUTEX(led_set_gpio_mutex);

void led_gpio_output(int pin, int level)
{

}
static inline int accdet_setup_eint(struct platform_device *accdet_device)
{
	int ret;
	u32 ints[2] = { 0, 0 };
	u32 ints1[2] = { 0, 0 };
	struct device_node *node = NULL;

	/*configure to GPIO function, external interrupt */
	ACCDET_INFO("[Accdet]accdet_setup_eint\n");
	accdet_pinctrl1 = devm_pinctrl_get(&accdet_device->dev);
	if (IS_ERR(accdet_pinctrl1)) {
		ret = PTR_ERR(accdet_pinctrl1);
		dev_err(&accdet_device->dev, "fwq Cannot find accdet accdet_pinctrl1!\n");
		return ret;
	}

	pins_eint_int = pinctrl_lookup_state(accdet_pinctrl1, "state_eint_as_int");
	if (IS_ERR(pins_eint_int)) {
		ret = PTR_ERR(pins_eint_int);
		dev_err(&accdet_device->dev, "fwq Cannot find accdet pinctrl state_eint_int!\n");
		return ret;
	}
	pinctrl_select_state(accdet_pinctrl1, pins_eint_int);

	pinctrlio63 = devm_pinctrl_get(&accdet_device->dev);
	if (IS_ERR(pinctrlio63)) {
		ret = PTR_ERR(pinctrlio63);
		printk("fwq Cannot find mygpio pinctrlio63!\n");
		return ret;
	}
	
	pinctrlio63_output0 = pinctrl_lookup_state(pinctrlio63, "speaker_off");
	if (IS_ERR(pinctrlio63_output0)) {
		ret = PTR_ERR(pinctrlio63_output0);
		dev_err(&accdet_device->dev, "fwq Cannot find pinctrlio63_output0!\n");
		return ret;
	}
	pinctrlio63_output1 = pinctrl_lookup_state(pinctrlio63, "speaker_on");
	if (IS_ERR(pinctrlio63_output1)) {
		ret = PTR_ERR(pinctrlio63_output1);
		dev_err(&accdet_device->dev, "fwq Cannot find pinctrlio63_output1!\n");
		return ret;
	}
	speaker_power(1);

	node = of_find_matching_node(node, accdet_of_match);
	if (node) {
		of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
		of_property_read_u32_array(node, "interrupts", ints1, ARRAY_SIZE(ints1));
		gpiopin = ints[0];
		headsetdebounce = ints[1];
		accdet_eint_type = ints1[1];
		gpio_set_debounce(gpiopin, headsetdebounce);
		accdet_irq = irq_of_parse_and_map(node, 0);
		ret = request_irq(accdet_irq, accdet_eint_func, IRQF_TRIGGER_NONE, "accdet-eint", NULL);
		if (ret != 0) {
			ACCDET_ERROR("[Accdet]EINT IRQ LINE NOT AVAILABLE\n");
		} else {
			ACCDET_ERROR("[Accdet]accdet set EINT finished, accdet_irq=%d, headsetdebounce=%d\n",
				     accdet_irq, headsetdebounce);
		}
		cur_eint_state = EINT_PIN_PLUG_IN;
		queue_work(accdet_eint_workqueue, &accdet_eint_work);
		ACCDET_INFO("[Accdet]queue_work accdet_eint_workqueue\n");
	} else {
		ACCDET_ERROR("[Accdet]%s can't find compatible node\n", __func__);
	}
	return 0;
}

#define KEY_SAMPLE_PERIOD        (60)	/*ms*/
#define MULTIKEY_ADC_CHANNEL	 (8)

static DEFINE_MUTEX(accdet_multikey_mutex);
#define NO_KEY			 (0x0)
#define UP_KEY			 (0x01)
#define MD_KEY			(0x02)
#define DW_KEY			 (0x04)
#define AS_KEY			 (0x08)

#ifndef CONFIG_FOUR_KEY_HEADSET
static int key_check(int b)
{
	/*ACCDET_DEBUG("adc_data: %d v\n",b);*/

	/* 0.24V ~ */
	/*ACCDET_DEBUG("[accdet] come in key_check!!\n");*/
	if ((b < accdet_dts_data.three_key.down_key) && (b >= accdet_dts_data.three_key.up_key))
		return DW_KEY;
	else if ((b < accdet_dts_data.three_key.up_key) && (b >= accdet_dts_data.three_key.mid_key))
		return UP_KEY;
	else if ((b < accdet_dts_data.three_key.mid_key) && (b >= 0))
		return MD_KEY;
	ACCDET_DEBUG("[accdet] leave key_check!!\n");
	return NO_KEY;
}
#else
static int key_check(int b)
{
	/* 0.24V ~ */
	/*ACCDET_DEBUG("[accdet] come in key_check!!\n");*/
	if ((b < accdet_dts_data.four_key.down_key_four) && (b >= accdet_dts_data.four_key.up_key_four))
		return DW_KEY;
	else if ((b < accdet_dts_data.four_key.up_key_four) && (b >= accdet_dts_data.four_key.voice_key_four))
		return UP_KEY;
	else if ((b < accdet_dts_data.four_key.voice_key_four) && (b >= accdet_dts_data.four_key.mid_key_four))
		return AS_KEY;
	else if ((b < accdet_dts_data.four_key.mid_key_four) && (b >= 0))
		return MD_KEY;
	ACCDET_DEBUG("[accdet] leave key_check!!\n");
	return NO_KEY;
}

#endif
static void send_key_event(int keycode, int flag)
{
	switch (keycode) {
	case DW_KEY:
		input_report_key(kpd_accdet_dev, KEY_VOLUMEDOWN, flag);
		input_sync(kpd_accdet_dev);
		ACCDET_DEBUG("[accdet]KEY_VOLUMEDOWN %d\n", flag);
		break;
	case UP_KEY:
		input_report_key(kpd_accdet_dev, KEY_VOLUMEUP, flag);
		input_sync(kpd_accdet_dev);
		ACCDET_DEBUG("[accdet]KEY_VOLUMEUP %d\n", flag);
		break;
	case MD_KEY:
		input_report_key(kpd_accdet_dev, KEY_PLAYPAUSE, flag);
		input_sync(kpd_accdet_dev);
		ACCDET_DEBUG("[accdet]KEY_PLAYPAUSE %d\n", flag);
		break;
	case AS_KEY:
		input_report_key(kpd_accdet_dev, KEY_VOICECOMMAND, flag);
		input_sync(kpd_accdet_dev);
		ACCDET_DEBUG("[accdet]KEY_VOICECOMMAND %d\n", flag);
		break;
	}
}

static void multi_key_detection(int current_status)
{
	int m_key = 0;
	int cali_voltage = 0;

	if (0 == current_status) {
		cali_voltage = PMIC_IMM_GetOneChannelValue(MULTIKEY_ADC_CHANNEL, 1, 1);
		/*ACCDET_DEBUG("[Accdet]adc cali_voltage1 = %d mv\n", cali_voltage);*/
		m_key = cur_key = key_check(cali_voltage);
	}
	mdelay(30);
	if (((pmic_pwrap_read(ACCDET_IRQ_STS) & IRQ_STATUS_BIT) != IRQ_STATUS_BIT) || eint_accdet_sync_flag) {
		send_key_event(cur_key, !current_status);
	} else {
		ACCDET_DEBUG("[Accdet]plug out side effect key press, do not report key = %d\n", cur_key);
		cur_key = NO_KEY;
	}
	if (current_status)
		cur_key = NO_KEY;
}

static void accdet_workqueue_func(void)
{
	int ret;

	ret = queue_work(accdet_workqueue, &accdet_work);
	if (!ret)
		ACCDET_DEBUG("[Accdet]accdet_work return:%d!\n", ret);
}

int accdet_irq_handler(void)
{
	u64 cur_time = 0;
	ACCDET_DEBUG("[Accdet]accdet_irq_handler\n");

	cur_time = accdet_get_current_time();

	if ((pmic_pwrap_read(ACCDET_IRQ_STS) & IRQ_STATUS_BIT))
		clear_accdet_interrupt();
	if (accdet_status == MIC_BIAS) {
		accdet_auxadc_switch(1);
		pmic_pwrap_write(ACCDET_PWM_WIDTH, REGISTER_VALUE(cust_headset_settings->pwm_width));
		pmic_pwrap_write(ACCDET_PWM_THRESH, REGISTER_VALUE(cust_headset_settings->pwm_width));
	}
	accdet_workqueue_func();
	while (((pmic_pwrap_read(ACCDET_IRQ_STS) & IRQ_STATUS_BIT)
		&& (accdet_timeout_ns(cur_time, ACCDET_TIME_OUT))))
		;
	return 1;
}

/*clear ACCDET IRQ in accdet register*/
static inline void clear_accdet_interrupt(void)
{
	/*it is safe by using polling to adjust when to clear IRQ_CLR_BIT*/
	pmic_pwrap_write(ACCDET_IRQ_STS, ((pmic_pwrap_read(ACCDET_IRQ_STS)) & 0x8000) | (IRQ_CLR_BIT));
	ACCDET_DEBUG("[Accdet]clear_accdet_interrupt: ACCDET_IRQ_STS = 0x%x\n", pmic_pwrap_read(ACCDET_IRQ_STS));
}

static inline void check_cable_type(void)
{
	int current_status = 0;
	int irq_temp = 0;	/*for clear IRQ_bit*/
	int wait_clear_irq_times = 0;

	current_status = 0;//((pmic_pwrap_read(ACCDET_STATE_RG) & 0xc0) >> 6);	/*A=bit1; B=bit0*/
	ACCDET_DEBUG("[Accdet]accdet interrupt happen:[%s]current AB = %d\n",
		     accdet_status_string[accdet_status], current_status);

	button_status = 0;
	pre_status = accdet_status;

	/*ACCDET_DEBUG("[Accdet]check_cable_type: ACCDET_IRQ_STS = 0x%x\n", pmic_pwrap_read(ACCDET_IRQ_STS));*/
	IRQ_CLR_FLAG = false;
	switch (accdet_status) {
	case PLUG_OUT:
		if (current_status == 0) {
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if (1 == eint_accdet_sync_flag) {
				cable_type = HEADSET_NO_MIC;
				accdet_status = HOOK_SWITCH;
			} else {
				ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			}
			mutex_unlock(&accdet_eint_irq_sync_mutex);
		} else if (current_status == 1) {
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if (1 == eint_accdet_sync_flag) {
				accdet_status = MIC_BIAS;
				cable_type = HEADSET_MIC;
				/*AB=11 debounce=30ms*/
				pmic_pwrap_write(ACCDET_DEBOUNCE3, cust_headset_settings->debounce3 * 30);
			} else {
				ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			}
			mutex_unlock(&accdet_eint_irq_sync_mutex);
			pmic_pwrap_write(ACCDET_DEBOUNCE0, button_press_debounce);
			/*recover polling set AB 00-01*/
		} else if (current_status == 3) {
			ACCDET_DEBUG("[Accdet]PLUG_OUT state not change!\n");
#ifdef CONFIG_ACCDET_EINT
			ACCDET_DEBUG("[Accdet] do not send plug out event in plug out\n");
#else
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if (1 == eint_accdet_sync_flag) {
				accdet_status = PLUG_OUT;
				cable_type = NO_DEVICE;
			} else {
				ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			}
			mutex_unlock(&accdet_eint_irq_sync_mutex);
#endif
		} else {
			ACCDET_DEBUG("[Accdet]PLUG_OUT can't change to this state!\n");
		}
		break;
	case MIC_BIAS:
		/*solution: resume hook switch debounce time*/
		pmic_pwrap_write(ACCDET_DEBOUNCE0, cust_headset_settings->debounce0);

		if (current_status == 0) {
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if (1 == eint_accdet_sync_flag) {
				while ((pmic_pwrap_read(ACCDET_IRQ_STS) & IRQ_STATUS_BIT)
					&& (wait_clear_irq_times < 3)) {
					ACCDET_DEBUG("[Accdet]check_cable_type: MIC BIAS clear IRQ on-going1....\n");
					wait_clear_irq_times++;
					msleep(20);
				}
				irq_temp = pmic_pwrap_read(ACCDET_IRQ_STS);
				irq_temp = irq_temp & (~IRQ_CLR_BIT);
				pmic_pwrap_write(ACCDET_IRQ_STS, irq_temp);
				IRQ_CLR_FLAG = true;
				accdet_status = HOOK_SWITCH;
			} else {
				ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			}
			mutex_unlock(&accdet_eint_irq_sync_mutex);
			button_status = 1;
			if (button_status) {
				mutex_lock(&accdet_eint_irq_sync_mutex);
				if (1 == eint_accdet_sync_flag)
					multi_key_detection(current_status);
				else
					ACCDET_DEBUG("[Accdet] multi_key_detection: Headset has plugged out\n");
				mutex_unlock(&accdet_eint_irq_sync_mutex);
				accdet_auxadc_switch(0);
				/*recover  pwm frequency and duty*/
				pmic_pwrap_write(ACCDET_PWM_WIDTH, REGISTER_VALUE(cust_headset_settings->pwm_width));
				pmic_pwrap_write(ACCDET_PWM_THRESH, REGISTER_VALUE(cust_headset_settings->pwm_thresh));
			}
		} else if (current_status == 1) {
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if (1 == eint_accdet_sync_flag) {
				accdet_status = MIC_BIAS;
				cable_type = HEADSET_MIC;
				ACCDET_DEBUG("[Accdet]MIC_BIAS state not change!\n");
			} else {
				ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			}
			mutex_unlock(&accdet_eint_irq_sync_mutex);
		} else if (current_status == 3) {
			ACCDET_DEBUG("[Accdet]do not send plug ou in micbiast\n");
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if (1 == eint_accdet_sync_flag)
				accdet_status = PLUG_OUT;
			else
				ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			mutex_unlock(&accdet_eint_irq_sync_mutex);
		} else {
			ACCDET_DEBUG("[Accdet]MIC_BIAS can't change to this state!\n");
		}
		break;

	case HOOK_SWITCH:
		if (current_status == 0) {
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if (1 == eint_accdet_sync_flag) {
				/*for avoid 01->00 framework of Headset will report press key info for Audio*/
				/*cable_type = HEADSET_NO_MIC;*/
				/*accdet_status = HOOK_SWITCH;*/
				ACCDET_DEBUG("[Accdet]HOOK_SWITCH state not change!\n");
			} else {
				ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			}
			mutex_unlock(&accdet_eint_irq_sync_mutex);
		} else if (current_status == 1) {
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if (1 == eint_accdet_sync_flag) {
				multi_key_detection(current_status);
				accdet_status = MIC_BIAS;
				cable_type = HEADSET_MIC;
			} else {
				ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			}
			mutex_unlock(&accdet_eint_irq_sync_mutex);
			/*accdet_auxadc_switch(0);*/
#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
			cable_pin_recognition = 0;
			ACCDET_DEBUG("[Accdet] cable_pin_recognition = %d\n", cable_pin_recognition);
			pmic_pwrap_write(ACCDET_PWM_WIDTH, REGISTER_VALUE(cust_headset_settings->pwm_width));
			pmic_pwrap_write(ACCDET_PWM_THRESH, REGISTER_VALUE(cust_headset_settings->pwm_thresh));
#endif
			/*solution: reduce hook switch debounce time to 0x400*/
			pmic_pwrap_write(ACCDET_DEBOUNCE0, button_press_debounce);
		} else if (current_status == 3) {

#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
			cable_pin_recognition = 0;
			ACCDET_DEBUG("[Accdet] cable_pin_recognition = %d\n", cable_pin_recognition);
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if (1 == eint_accdet_sync_flag)
				accdet_status = PLUG_OUT;
			else
				ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			mutex_unlock(&accdet_eint_irq_sync_mutex);
#endif
#if defined CONFIG_ACCDET_EINT
			ACCDET_DEBUG("[Accdet] do not send plug out event in hook switch\n");
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if (1 == eint_accdet_sync_flag)
				accdet_status = PLUG_OUT;
			else
				ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			mutex_unlock(&accdet_eint_irq_sync_mutex);
#endif
		} else {
			ACCDET_DEBUG("[Accdet]HOOK_SWITCH can't change to this state!\n");
		}
		break;
	case STAND_BY:
		if (current_status == 3) {
#if defined CONFIG_ACCDET_EINT
			ACCDET_DEBUG("[Accdet]accdet do not send plug out event in stand by!\n");
#endif
		} else {
			ACCDET_DEBUG("[Accdet]STAND_BY can't change to this state!\n");
		}
		break;

	default:
		ACCDET_DEBUG("[Accdet]check_cable_type: accdet current status error!\n");
		break;

	}

	if (!IRQ_CLR_FLAG) {
		mutex_lock(&accdet_eint_irq_sync_mutex);
		if (1 == eint_accdet_sync_flag) {
			while ((pmic_pwrap_read(ACCDET_IRQ_STS) & IRQ_STATUS_BIT) && (wait_clear_irq_times < 3)) {
				ACCDET_DEBUG("[Accdet]check_cable_type: Clear interrupt on-going2....\n");
				wait_clear_irq_times++;
				msleep(20);
			}
		}
		irq_temp = pmic_pwrap_read(ACCDET_IRQ_STS);
		irq_temp = irq_temp & (~IRQ_CLR_BIT);
		pmic_pwrap_write(ACCDET_IRQ_STS, irq_temp);
		mutex_unlock(&accdet_eint_irq_sync_mutex);
		IRQ_CLR_FLAG = true;
		ACCDET_DEBUG("[Accdet]check_cable_type:Clear interrupt:Done[0x%x]!\n", pmic_pwrap_read(ACCDET_IRQ_STS));

	} else {
		IRQ_CLR_FLAG = false;
	}

	ACCDET_DEBUG("[Accdet]cable type:[%s], status switch:[%s]->[%s]\n",
		     accdet_report_string[cable_type], accdet_status_string[pre_status],
		     accdet_status_string[accdet_status]);
}

static void accdet_work_callback(struct work_struct *work)
{

	wake_lock(&accdet_irq_lock);
	check_cable_type();

	mutex_lock(&accdet_eint_irq_sync_mutex);
	if (1 == eint_accdet_sync_flag)
		switch_set_state((struct switch_dev *)&accdet_data, cable_type);
	else
		ACCDET_DEBUG("[Accdet] Headset has plugged out don't set accdet state\n");
	mutex_unlock(&accdet_eint_irq_sync_mutex);
	ACCDET_DEBUG(" [accdet] set state in cable_type  status\n");

	wake_unlock(&accdet_irq_lock);
}

void accdet_get_dts_data(void)
{
	struct device_node *node = NULL;
	int debounce[7];
	#ifdef CONFIG_FOUR_KEY_HEADSET
	int four_key[5];
	#else
	int three_key[4];
	#endif

	ACCDET_INFO("[ACCDET]Start accdet_get_dts_data");
	node = of_find_matching_node(node, accdet_of_match);
	if (node) {
		of_property_read_u32_array(node, "headset-mode-setting", debounce, ARRAY_SIZE(debounce));
		of_property_read_u32(node, "accdet-mic-vol", &accdet_dts_data.mic_mode_vol);
		of_property_read_u32(node, "accdet-plugout-debounce", &accdet_dts_data.accdet_plugout_debounce);
		of_property_read_u32(node, "accdet-mic-mode", &accdet_dts_data.accdet_mic_mode);
		#ifdef CONFIG_FOUR_KEY_HEADSET
		of_property_read_u32_array(node, "headset-four-key-threshold", four_key, ARRAY_SIZE(four_key));
		memcpy(&accdet_dts_data.four_key, four_key+1, sizeof(four_key));
		ACCDET_INFO("[Accdet]mid-Key = %d, voice = %d, up_key = %d, down_key = %d\n",
		     accdet_dts_data.four_key.mid_key_four, accdet_dts_data.four_key.voice_key_four,
		     accdet_dts_data.four_key.up_key_four, accdet_dts_data.four_key.down_key_four);
		#else
		of_property_read_u32_array(node, "headset-three-key-threshold", three_key, ARRAY_SIZE(three_key));
		memcpy(&accdet_dts_data.three_key, three_key+1, sizeof(three_key));
		ACCDET_INFO("[Accdet]mid-Key = %d, up_key = %d, down_key = %d\n",
		     accdet_dts_data.three_key.mid_key, accdet_dts_data.three_key.up_key,
		     accdet_dts_data.three_key.down_key);
		#endif

		memcpy(&accdet_dts_data.headset_debounce, debounce, sizeof(debounce));
		cust_headset_settings = &accdet_dts_data.headset_debounce;
		ACCDET_INFO("[Accdet]pwm_width = %x, pwm_thresh = %x\n deb0 = %x, deb1 = %x, mic_mode = %d\n",
		     cust_headset_settings->pwm_width, cust_headset_settings->pwm_thresh,
		     cust_headset_settings->debounce0, cust_headset_settings->debounce1,
		     accdet_dts_data.accdet_mic_mode);
	} else {
		ACCDET_ERROR("[Accdet]%s can't find compatible dts node\n", __func__);
	}
}

static inline void accdet_init(void)
{
	ACCDET_DEBUG("[Accdet]accdet hardware init\n");
	/*clock*/
	pmic_pwrap_write(TOP_CKPDN_CLR, RG_ACCDET_CLK_CLR);
	/*ACCDET_DEBUG("[Accdet]accdet TOP_CKPDN=0x%x!\n", pmic_pwrap_read(TOP_CKPDN)); */
	/*reset the accdet unit*/
	/*ACCDET_DEBUG("ACCDET reset : reset start!\n\r");*/
	pmic_pwrap_write(TOP_RST_ACCDET_SET, ACCDET_RESET_SET);
	/*ACCDET_DEBUG("ACCDET reset function test: reset finished!!\n\r");*/
	pmic_pwrap_write(TOP_RST_ACCDET_CLR, ACCDET_RESET_CLR);
	/*init  pwm frequency and duty*/
	pmic_pwrap_write(ACCDET_PWM_WIDTH, REGISTER_VALUE(cust_headset_settings->pwm_width));
	pmic_pwrap_write(ACCDET_PWM_THRESH, REGISTER_VALUE(cust_headset_settings->pwm_thresh));
	pmic_pwrap_write(ACCDET_STATE_SWCTRL, 0x07);

	/*rise and fall delay of PWM*/
	pmic_pwrap_write(ACCDET_EN_DELAY_NUM,
			 (cust_headset_settings->fall_delay << 15 | cust_headset_settings->rise_delay));
	/* init the debounce time*/
#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
	pmic_pwrap_write(ACCDET_DEBOUNCE0, cust_headset_settings->debounce0);
	pmic_pwrap_write(ACCDET_DEBOUNCE1, 0xFFFF);	/*2.0s*/
	pmic_pwrap_write(ACCDET_DEBOUNCE3, cust_headset_settings->debounce3);
#else
	pmic_pwrap_write(ACCDET_DEBOUNCE0, cust_headset_settings->debounce0);
	pmic_pwrap_write(ACCDET_DEBOUNCE1, cust_headset_settings->debounce1);
	pmic_pwrap_write(ACCDET_DEBOUNCE3, cust_headset_settings->debounce3);
#endif
	/*enable INT */
	pmic_pwrap_write(ACCDET_IRQ_STS, pmic_pwrap_read(ACCDET_IRQ_STS) & (~IRQ_CLR_BIT));
	pmic_pwrap_write(INT_CON_ACCDET_SET, RG_ACCDET_IRQ_SET);
    /**************************************************************************************************/
#if defined CONFIG_ACCDET_EINT
	/* disable ACCDET unit*/
	pre_state_swctrl = pmic_pwrap_read(ACCDET_STATE_SWCTRL);
	pmic_pwrap_write(ACCDET_CTRL, ACCDET_DISABLE);
	pmic_pwrap_write(ACCDET_STATE_SWCTRL, 0x0);
	pmic_pwrap_write(TOP_CKPDN_SET, RG_ACCDET_CLK_SET);
#else
	/* enable ACCDET unit*/
	pmic_pwrap_write(ACCDET_CTRL, ACCDET_ENABLE);
#endif
}

/*-----------------------------------sysfs-----------------------------------------*/
#if DEBUG_THREAD
static int dump_register(void)
{
	int i = 0;

	for (i = ACCDET_RSV; i <= ACCDET_RSV_CON1; i += 2)
		ACCDET_DEBUG(" ACCDET_BASE + %x=%x\n", i, pmic_pwrap_read(ACCDET_BASE + i));

	ACCDET_DEBUG(" TOP_RST_ACCDET(0x%x) =%x\n", TOP_RST_ACCDET, pmic_pwrap_read(TOP_RST_ACCDET));
	ACCDET_DEBUG(" INT_CON_ACCDET(0x%x) =%x\n", INT_CON_ACCDET, pmic_pwrap_read(INT_CON_ACCDET));
	ACCDET_DEBUG(" TOP_CKPDN(0x%x) =%x\n", TOP_CKPDN, pmic_pwrap_read(TOP_CKPDN));
#ifdef CONFIG_ACCDET_PIN_SWAP
	/*ACCDET_DEBUG(" 0x00004000 =%x\n",pmic_pwrap_read(0x00004000));*/
	/*VRF28 power for PIN swap feature*/
#endif
	return 0;
}

static ssize_t accdet_store_call_state(struct device_driver *ddri, const char *buf, size_t count)
{
	int ret;

	ret = sscanf(buf, "%s", &call_status);
	if (ret != 1) {
		ACCDET_DEBUG("accdet: Invalid values\n");
		return -EINVAL;
	}

	switch (call_status) {
	case CALL_IDLE:
		ACCDET_DEBUG("[Accdet]accdet call: Idle state!\n");
		break;

	case CALL_RINGING:

		ACCDET_DEBUG("[Accdet]accdet call: ringing state!\n");
		break;

	case CALL_ACTIVE:
		ACCDET_DEBUG("[Accdet]accdet call: active or hold state!\n");
		ACCDET_DEBUG("[Accdet]accdet_ioctl : Button_Status=%d (state:%d)\n", button_status, accdet_data.state);
		/*return button_status;*/
		break;

	default:
		ACCDET_DEBUG("[Accdet]accdet call : Invalid values\n");
		break;
	}
	return count;
}

static ssize_t show_pin_recognition_state(struct device_driver *ddri, char *buf)
{
#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
	ACCDET_DEBUG("ACCDET show_pin_recognition_state = %d\n", cable_pin_recognition);
	return sprintf(buf, "%u\n", cable_pin_recognition);
#else
	return sprintf(buf, "%u\n", 0);
#endif
}

static DRIVER_ATTR(accdet_pin_recognition, 0664, show_pin_recognition_state, NULL);
static DRIVER_ATTR(accdet_call_state, 0664, NULL, accdet_store_call_state);

static int g_start_debug_thread;
static struct task_struct *thread;
static int g_dump_register;
static int dbug_thread(void *unused)
{
	while (g_start_debug_thread) {
		if (g_dump_register) {
			dump_register();
			/*dump_pmic_register();*/
		}

		msleep(500);

	}
	return 0;
}

static ssize_t store_accdet_start_debug_thread(struct device_driver *ddri, const char *buf, size_t count)
{

	char start_flag;
	int error;
	int ret;

	ret = sscanf(buf, "%s", &start_flag);
	if (ret != 1) {
		ACCDET_DEBUG("accdet: Invalid values\n");
		return -EINVAL;
	}

	ACCDET_DEBUG("[Accdet] start flag =%d\n", start_flag);

	g_start_debug_thread = start_flag;

	if (1 == start_flag) {
		thread = kthread_run(dbug_thread, 0, "ACCDET");
		if (IS_ERR(thread)) {
			error = PTR_ERR(thread);
			ACCDET_DEBUG(" failed to create kernel thread: %d\n", error);
		}
	}

	return count;
}

static ssize_t store_accdet_set_headset_mode(struct device_driver *ddri, const char *buf, size_t count)
{

	char value;
	int ret;

	ret = sscanf(buf, "%s", &value);
	if (ret != 1) {
		ACCDET_DEBUG("accdet: Invalid values\n");
		return -EINVAL;
	}

	ACCDET_DEBUG("[Accdet]store_accdet_set_headset_mode value =%d\n", value);

	return count;
}

static ssize_t store_accdet_dump_register(struct device_driver *ddri, const char *buf, size_t count)
{
	char value;
	int ret;

	ret = sscanf(buf, "%s", &value);
	if (ret != 1) {
		ACCDET_DEBUG("accdet: Invalid values\n");
		return -EINVAL;
	}

	g_dump_register = value;

	ACCDET_DEBUG("[Accdet]store_accdet_dump_register value =%d\n", value);

	return count;
}

/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(dump_register, S_IWUSR | S_IRUGO, NULL, store_accdet_dump_register);

static DRIVER_ATTR(set_headset_mode, S_IWUSR | S_IRUGO, NULL, store_accdet_set_headset_mode);

static DRIVER_ATTR(start_debug, S_IWUSR | S_IRUGO, NULL, store_accdet_start_debug_thread);

/*----------------------------------------------------------------------------*/
static struct driver_attribute *accdet_attr_list[] = {
	&driver_attr_start_debug,
	&driver_attr_set_headset_mode,
	&driver_attr_dump_register,
	&driver_attr_accdet_call_state,
	/*#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION*/
	&driver_attr_accdet_pin_recognition,
	/*#endif*/
};

static int accdet_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(accdet_attr_list) / sizeof(accdet_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;
	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, accdet_attr_list[idx]);
		if (err) {
			ACCDET_DEBUG("driver_create_file (%s) = %d\n", accdet_attr_list[idx]->attr.name, err);
			break;
		}
	}
	return err;
}

#endif

static ssize_t accdet_set_amp(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	unsigned int value = 0;
	ret = kstrtouint(buf, 0, &value);
	//accdet_hdmi_enable(value);
	printk("[accdet] accdet_set_amp:%d\n",value);
	speaker_power(value);
	return size;
}

static DEVICE_ATTR(amp, 0664, NULL, accdet_set_amp);

static struct device_attribute *accdet_attributes[] = {
	&dev_attr_amp,
	NULL
};

int mt_accdet_probe(struct platform_device *dev)
{
	int ret = 0;
	int err = 0;
	struct device_attribute **attrs = accdet_attributes;
	struct device_attribute *attr;
	
#if DEBUG_THREAD
	struct platform_driver accdet_driver_hal = accdet_driver_func();
#endif

	ACCDET_INFO("[Accdet]accdet_probe begin!\n");

	/*--------------------------------------------------------------------
	// below register accdet as switch class
	//------------------------------------------------------------------*/
	accdet_data.name = "h2w";
	accdet_data.index = 0;
	accdet_data.state = NO_DEVICE;
	
	ret = switch_dev_register(&accdet_data);
	if (ret) {
		ACCDET_ERROR("[Accdet]switch_dev_register returned:%d!\n", ret);
		return 1;
	}
	/*----------------------------------------------------------------------
	// Create normal device for auido use
	//--------------------------------------------------------------------*/
	ret = alloc_chrdev_region(&accdet_devno, 0, 1, ACCDET_DEVNAME);
	if (ret)
		ACCDET_ERROR("[Accdet]alloc_chrdev_region: Get Major number error!\n");

	accdet_cdev = cdev_alloc();
	accdet_cdev->owner = THIS_MODULE;
	accdet_cdev->ops = accdet_get_fops();
	ret = cdev_add(accdet_cdev, accdet_devno, 1);
	if (ret)
		ACCDET_ERROR("[Accdet]accdet error: cdev_add\n");

	accdet_class = class_create(THIS_MODULE, ACCDET_DEVNAME);

	/* if we want auto creat device node, we must call this*/
	accdet_nor_device = device_create(accdet_class, NULL, accdet_devno, NULL, ACCDET_DEVNAME);

	/*--------------------------------------------------------------------
	// Create input device
	//------------------------------------------------------------------*/
	kpd_accdet_dev = input_allocate_device();
	if (!kpd_accdet_dev) {
		ACCDET_ERROR("[Accdet]kpd_accdet_dev : fail!\n");
		return -ENOMEM;
	}
	/*INIT the timer to disable micbias.*/
	init_timer(&micbias_timer);
	micbias_timer.expires = jiffies + MICBIAS_DISABLE_TIMER;
	micbias_timer.function = &disable_micbias;
	micbias_timer.data = ((unsigned long)0);

	/*define multi-key keycode*/
	__set_bit(EV_KEY, kpd_accdet_dev->evbit);
	__set_bit(KEY_PLAYPAUSE, kpd_accdet_dev->keybit);
	__set_bit(KEY_VOLUMEDOWN, kpd_accdet_dev->keybit);
	__set_bit(KEY_VOLUMEUP, kpd_accdet_dev->keybit);
	__set_bit(KEY_VOICECOMMAND, kpd_accdet_dev->keybit);

	kpd_accdet_dev->id.bustype = BUS_HOST;
	kpd_accdet_dev->name = "ACCDET";
	if (input_register_device(kpd_accdet_dev))
		ACCDET_ERROR("[Accdet]kpd_accdet_dev register : fail!\n");
	/*------------------------------------------------------------------
	// Create workqueue
	//------------------------------------------------------------------ */
	accdet_workqueue = create_singlethread_workqueue("accdet");
	INIT_WORK(&accdet_work, accdet_work_callback);

	/*------------------------------------------------------------------
	// wake lock
	//------------------------------------------------------------------*/
	wake_lock_init(&accdet_suspend_lock, WAKE_LOCK_SUSPEND, "accdet wakelock");
	wake_lock_init(&accdet_irq_lock, WAKE_LOCK_SUSPEND, "accdet irq wakelock");
	wake_lock_init(&accdet_key_lock, WAKE_LOCK_SUSPEND, "accdet key wakelock");
	wake_lock_init(&accdet_timer_lock, WAKE_LOCK_SUSPEND, "accdet timer wakelock");
#if DEBUG_THREAD
	ret = accdet_create_attr(&accdet_driver_hal.driver);
	if (ret != 0)
		ACCDET_ERROR("create attribute err = %d\n", ret);
#endif
	ACCDET_INFO("[Accdet]accdet_probe : ACCDET_INIT\n");
	if (g_accdet_first == 1) {
		eint_accdet_sync_flag = 1;
		/*Accdet Hardware Init*/
		pmic_pwrap_write(ACCDET_RSV, ACCDET_1V9_MODE_OFF);
		accdet_get_dts_data();
		accdet_init();

		/*schedule a work for the first detection*/
		//queue_work(accdet_workqueue, &accdet_work);
#ifdef CONFIG_ACCDET_EINT
		accdet_disable_workqueue = create_singlethread_workqueue("accdet_disable");
		INIT_WORK(&accdet_disable_work, disable_micbias_callback);
		accdet_eint_workqueue = create_singlethread_workqueue("accdet_eint");
		INIT_WORK(&accdet_eint_work, accdet_eint_work_callback);
		accdet_speaker_workqueue = create_singlethread_workqueue("accdet_eint");
		INIT_WORK(&accdet_speaker_work, accdet_speaker_work_callback);
		accdet_setup_eint(dev);
#endif
		g_accdet_first = 0;
	}
	ACCDET_INFO("[Accdet]accdet_probe done!\n");

	while ((attr = *attrs++)) {
		err = device_create_file(accdet_nor_device, attr);
		if (err) {
			return err;
		}
	}
	return 0;
}

void mt_accdet_remove(void)
{
	ACCDET_DEBUG("[Accdet]accdet_remove begin!\n");

	/*cancel_delayed_work(&accdet_work);*/
#if defined CONFIG_ACCDET_EINT
	destroy_workqueue(accdet_eint_workqueue);
#endif
	destroy_workqueue(accdet_workqueue);
	switch_dev_unregister(&accdet_data);
	device_del(accdet_nor_device);
	class_destroy(accdet_class);
	cdev_del(accdet_cdev);
	unregister_chrdev_region(accdet_devno, 1);
	input_unregister_device(kpd_accdet_dev);
	ACCDET_DEBUG("[Accdet]accdet_remove Done!\n");
}

void mt_accdet_suspend(void)	/*only one suspend mode*/
{
#if defined CONFIG_ACCDET_EINT
	ACCDET_DEBUG("[Accdet] in suspend1: ACCDET_IRQ_STS = 0x%x\n", pmic_pwrap_read(ACCDET_IRQ_STS));
#else
	ACCDET_DEBUG("[Accdet]accdet_suspend: ACCDET_CTRL=[0x%x], STATE=[0x%x]->[0x%x]\n",
	       pmic_pwrap_read(ACCDET_CTRL), pre_state_swctrl, pmic_pwrap_read(ACCDET_STATE_SWCTRL));
#endif
	printk("[accdet] mt_accdet_suspend\n");
}

void mt_accdet_resume(void)	/*wake up*/
{
#if defined CONFIG_ACCDET_EINT
	ACCDET_DEBUG("[Accdet] in resume1: ACCDET_IRQ_STS = 0x%x\n", pmic_pwrap_read(ACCDET_IRQ_STS));
#else
	ACCDET_DEBUG("[Accdet]accdet_resume: ACCDET_CTRL=[0x%x], STATE_SWCTRL=[0x%x]\n",
	       pmic_pwrap_read(ACCDET_CTRL), pmic_pwrap_read(ACCDET_STATE_SWCTRL));

#endif
	printk("[accdet] mt_accdet_resume\n");

}

/**********************************************************************
//add for IPO-H need update headset state when resume

***********************************************************************/
#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
struct timer_list accdet_disable_ipoh_timer;
static void mt_accdet_pm_disable(unsigned long a)
{
	if (cable_type == NO_DEVICE && eint_accdet_sync_flag == 0) {
		/*disable accdet*/
		pre_state_swctrl = pmic_pwrap_read(ACCDET_STATE_SWCTRL);
		pmic_pwrap_write(ACCDET_STATE_SWCTRL, 0);
#ifdef CONFIG_ACCDET_EINT
		pmic_pwrap_write(ACCDET_CTRL, ACCDET_DISABLE);
		/*disable clock*/
		pmic_pwrap_write(TOP_CKPDN_SET, RG_ACCDET_CLK_SET);
#endif
		ACCDET_DEBUG("[Accdet]daccdet_pm_disable: disable!\n");
	} else {
		ACCDET_DEBUG("[Accdet]daccdet_pm_disable: enable!\n");
	}
}
#endif
void mt_accdet_pm_restore_noirq(void)
{
	int current_status_restore = 0;

	ACCDET_DEBUG("[Accdet]accdet_pm_restore_noirq start!\n");
	/*enable ACCDET unit*/
	ACCDET_DEBUG("accdet: enable_accdet\n");
	/*enable clock*/
	pmic_pwrap_write(TOP_CKPDN_CLR, RG_ACCDET_CLK_CLR);
	enable_accdet(ACCDET_SWCTRL_EN);
	pmic_pwrap_write(ACCDET_STATE_SWCTRL, (pmic_pwrap_read(ACCDET_STATE_SWCTRL) | ACCDET_SWCTRL_IDLE_EN));

	eint_accdet_sync_flag = 1;
	current_status_restore = ((pmic_pwrap_read(ACCDET_STATE_RG) & 0xc0) >> 6);	/*AB*/

	switch (current_status_restore) {
	case 0:		/*AB=0*/
		cable_type = HEADSET_NO_MIC;
		accdet_status = HOOK_SWITCH;
		break;
	case 1:		/*AB=1*/
		cable_type = HEADSET_MIC;
		accdet_status = MIC_BIAS;
		break;
	case 3:		/*AB=3*/
		cable_type = NO_DEVICE;
		accdet_status = PLUG_OUT;
		break;
	default:
		ACCDET_DEBUG("[Accdet]accdet_pm_restore_noirq: accdet current status error!\n");
		break;
	}
	switch_set_state((struct switch_dev *)&accdet_data, cable_type);
	if (cable_type == NO_DEVICE) {
#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
		init_timer(&accdet_disable_ipoh_timer);
		accdet_disable_ipoh_timer.expires = jiffies + 3 * HZ;
		accdet_disable_ipoh_timer.function = &mt_accdet_pm_disable;
		accdet_disable_ipoh_timer.data = ((unsigned long)0);
		add_timer(&accdet_disable_ipoh_timer);
		ACCDET_DEBUG("[Accdet]enable! pm timer\n");

#else
		/*disable accdet*/
		pre_state_swctrl = pmic_pwrap_read(ACCDET_STATE_SWCTRL);
#ifdef CONFIG_ACCDET_EINT
		pmic_pwrap_write(ACCDET_STATE_SWCTRL, 0);
		pmic_pwrap_write(ACCDET_CTRL, ACCDET_DISABLE);
		/*disable clock*/
		pmic_pwrap_write(TOP_CKPDN_SET, RG_ACCDET_CLK_SET);
#endif
#endif
	}
}

/*//////////////////////////////////IPO_H end/////////////////////////////////////////////*/
long mt_accdet_unlocked_ioctl(unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case ACCDET_INIT:
		break;
	case SET_CALL_STATE:
		call_status = (int)arg;
		ACCDET_DEBUG("[Accdet]accdet_ioctl : CALL_STATE=%d\n", call_status);
		break;
	case GET_BUTTON_STATUS:
		ACCDET_DEBUG("[Accdet]accdet_ioctl : Button_Status=%d (state:%d)\n", button_status, accdet_data.state);
		return button_status;
	default:
		ACCDET_DEBUG("[Accdet]accdet_ioctl : default\n");
		break;
	}
	return 0;
}
