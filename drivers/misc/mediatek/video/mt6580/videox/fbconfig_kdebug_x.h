#ifndef __FBCONFIG_KDEBUG_H
#define __FBCONFIG_KDEBUG_H

#include <linux/types.h>

/* *****************debug for fbconfig tool in kernel part************* */
#define MAX_INSTRUCTION 35
#define NUM_OF_DSI 1

typedef enum {
	RECORD_CMD = 0,
	RECORD_MS = 1,
	RECORD_PIN_SET = 2,
} RECORD_TYPE;

typedef enum {
	PM_DSI0 = 0,
	PM_DSI1 = 1,
	PM_DSI_DUAL = 2,
	PM_DSI_MAX = 0XFF,
} DSI_INDEX;

typedef struct CONFIG_RECORD {
	RECORD_TYPE type;	/* msleep;cmd;setpin;resetpin. */
	int ins_num;
	int ins_array[MAX_INSTRUCTION];
} CONFIG_RECORD;

typedef struct CONFIG_RECORD_LIST {
	CONFIG_RECORD record;
	struct list_head list;
} CONFIG_RECORD_LIST;

typedef enum MIPI_SETTING_TYPE {
	HS_PRPR = 0,
	HS_ZERO = 1,
	HS_TRAIL = 2,
	TA_GO = 3,
	TA_SURE = 4,
	TA_GET = 5,
	DA_HS_EXIT = 6,
	CLK_ZERO = 7,
	CLK_TRAIL = 8,
	CONT_DET = 9,
	CLK_HS_PRPR = 10,
	CLK_HS_POST = 11,
	CLK_HS_EXIT = 12,
	HPW = 13,
	HFP = 14,
	HBP = 15,
	VPW = 16,
	VFP = 17,
	VBP = 18,
	LPX = 19,
	SSC_EN = 0xFE,
	MAX = 0XFF,
} MIPI_SETTING_TYPE;

typedef struct MIPI_TIMING {
	MIPI_SETTING_TYPE type;
	unsigned int value;
} MIPI_TIMING;

typedef struct SETTING_VALUE {
	DSI_INDEX dsi_index;
	unsigned int value[NUM_OF_DSI];
} SETTING_VALUE;

typedef struct PM_LAYER_EN {
	int layer_en[4];	/* layer id :0 1 2 3 */
} PM_LAYER_EN;

typedef struct PM_LAYER_INFO {
	int index;
	int height;
	int width;
	int fmt;
	unsigned int layer_size;
} PM_LAYER_INFO;

typedef struct ESD_PARA {
	int addr;
	int type;
	int para_num;
	char *esd_ret_buffer;
} ESD_PARA;
#if 0
typedef struct LAYER_H_SIZE {
	int layer_size;
	int height;
	int fmt;
} LAYER_H_SIZE;
#endif
typedef struct MIPI_CLK_V2 {
	unsigned char div1;
	unsigned char div2;
	unsigned short fbk_div;
} MIPI_CLK_V2;

typedef struct LCM_TYPE_FB {
	int clock;
	int lcm_type;
} LCM_TYPE_FB;

typedef struct DSI_RET {
	int dsi[NUM_OF_DSI];	/* for there are totally 2 dsi. */
} DSI_RET;

typedef struct LCM_REG_READ {
	int check_addr;
	int check_para_num;
	int check_type;
	char *check_buffer;
} LCM_REG_READ;

typedef struct {
	void (*set_cmd_mode)(void);
	int (*set_mipi_clk)(unsigned int clk);
	void (*set_dsi_post)(void);
	void (*set_lane_num)(unsigned int lane_num);
	void (*set_mipi_timing)(MIPI_TIMING timing);
	void (*set_te_enable)(char enable);
	void (*set_continuous_clock)(int enable);
	int (*set_spread_frequency)(unsigned int clk);
	int (*set_get_misc)(const char *name, void *parameter);

} FBCONFIG_DISP_IF;

typedef struct _property {
	unsigned int dual_port:1;
	unsigned int reserved:31;
} misc_property;

void Panel_Master_DDIC_config(void);

void PanelMaster_Init(void);
void PanelMaster_Deinit(void);
int fb_config_execute_cmd(void);
int fbconfig_get_esd_check_exec(void);
int fbconfig_get_esd_check(DSI_INDEX dsi_id, uint32_t cmd, uint8_t *buffer, uint32_t num);

#endif /* __FBCONFIG_KDEBUG_H */
