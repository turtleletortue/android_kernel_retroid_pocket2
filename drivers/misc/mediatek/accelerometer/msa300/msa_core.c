/* Core file for MEMS 3-Axis Accelerometer's driver. 
 *
 * msa_core.c - Linux kernel modules for 3-Axis Accelerometer
 *
 * Copyright (C) 2007-2016 MEMS Sensing Technology Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "msa_core.h"
#include "msa_cust.h"

#define MSA_REG_ADDR(REG)                ((REG)&0xFF)

#define MSA_OFFSET_THRESHOLD             20
#define PEAK_LVL                            800                        
#define STICK_LSB                           2000
#define AIX_HISTORY_SIZE                    20

typedef struct reg_obj_s {
    
    short               addr;
    unsigned char       mask;
    unsigned char       value;
    
} reg_obj_t;

struct gsensor_data_fmt_s {

    unsigned char       msbw;
    unsigned char       lsbw;
    unsigned char       endian;                         /* 0: little endian; 1: big endian */
};

struct gsensor_data_obj_s {

#define MSA_DATA_LEN         6
    reg_obj_t                   data_sect[MSA_DATA_LEN];
    struct gsensor_data_fmt_s   data_fmt;
};

struct gsensor_obj_s {
    
    char                        asic[10];

    reg_obj_t                   chip_id;
    reg_obj_t                   mod_id;
    reg_obj_t                   soft_reset;
    reg_obj_t                   power;

#define MSA_INIT_SECT_LEN    11
#define MSA_OFF_SECT_LEN     MSA_OFFSET_LEN
#define MSA_ODR_SECT_LEN     3

    reg_obj_t                   init_sect[MSA_INIT_SECT_LEN];
    reg_obj_t                   offset_sect[MSA_OFF_SECT_LEN];
    reg_obj_t                   odr_sect[MSA_ODR_SECT_LEN];  
    
    struct gsensor_data_obj_s   data;

    int                         (*calibrate)(MSA_HANDLE handle, int z_dir);
    int                         (*auto_calibrate)(MSA_HANDLE handle, int xyz[3]);
    int                         (*int_ops)(MSA_HANDLE handle, msa_int_ops_t *ops);
    int                         (*get_reg_data)(MSA_HANDLE handle, char *buf);
};

struct gsensor_drv_s {
    
    struct general_op_s         *method;

    struct gsensor_obj_s        *obj;
};

typedef enum _asic_type{
    ASIC_NONE,  
    ASIC_2511,
    ASIC_2512B,
} asic_type;

typedef enum _mems_type{
    MEMS_NONE,  
    MEMS_T4,
    MEMS_T9,
    MEMS_TV03,
    MEMS_RTO3,
} mems_type;

typedef enum _package_type{
    PACKAGE_NONE,   
    PACKAGE_2X2_12PIN,
} package_type;

struct  chip_info_s{
    unsigned char    reg_value;
    package_type     package;
    asic_type        asic;
    mems_type        mems;
};

//MSA �Ĵ�����Ϣ��ʼ��

struct  chip_info_s gsensor_chip_info;

static struct chip_info_s         msa_chip_info_list[]=
{
    {0x00,PACKAGE_2X2_12PIN,ASIC_2512B,MEMS_TV03},
    {0x01,PACKAGE_2X2_12PIN,ASIC_2511,MEMS_T4},
    {0x02,PACKAGE_2X2_12PIN,ASIC_2511,MEMS_T9},
};

#define MSA_NSA_INIT_SECTION                         { NSA_REG_G_RANGE,              0x03,   0x00    },                                  \
                                                        { NSA_REG_POWERMODE_BW,         0xFF,   0x5e    },                                  \
                                                        { NSA_REG_ODR_AXIS_DISABLE,     0xFF,   0x07    },                                  \
                                                        { NSA_REG_INTERRUPT_SETTINGS2,  0xFF,   0x00    },                                  \
                                                        { NSA_REG_INTERRUPT_MAPPING2,   0xFF,   0x00    },                                  \
                                                        { NSA_REG_ENGINEERING_MODE,     0xFF,   0x83    },                                  \
                                                        { NSA_REG_ENGINEERING_MODE,     0xFF,   0x69    },                                  \
                                                        { NSA_REG_ENGINEERING_MODE,     0xFF,   0xBD    },                                  \
                                                        { NSA_REG_INT_PIN_CONFIG,       0x0F,   0x05    },                                  \
                                                        { -1,                           0x00,   0x00    },                                  \
                                                        { -1,                           0x00,   0x00    },                                  \
                                                        
                                                                                                 
#define MSA_NSA_OFFSET_SECTION                          { NSA_REG_COARSE_OFFSET_TRIM_X, 0xFF,   0x00    },                                  \
                                                        { NSA_REG_COARSE_OFFSET_TRIM_Y, 0xFF,   0x00    },                                  \
                                                        { NSA_REG_COARSE_OFFSET_TRIM_Z, 0xFF,   0x00    },                                  \
                                                        { NSA_REG_FINE_OFFSET_TRIM_X,   0xFF,   0x00    },                                  \
                                                        { NSA_REG_FINE_OFFSET_TRIM_Y,   0xFF,   0x00    },                                  \
                                                        { NSA_REG_FINE_OFFSET_TRIM_Z,   0xFF,   0x00    },                                  \
                                                        { NSA_REG_CUSTOM_OFFSET_X,      0xFF,   0x00    },                                  \
                                                        { NSA_REG_CUSTOM_OFFSET_Y,      0xFF,   0x00    },                                  \
                                                        { NSA_REG_CUSTOM_OFFSET_Z,      0xFF,   0x00    },                                  \
 
#define MSA_NSA_ODR_SECTION                             { NSA_REG_ODR_AXIS_DISABLE,     0x0F,   0x06    },                                  \
                                                        { NSA_REG_ODR_AXIS_DISABLE,     0x0F,   0x07    },                                  \
                                                        { NSA_REG_ODR_AXIS_DISABLE,     0x0F,   0x08    },                                  \


#define MSA_NSA_DATA_SECTION                       {    { NSA_REG_ACC_X_LSB,            0xFF,   0x00    },                                  \
                                                        { NSA_REG_ACC_X_MSB,            0xFF,   0x00    },                                  \
                                                        { NSA_REG_ACC_Y_LSB,            0xFF,   0x00    },                                  \
                                                        { NSA_REG_ACC_Y_MSB,            0xFF,   0x00    },                                  \
                                                        { NSA_REG_ACC_Z_LSB,            0xFF,   0x00    },                                  \
                                                        { NSA_REG_ACC_Z_MSB,            0xFF,   0x00    } },                                \
                                                        { 8,                            4,      0       }
                                         
                                         
static int NSA_NTO_calibrate(MSA_HANDLE handle, int z_dir);
static int NSA_NTO_auto_calibrate(MSA_HANDLE handle, int xyz[3]);
#if MSA_AUTO_CALIBRATE
static int msa_auto_calibrate(MSA_HANDLE handle, int x, int y, int z);
#endif /* !MSA_AUTO_CALIBRATE */
static int NSA_interrupt_ops(MSA_HANDLE handle, msa_int_ops_t *ops);
static int NSA_get_reg_data(MSA_HANDLE handle, char *buf);

#define MSA_NSA_NTO                     { "NSA_NTO",    { NSA_REG_WHO_AM_I,             0xFF,   0x13    },                                  \
                                                        { NSA_REG_FIFO_CTRL,            0xFF,   0x00    },                                  \
                                                        { NSA_REG_SPI_I2C,              0x24,   0x24    },                                  \
                                                        { NSA_REG_POWERMODE_BW,         0xC0,   0xC0    },                                  \
                                                        { MSA_NSA_INIT_SECTION                       },                                  \
                                                        { MSA_NSA_OFFSET_SECTION                     },                                  \
                                                        { MSA_NSA_ODR_SECTION                        },                                  \
                                                        { MSA_NSA_DATA_SECTION                       },                                  \
                                                          NSA_NTO_calibrate                              ,                                  \
                                                          NSA_NTO_auto_calibrate                         ,                                  \
                                                          NSA_interrupt_ops                              ,                                  \
                                                          NSA_get_reg_data                               ,                                  \
                                        }
/**************************************************************** COMMON ***************************************************************************/
#define MSA_GSENSOR_SCHEME           MSA_SUPPORT_CHIP_LIST

#if YZ_CROSS_TALK_ENABLE 
static short yzcross;
#endif

/* this level can be modified while runtime through system attribute */
int                                 Log_level = DEBUG_ERR;//|DEBUG_ASSERT|DEBUG_MSG|DEBUG_FUNC|DEBUG_DATA;
int                                gsensor_mod = -1;        /* Initial value */
int                                gsensor_type = -1;        /* Initial value */
static struct gsensor_obj_s         msa_gsensor[] = { MSA_GSENSOR_SCHEME };
struct gsensor_drv_s                msa_gsensor_drv;

#define MI_DATA(format, ...)            if(DEBUG_DATA&Log_level){msa_gsensor_drv.method->myprintf(MI_TAG format "\n", ## __VA_ARGS__);}
#define MI_MSG(format, ...)             if(DEBUG_MSG&Log_level){msa_gsensor_drv.method->myprintf(MI_TAG format "\n", ## __VA_ARGS__);}
#define MI_ERR(format, ...)             if(DEBUG_ERR&Log_level){msa_gsensor_drv.method->myprintf(MI_TAG format "\n", ## __VA_ARGS__);}
#define MI_FUN                          if(DEBUG_FUNC&Log_level){msa_gsensor_drv.method->myprintf(MI_TAG "%s is called, line: %d\n", __FUNCTION__,__LINE__);}
#define MI_ASSERT(expr)                 \
    if (!(expr)) {\
        msa_gsensor_drv.method->myprintf("Assertion failed! %s,%d,%s,%s\n",\
            __FILE__, __LINE__, __func__, #expr);\
    }

#if !MTK_ANDROID_M
#define abs(x) ({ long __x = (x); (__x < 0) ? -__x : __x; })
#endif 

#if FILTER_AVERAGE_ENHANCE
typedef struct FilterAverageContextTag{
    int sample_l;
    int sample_h;
    int filter_param_l;
    int filter_param_h;
    int filter_threhold;

    int refN_l;
    int refN_h;
        
} FilterAverageContext;

typedef struct msa_core_ctx_s{
    struct msa_filter_param_s    filter_param;
    FilterAverageContext            tFac[3];   
} msa_core_ctx;

static msa_core_ctx       core_ctx;
#endif 

#if MSA_SENS_TEMP_SOLUTION
static int bSensZoom = 0;
#endif

#if MSA_OFFSET_TEMP_SOLUTION
static int is_cali = 0;
char bLoad = FILE_CHECKING;
static char readOffsetCnt=0;
static char readsubfileCnt=0;
static unsigned char original_offset[9];
static int msa_write_offset_to_file(unsigned char* offset);
static int msa_read_offset_from_file(unsigned char* offset);
void manual_load_cali_file(MSA_HANDLE handle);
#endif /* !MSA_OFFSET_TEMP_SOLUTION */

/********************************************
Function: MSA_STK_TEMP_SOLUTION
Description: X,Y,Z��ʱ���ݴ���
Input: 
Output: 
Return: 
*************************************************/
#if MSA_STK_TEMP_SOLUTION
static short aixHistort[AIX_HISTORY_SIZE*3] = {0};
static short aixHistoryIndex = 0;
char bxstk = 0;
char bystk = 0;
char bzstk = 0;

// ��ӱ���XYZ��ʷ����
static void addAixHistory(short x,short y,short z){
    aixHistort[aixHistoryIndex++] = x;
    aixHistort[aixHistoryIndex++] = y;
    aixHistort[aixHistoryIndex++] = z;    
    aixHistoryIndex = (aixHistoryIndex)%(AIX_HISTORY_SIZE*3);
}

// �Ƿ���X����
static char isXStick(void){
	int i=0,j=0,temp=0;
	for (i = 0; i < AIX_HISTORY_SIZE; i++){
	    if ((abs(aixHistort[i*3]) < STICK_LSB)&&(aixHistort[i*3] != 0)){
	        break;
	    }
	}

       for(j = 0; j< AIX_HISTORY_SIZE; j++){
		temp |= aixHistort[j*3];
       }

       if(0 == temp)
		return 1;
  
 	return i == AIX_HISTORY_SIZE; 
}

// �Ƿ���Y����
static char isYStick(void){
	int i=0,j=0,temp=0;
	for (i = 0; i < AIX_HISTORY_SIZE; i++){
	    if ((abs(aixHistort[i*3+1]) < STICK_LSB)&&(aixHistort[i*3+1] != 0)){
	        break;
	    }
	}

       for(j = 0; j < AIX_HISTORY_SIZE; j++){
		temp |= aixHistort[j*3+1];
       }

       if(0 == temp)
		return 1;	
	   
	return i == AIX_HISTORY_SIZE;
}

// �Ƿ���Z����
static char isZStick(void){
	int i=0,j=0,temp=0;
	for (i = 0; i < AIX_HISTORY_SIZE; i++){
	    if ((abs(aixHistort[i*3+2]) < STICK_LSB)&&(aixHistort[i*3+2] != 0)){
	        break;
	    }
	}

       for(j = 0; j < AIX_HISTORY_SIZE; j++){
		temp |= aixHistort[j*3+2];
       }

       if(0 == temp)
		return 1;

	return i == AIX_HISTORY_SIZE; 
}

 /********************************************
 Function: squareRoot
 Description: ��ƽ����
 Input: 
 Output: 
 Return: 
 *************************************************/
 int squareRoot(int val){
    int r = 0;
    int shift;
    
    if (val < 0){
        return 0;
    }
    
    for(shift=0;shift<32;shift+=2)
    { 
        int x=0x40000000l >> shift;
        if(x + r <= val)
        { 
            val -= x + r;
            r = (r >> 1) | x;
        } else{ 
            r = r >> 1;
        }
    }
    
    return r;
}
#endif /* ! MSA_STK_TEMP_SOLUTION */

#if FILTER_AVERAGE_ENHANCE
/********************************************
Function: filter_average
Description: ����ƽ��ֵ����
Input: 
Output: 
Return: 
*************************************************/
static short filter_average(short preAve, short sample, int paramN, int* refNum)
{
 #if FILTER_AVERAGE_EX
    if( abs(sample-preAve) > PEAK_LVL  && *refNum < 3  ){ 
         MI_DATA("Hit, sample = %d, preAve = %d, refN =%d\n", sample, preAve, *refNum);
         sample = preAve;
         (*refNum) ++;
    }else{
         if (*refNum == 3){
                preAve = sample;
         }
         
         *refNum  = 0;
    }
#endif

    return preAve + (sample - preAve)/paramN;
}
/********************************************
Function: filter_average_enhance
Description: ����ƽ��ֵ���˼�ǿ
Input: 
Output: 
Return: 
*************************************************/
static int filter_average_enhance(FilterAverageContext* fac, short sample)
{
    if (fac == 0){
        MI_ERR("0 parameter fac");
        return 0;
    }

    if (fac->filter_param_l == fac->filter_param_h){
        fac->sample_l = fac->sample_h = filter_average(fac->sample_l, sample, fac->filter_param_l, &fac->refN_l);
    }else{
        fac->sample_l = filter_average(fac->sample_l, sample, fac->filter_param_l,  &fac->refN_l);
        fac->sample_h= filter_average(fac->sample_h, sample, fac->filter_param_h, &fac->refN_h);  
        if (abs(fac->sample_l- fac->sample_h) > fac->filter_threhold){
            MI_DATA("adjust, fac->sample_l = %d, fac->sample_h = %d\n", fac->sample_l, fac->sample_h); 
            fac->sample_h = fac->sample_l;            
        }
     }

    return fac->sample_h;    
}
#endif /* ! FILTER_AVERAGE_ENHANCE */

/********************************************
Function: msa_register_read
Description: MSA�Ĵ�����ȡ
Input: 
Output: 
Return: 
*************************************************/
int msa_register_read(MSA_HANDLE handle, short addr, unsigned char *data)
{
    int     res = 0;

    res = msa_gsensor_drv.method->smi.read(handle, MSA_REG_ADDR(addr), data);
	
    return res;
}

/********************************************
Function: msa_register_read_continuously
Description: MSA�Ĵ���������ȡ
Input: 
Output: 
Return: 
*************************************************/
int msa_register_read_continuously(MSA_HANDLE handle, short addr, unsigned char count, unsigned char *data)
{
    int     res = 0;

    res = (count==msa_gsensor_drv.method->smi.read_block(handle, MSA_REG_ADDR(addr), count, data)) ? 0 : 1;

    return res;
}

/********************************************
Function: msa_register_write
Description: MSA�Ĵ���д
Input: 
Output: 
Return: 
*************************************************/
int msa_register_write(MSA_HANDLE handle, short addr, unsigned char data)
{
    int     res = 0;
	
    res = msa_gsensor_drv.method->smi.write(handle, MSA_REG_ADDR(addr), data);

    return res;
}

/********************************************
Function: msa_register_mask_write
Description: MSA�Ĵ���д
Input: 
Output: 
Return: 
*************************************************/
int msa_register_mask_write(MSA_HANDLE handle, short addr, unsigned char mask, unsigned char data)
{
    int     res = 0;
    unsigned char      tmp_data;

    res = msa_register_read(handle, addr, &tmp_data);
    if(res) {
        return res;
    }

    tmp_data &= ~mask; 
    tmp_data |= data & mask;
    res = msa_register_write(handle, addr, tmp_data);

    return res;
}

/********************************************
Function: msa_read_raw_data
Description: MSAԭʼ���ݶ�ȡ
Input: 
Output: 
Return: 
*************************************************/
static int msa_read_raw_data(MSA_HANDLE handle, short *x, short *y, short *z)
{
    unsigned char    tmp_data[6] = {0};

    if (msa_register_read_continuously(handle, msa_gsensor_drv.obj[gsensor_mod].data.data_sect[0].addr, 6, tmp_data) != 0) {
        MI_ERR("i2c block read failed\n");
        return -1;
    }

    *x = ((short)(tmp_data[1] << msa_gsensor_drv.obj[gsensor_mod].data.data_fmt.msbw | tmp_data[0]))>> (8-msa_gsensor_drv.obj[gsensor_mod].data.data_fmt.lsbw);
    *y = ((short)(tmp_data[3] << msa_gsensor_drv.obj[gsensor_mod].data.data_fmt.msbw | tmp_data[2]))>> (8-msa_gsensor_drv.obj[gsensor_mod].data.data_fmt.lsbw);
    *z = ((short)(tmp_data[5] << msa_gsensor_drv.obj[gsensor_mod].data.data_fmt.msbw | tmp_data[4]))>> (8-msa_gsensor_drv.obj[gsensor_mod].data.data_fmt.lsbw);

    MI_DATA("msa_raw: x=%d, y=%d, z=%d",  *x, *y, *z);
    
#if MSA_SENS_TEMP_SOLUTION
    if (bSensZoom == 1){
        *z = (*z )*5/4;
        MI_DATA("SensZoom take effect, Zoomed Z = %d", *z);
    }
#endif

#if YZ_CROSS_TALK_ENABLE 
    if(yzcross)
      *y=*y-(*z)*yzcross/100;
#endif
    
    return 0;
}

/********************************************
Function: msa_direction_remap
Description: MSA��ʼ������ͼ
Input: 
Output: 
Return: 
*************************************************/

static int remap[8][4] = {{0,0,0,0},
                    {0,1,0,1},
                    {1,1,0,0},
                    {1,0,0,1},
                    {1,0,1,0},
                    {0,0,1,1},
                    {0,1,1,0},
                    {1,1,1,1}};
                    
int msa_direction_remap(short *x,short *y, short *z, int direction)
{
    short temp = 0;
    
    *x = *x - ((*x) * remap[direction][0]*2);
    *y = *y - ((*y) * remap[direction][1]*2);
    *z = *z - ((*z) * remap[direction][2]*2);
    
    if(remap[direction][3])
    {
        temp = *x;
        *x = *y;
        *y = temp;
    }
    
    if(remap[direction][2])
        return -1;
    
    return 1;
}


/********************************************
Function: msa_read_data
Description: MSAУ׼�����ݶ�ȡ
Input: 
Output: 
Return: 
*************************************************/
int msa_read_data(MSA_HANDLE handle, short *x, short *y, short *z)
{
    int    rst = 0;
	static int resume_times=0;

#if MSA_SUPPORT_MULTI_LAYOUT
    short temp =0;
#endif  

#if MSA_OFFSET_TEMP_SOLUTION
    if(is_cali){
        *x = *y = *z = 0;
        return 0;
    }

    manual_load_cali_file(handle);
#endif

    rst = msa_read_raw_data(handle, x, y, z);
    if (rst != 0){
        MI_ERR("msa_read_raw_data failed, rst = %d", rst);
        return rst;
    }

#if MSA_AUTO_CALIBRATE
#if MSA_SUPPORT_FAST_AUTO_CALI       
    if((msa_gsensor_drv.method->support_fast_auto_cali() &&(bLoad !=FILE_EXIST))||(bLoad ==FILE_NO_EXIST))
#else
    if(bLoad ==FILE_NO_EXIST)
#endif  
    {   
       msa_auto_calibrate(handle, *x, *y, *z);
    }
#endif
    
#if MSA_STK_TEMP_SOLUTION   
    addAixHistory(*x,*y,*z);

    bxstk = isXStick();
    bystk = isYStick();
    bzstk = isZStick();

    if((gsensor_chip_info.mems==MEMS_TV03 ||gsensor_chip_info.mems==MEMS_RTO3)
   	&&(gsensor_chip_info.reg_value != 0x4B)
   	&&(gsensor_chip_info.reg_value != 0x8C)
   	&&(gsensor_chip_info.reg_value != 0xCA))
    {
	   if ((bxstk + bystk+ bzstk) >0){
		   if(resume_times<20){
			   resume_times++;
			   MI_DATA("IN USE STK & resume!!\n");
			   msa_chip_resume(handle);
		   }
	   }else
		   resume_times = 0;
    }
    else
    {
	    if ((bxstk + bystk+ bzstk) < 2){
	        if(bxstk)
	        *x = squareRoot(1024*1024 - (*y)*(*y) - (*z)*(*z));
	    if(bystk)
	        *y = squareRoot(1024*1024 - (*x)*(*x) - (*z)*(*z));
	    if(bzstk)
	        *z = squareRoot(1024*1024 - (*x)*(*x) - (*y)*(*y));
	    }else{
	        // MI_ERR( "CHIP ERR !MORE STK!\n"); 
	        return 0;
	    }
    }
#endif


#if FILTER_AVERAGE_ENHANCE
    *x = filter_average_enhance(&core_ctx.tFac[0], *x);
    *y = filter_average_enhance(&core_ctx.tFac[1], *y);
    *z = filter_average_enhance(&core_ctx.tFac[2], *z);
    MI_DATA("msa_filt: x=%d, y=%d, z=%d",  *x, *y, *z);
#endif


#if MSA_SUPPORT_MULTI_LAYOUT
    if(gsensor_chip_info.package ==PACKAGE_2X2_12PIN ){
        *x =*x;  
        *y =*z;
        *z =*z;
    }else if(gsensor_chip_info.package ==PACKAGE_3X3_10PIN){
        temp = *x;
        *x = *y;
        *y = temp;
        *z =*z; 
    }else if(gsensor_chip_info.package ==PACKAGE_3X3_16PIN){
        temp = -1*(*x);
        *x = -1*(*y);
        *y = temp;
        *z =*z; 
    }
#endif

	if((gsensor_chip_info.reg_value == 0x4B)
		||(gsensor_chip_info.reg_value == 0x8C)
   		||(gsensor_chip_info.reg_value == 0xCA))
	{
		*z = 0;
#if MSA_STK_TEMP_SOLUTION   
		bzstk = 1;
#endif
	}
    
    return 0;
}

/********************************************
Function: cycle_read_xyz
Description: MSA���ڶ�ȡXYZֵ
Input: 
Output: 
Return: 
*************************************************/
int cycle_read_xyz(MSA_HANDLE handle, int* x, int* y, int* z, int ncycle)
{
    unsigned int j = 0;
    short raw_x,raw_y,raw_z;    

    *x = *y = *z = 0;

    for (j = 0; j < ncycle; j++)
    {
        raw_x = raw_y = raw_z = 0;
        msa_read_raw_data (handle, &raw_x, &raw_y, &raw_z);

        (*x) += raw_x;
        (*y) += raw_y;
        (*z) += raw_z;

        msa_gsensor_drv.method->msdelay(5);
    }
    
    (*x) /= ncycle;
    (*y) /= ncycle;
    (*z) /= ncycle;
    
    return 0;
}


/********************************************
Function: msa_read_offset
Description: MSA��ȡУ׼����
Input: 
Output: 
Return: 
*************************************************/

int msa_read_offset(MSA_HANDLE handle, unsigned char* offset)
{
    int     i, res = 0;
    
    for(i=0;i<MSA_OFF_SECT_LEN;i++) {
        if( msa_gsensor_drv.obj[gsensor_mod].offset_sect[i].addr < 0 ) {
            break;
        }

        res = msa_register_read(handle, msa_gsensor_drv.obj[gsensor_mod].offset_sect[i].addr, &offset[i]);
        if(res != 0) {
            return res;
        } 
    }
    
    return res;
}

/********************************************
Function: msa_write_offset
Description: MSAдУ׼����
Input: 
Output: 
Return: 
*************************************************/
int msa_write_offset(MSA_HANDLE handle, unsigned char* offset)
{
    int     i, res = 0;

    for(i=0;i<MSA_OFF_SECT_LEN;i++) {
        if( msa_gsensor_drv.obj[gsensor_mod].offset_sect[i].addr < 0 ) {
            break;
        }

        res = msa_register_write(handle, msa_gsensor_drv.obj[gsensor_mod].offset_sect[i].addr, offset[i]);
        if(res != 0) {
            return res;
        } 
    }

    return res;
}

#if MSA_OFFSET_TEMP_SOLUTION
/********************************************
Function: msa_write_offset_to_file
Description: MSAдУ׼���ݵ��ļ�
Input: 
Output: 
Return: 
*************************************************/
static int msa_write_offset_to_file(unsigned char* offset)
{
    int     ret = 0;
    
    if(0 == msa_gsensor_drv.method->data_save)
        return 0;
        
    ret = msa_gsensor_drv.method->data_save(offset);
    
    MI_MSG("====sensor_sync_write, offset = 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x", offset[0],offset[1],offset[2],offset[3],offset[4],offset[5],offset[6],offset[7],offset[8]);
    
    return ret;
}

/********************************************
Function: msa_read_offset_from_file
Description: MSA���ļ���ȡУ׼����
Input: 
Output: 
Return: 
*************************************************/
static int msa_read_offset_from_file(unsigned char* offset)
{
    int     ret = 0;
    int     i=0,sum=0;

    if(0 == msa_gsensor_drv.method->data_get)
        return -1;
    
    ret = msa_gsensor_drv.method->data_get(offset);

    for(i=0;i<MSA_OFF_SECT_LEN;i++){
        sum += offset[i];
    }

    if(sum==0)
       return -1;   

    MI_MSG("====sensor_sync_read, offset = 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x", offset[0],offset[1],offset[2],offset[3],offset[4],offset[5],offset[6],offset[7],offset[8]);
    
    return ret;
}

/********************************************
Function: manual_load_cali_file
Description: MSA�ֶ�����У׼�ļ�
Input: 
Output: 
Return: 
*************************************************/
void manual_load_cali_file(MSA_HANDLE handle)
{ 
	unsigned char  offset[MSA_OFFSET_LEN] = {0};
	
	if (bLoad ==FILE_CHECKING){
		
		readOffsetCnt++;	
		
 		if(readOffsetCnt%8 == 0){
            
			readOffsetCnt =0;

			MI_ERR("====444 manual_load_cali_file(), bLoad = %d, readOffsetCnt=%d.\n", bLoad, readOffsetCnt); 

			if(msa_gsensor_drv.method->data_check()){

				readsubfileCnt++;

				if(!msa_read_offset_from_file(offset)) {
					MI_ERR("========= FILE EXIST & WRITE OFFSET!");
					msa_write_offset(handle, offset);
					bLoad = FILE_EXIST;
				}else if(5 == readsubfileCnt){
					MI_ERR("========= NO FILE EXIST!");					
					bLoad = FILE_NO_EXIST;	
				}
			}else{
				MI_ERR("========= FILE CHECKING....");				
				bLoad = FILE_CHECKING;
				readsubfileCnt =0;
			}
	     }
    }
}

static void msa_cali_off_to_lsb(int off, int *coarse, int coarse_step, int *fine, int fine_step)
{
    *coarse = off/coarse_step;
    *fine = 100*(off-(*coarse)*coarse_step)/fine_step;
    
    MI_MSG("off = %d; delta_coarse = %d; delta_fine = %d", off, *coarse, *fine);
}

#if MSA_AUTO_CALIBRATE
/********************************************
Function: NSA_once_calibrate
Description: MSAһ��У׼
Input: 
Output: 
Return: 
*************************************************/
static int NSA_once_calibrate(MSA_HANDLE handle, int coarse_step[3], int fine_step[3], int xyz[3])
{
    int     coarse[3] = {0};
    int     coarse_delta[3] = {0};
    int     fine[3] = {0};
    int     fine_delta[3] = {0};
    int     target[3] = {0};
    int     i;
    unsigned char   offset_data[9] = {0};

    if(msa_read_offset(handle, offset_data)){
        MI_ERR("Get old offset failed !");
        return -1;
    }
    coarse[0] = offset_data[0] & 0x3f;
    coarse[1] = offset_data[1] & 0x3f;
    coarse[2] = offset_data[2] & 0x3f;
    fine[0] = (((int)offset_data[0] << 2) & 0x300)|offset_data[3];
    fine[1] = (((int)offset_data[1] << 2) & 0x300)|offset_data[4];
    fine[2] = (((int)offset_data[2] << 2) & 0x300)|offset_data[5];
    
    MI_MSG("Old coarse_x = %d; coarse_y = %d; coarse_z = %d; fine_x = %d; fine_y = %d; fine_z = %d;", coarse[0], coarse[1], coarse[2], fine[0], fine[1], fine[2]);
        
    /* 0 means auto detect z direction assume z axis is verticle */
    if ((abs(target[0]) + abs(target[1]) + abs(target[2])) == 0){
        target[2] = (xyz[2] > 0) ? 1024 : (-1024);
    }
    
    for(i = 0;i < 3; i++){
        coarse_step[i] *= coarse[i] >= 32 ? (-1) : 1;
        msa_cali_off_to_lsb((xyz[i]-target[i]), &coarse_delta[i], coarse_step[i], &fine_delta[i], fine_step[i]);
        
        coarse[i] += coarse_delta[i];
        fine[i] += fine_delta[i];
        offset_data[i] = coarse[i]|((fine[i]>>2)&0xc0);
        offset_data[i+3] = fine[i]&0xFF;
    }
    
    if(msa_write_offset(handle, offset_data)){
        MI_ERR("Update offset failed !");
        return -1;
    }
    /* Discard unstable data after offset register changed */
    cycle_read_xyz(handle, &xyz[0], &xyz[1], &xyz[2], 5);
    if(cycle_read_xyz(handle, &xyz[0], &xyz[1], &xyz[2], 10)){
        return -1;
    }
    MI_MSG("---calibrate_Done, x = %d, y = %d, z = %d, coarse_x = %d, coarse_y = %d, coarse_z = %d, fine_x = %d, fine_y = %d, fine_z = %d", xyz[0], xyz[1], xyz[2], coarse[0], coarse[1], coarse[2], fine[0], fine[1], fine[2]);
    
    return msa_write_offset_to_file(offset_data);
}
#endif /* !MSA3DA_AUTO_CALIBRATE */

/********************************************
Function: NSA_calibrate
Description: MSAУ׼
Input: 
Output: 
Return: 
*************************************************/
static int NSA_calibrate(MSA_HANDLE handle, int coarse_step[3], int fine_step[3], int fine_max, int target[3])
{
    int             i = 0, j = 0;
    unsigned char   ncycle = 20;
    unsigned char   nLoop = 20;
    unsigned char   offset_data[9] = {0};
    unsigned char   fine_ok_map = 0;
    
    int             xyz[3] = {0};  
    int             coarse[3] = {0};
    int             coarse_delta[3] = {0};
    int             fine[3] = {0};
    int             fine_delta[3] = {0};
    
    if( (abs(target[0]) + abs(target[1]) + abs(target[2])) != 0 && (abs(target[0]) + abs(target[1]) + abs(target[2])) != 1024 ) {
        MI_ERR("Invalid argument !");
        return -1;
    }
        
    /* 0 means auto detect z direction assume z axis is verticle */
    if ((abs(target[0]) + abs(target[1]) + abs(target[2])) == 0){
       if(cycle_read_xyz(handle, &xyz[0], &xyz[1], &xyz[2], 5)){
            MI_ERR("check z direction failed\n");
            return -1;
       }
       target[2] = (xyz[2] > 0) ? 1024 : (-1024);
    }

    MI_MSG("---Start Calibrate, trim target %d, %d, %d---\n", target[0], target[1], target[2]);

    // Stage1: Coarse tune once
    MI_MSG("---Stage1, coarse tune---");            
    // change to 16G mode
    if(msa_register_mask_write(handle, NSA_REG_G_RANGE, 0x03, 3)){
        MI_ERR("i2c mask write failed !\n");
        return -1;
    }

    /* reset coarse offset register */
    msa_write_offset(handle, offset_data);  
    /* Discard unstable data after offset register changed */
    cycle_read_xyz(handle, &xyz[0], &xyz[1], &xyz[2], 5);
  
    if( cycle_read_xyz(handle, &xyz[0], &xyz[1], &xyz[2], ncycle) ){
        goto EXIT_16G_MOD;
    }

    for(i = 0; i < 3; i++){
        /* check rule */
        xyz[i] *= 8;

        coarse[i] = ((xyz[i]-target[i]) > 0) ? 0 : 32;

        MI_MSG("xyz[%d] = %d, coarse[%d] = 0x%x", i, xyz[i], i, coarse[i]);
        
        coarse_step[i] *= coarse[i] >= 32 ? (-1) : 1;
        msa_cali_off_to_lsb((xyz[i]-target[i]), &coarse_delta[i], coarse_step[i], &fine_delta[i], fine_step[i]);

        coarse[i] += coarse_delta[i];
        fine[i] += fine_delta[i];
        msa_register_mask_write(handle, NSA_REG_COARSE_OFFSET_TRIM_X+i, 0x3f, (unsigned char)coarse[i]);
    }
    
    /* Discard unstable data after offset register changed */
    cycle_read_xyz(handle, &xyz[0], &xyz[1], &xyz[2], 5);
    if(cycle_read_xyz(handle, &xyz[0], &xyz[1], &xyz[2], 5)){
        return -1;
    }
    for(i = 0; i < 3; i++){   
        fine[i] += (xyz[i] > 0) ? 0 : fine_max;
        msa_register_write(handle, NSA_REG_FINE_OFFSET_TRIM_X+i, (unsigned char)(fine[i]&0xff));
        msa_register_mask_write(handle, NSA_REG_COARSE_OFFSET_TRIM_X+i, 0xc0, (unsigned char)(0xc0&(fine[i]>>2)));
    }

EXIT_16G_MOD:
    // change back to 2G mode
    if(msa_register_mask_write(handle, NSA_REG_G_RANGE, 0x03, 0)){
        MI_ERR("i2c mask write failed !\n");
        return -1;
    }
    /* Discard unstable data after offset register changed */
    cycle_read_xyz(handle, &xyz[0], &xyz[1], &xyz[2], 5);
    if(cycle_read_xyz(handle, &xyz[0], &xyz[1], &xyz[2], ncycle)){
        return -1;
    }
    MI_MSG("---Stage1, coarse tune done: x = %d, y = %d, z = %d, coarse_x = %d, coarse_y = %d, coarse_z = %d, fine_x = %d, fine_y = %d, fine_z = %d", xyz[0], xyz[1], xyz[2], coarse[0], coarse[1], coarse[2], fine[0], fine[1], fine[2]);
    
    // Stage2: Fine tune
    MI_MSG("---Stage2, Fine tune---");
    for (i = 0; i < nLoop; i++){
    
        if( 0x07==(fine_ok_map & 0x07) ){
            break;
        }
        /* Discard unstable data after offset register changed */
        cycle_read_xyz(handle, &xyz[0], &xyz[1], &xyz[2], 5);
        MI_MSG("---Stage2, Fine loop %d", i);
        if(cycle_read_xyz(handle, &xyz[0], &xyz[1], &xyz[2], ncycle)){
            return -1;
        }
        
        for(j = 0; j < 3; j++){
            MI_MSG("xyz[%d] = %d, caorse[%d] = 0x%x, fine[%d] = 0x%x", j, xyz[j], j, coarse[j], j, fine[j]);
            if( abs(xyz[j]-target[j]) < MSA_OFFSET_THRESHOLD ){
                fine_ok_map |= (1<<j);
                offset_data[j] = coarse[j]|((fine[j]>>2)&0xc0);
                offset_data[j+3] = fine[j];
                continue;        
            }
            msa_cali_off_to_lsb((xyz[j]-target[j]), &coarse_delta[j], coarse_step[j], &fine_delta[j], fine_step[j]);

            coarse[j] += coarse_delta[j];
            fine[j] += fine_delta[j];
            msa_register_write(handle, NSA_REG_FINE_OFFSET_TRIM_X+j, (unsigned char)(fine[j]&0xff));
            msa_register_mask_write(handle, NSA_REG_COARSE_OFFSET_TRIM_X+j, 0xFF, (unsigned char)(0xc0&(fine[j]>>2))|coarse[j]);
        }                       
    }
    MI_MSG("---Stage2, Fine tune done: x = %d, y = %d, z = %d, coarse_x = %d, coarse_y = %d, coarse_z = %d, fine_x = %d, fine_y = %d, fine_z = %d", xyz[0], xyz[1], xyz[2], coarse[0], coarse[1], coarse[2], fine[0], fine[1], fine[2]);
    
    if( 0x07==(fine_ok_map & 0x07) ){
        goto SUCCESS_EXIT;
    }
#if MSA_STK_TEMP_SOLUTION
    if( 0x03==(fine_ok_map & 0x07) ){
        goto SUCCESS_EXIT;
    }
#endif
    
    MI_MSG("---calibrate Failed !---");
    return -1;
    
SUCCESS_EXIT:
    MI_MSG("---calibrate OK !---");
    return msa_write_offset_to_file(offset_data);
}

/********************************************
Function: NSA_NTO_cali_step_calc
Description: MSAУ׼ѡ���
Input: 
Output: 
Return: 
*************************************************/
static int NSA_NTO_cali_step_calc(MSA_HANDLE handle, int coarse[3], int x100_fine[3], int x100_cust[3])
{
    int                i;
    unsigned int       total_gain[3] = {0}; 
    unsigned char      coarse_gain = 0;
    unsigned char      fine_gain[3] = {0};
    unsigned int       const coarse_gain_map[] = {1000, 1125, 1250, 1375, 500, 625, 750, 875};   /* *1000  */
    unsigned char      const fine_dig_gain_map[] = {1, 2, 4, 8};
        
    if(msa_register_read_continuously(handle, NSA_REG_SENSITIVITY_TRIM_X, 3, fine_gain) != 0){
        MI_ERR("i2c block read failed\n");
        return -1;
    }

    if(msa_register_read(handle, NSA_REG_SENS_COARSE_TRIM, &coarse_gain) != 0){
        MI_ERR("i2c block read failed\n");
        return -1;
    }
    
    for(i = 0;i < 3;i++) {        
        // *100*1000
        total_gain[i] = ((1000 + (fine_gain[i]&0x1F)*1000/32)/15) * fine_dig_gain_map[((fine_gain[i]>>5)&0x03)] * coarse_gain_map[coarse_gain&0x07]; 
        coarse[i] = (int)(total_gain[i] * 500 / 100000);
        x100_fine[i] = (int)(total_gain[i] * 293 / 100000);
        x100_cust[i] = (int)(total_gain[i] * 390 / 100000);
    }
    MI_MSG("coarse_step_x = %d, coarse_step_y = %d, coarse_step_z = %d\n", coarse[0], coarse[1], coarse[2]);
    MI_MSG("fine_step_x = %d, fine_step_y = %d, fine_step_z = %d\n", x100_fine[0], x100_fine[1], x100_fine[2]);
    MI_MSG("custom_step_x = %d, custom_step_y = %d, custom_step_z = %d\n", x100_cust[0], x100_cust[1], x100_cust[2]);
    
    return 0;    
}
#endif /* !MSA_OFFSET_TEMP_SOLUTION */

/********************************************
Function: NSA_NTO_calibrate
Description: MSAУ׼ѡ��
Input: 
Output: 
Return: 
*************************************************/
static int NSA_NTO_calibrate(MSA_HANDLE handle, int z_dir)
{
    int     result = 0;

#if MSA_OFFSET_TEMP_SOLUTION
        int     coarse_step[3] = {0};
        int     fine_step[3] = {0};
        int     custom_step[3] = {0};
        int     target[3] = {0};

   unsigned char     swap_plarity_old = 0;  
   
    /* compute step */
    if( NSA_NTO_cali_step_calc(handle, coarse_step, fine_step, custom_step) ) {
        MI_ERR("Compute step failed !");
        return -1; 
    }
    target[2] = z_dir*1024;

    // save swap/plarity old setting
    if(msa_register_read(handle, NSA_REG_SWAP_POLARITY, &swap_plarity_old)){
        MI_ERR("Get SWAP/PLARITY setting failed !");
        return -1;
    }

    if(gsensor_chip_info.asic == ASIC_2512B){
            coarse_step[2] = 2 *coarse_step[2];
            target[2] = ((swap_plarity_old & (1<<1)) == 0) ? (-target[2]) :target[2];
            if(msa_register_mask_write(handle, NSA_REG_SWAP_POLARITY, 0x0F, 0x0E)){
                    MI_ERR("Set Plarity failed !");
                    return -1;
            }
    }else if(gsensor_chip_info.asic == ASIC_2511){
            target[2] = ((swap_plarity_old & (1<<1)) != 0) ? (-target[2]) :target[2];
            if(msa_register_mask_write(handle, NSA_REG_SWAP_POLARITY, 0x0F, 0x00)){
                    MI_ERR("Set Plarity failed !");
                    return -1;
            }
    }   

    result=NSA_calibrate(handle, coarse_step, fine_step, 0x3ff, target);

    // Restore swap/plarity setting
    if(msa_register_mask_write(handle, NSA_REG_SWAP_POLARITY, 0x0F, swap_plarity_old&0x0F)){
        MI_ERR("Restore SWAP/PLARITY setting failed !");
        return -1;
    }
#endif /* !MSA_OFFSET_TEMP_SOLUTION */
    return result;
}


/********************************************
Function: NSA_NTO_auto_calibrate
Description: MSA�Զ�У׼ѡ��
Input: 
Output: 
Return: 
*************************************************/
static int NSA_NTO_auto_calibrate(MSA_HANDLE handle, int xyz[3])
{
    int     result = 0;

#if MSA_AUTO_CALIBRATE
    int     coarse_step[3];
    int     fine_step[3];
    int     custom_step[3] = {0};
   unsigned char     swap_plarity_old = 0;
    int     temp=0;  

    /* compute step */
    if( NSA_NTO_cali_step_calc(handle, coarse_step, fine_step, custom_step) ) {
        MI_ERR("Compute step failed !");
        return -1; 
    }

    // save swap/plarity old setting
    if(msa_register_read(handle, NSA_REG_SWAP_POLARITY, &swap_plarity_old)){
        MI_ERR("Get SWAP/PLARITY setting failed !");
        return -1;
    }

        if(gsensor_chip_info.asic == ASIC_2512B){
                coarse_step[2] = 2 *coarse_step[2];

                if((swap_plarity_old & (1<<0))){
                   temp = xyz[0];
                   xyz[0] = ((swap_plarity_old & (1<<2)) == 0) ? (-xyz[1]) :xyz[1];
                   xyz[1] = ((swap_plarity_old & (1<<3)) == 0) ? (-temp) :temp;
                }else{
                   xyz[0] = ((swap_plarity_old & (1<<3)) == 0) ? (-xyz[0]) :xyz[0];
                   xyz[1] = ((swap_plarity_old & (1<<2)) == 0) ? (-xyz[1]) :xyz[1];
                }
                xyz[2] = ((swap_plarity_old & (1<<1)) == 0) ? (-xyz[2]) :xyz[2];
        }else if(gsensor_chip_info.asic == ASIC_2511){
                if((swap_plarity_old & (1<<0))){
                   temp = xyz[0];
                   xyz[0] = ((swap_plarity_old & (1<<2)) != 0) ? (-xyz[1]) :xyz[1];
                   xyz[1] = ((swap_plarity_old & (1<<3)) != 0) ? (-temp) :temp;
                }else{
                   xyz[0] = ((swap_plarity_old & (1<<3)) != 0) ? (-xyz[0]) :xyz[0];
                   xyz[1] = ((swap_plarity_old & (1<<2)) != 0) ? (-xyz[1]) :xyz[1];
                }

                xyz[2] = ((swap_plarity_old & (1<<1)) != 0) ? (-xyz[2]) :xyz[2];
        }   
    
    result = NSA_once_calibrate(handle, coarse_step, fine_step, xyz);
#endif /* !MSA_AUTO_CALIBRATE */
    return result;
}

/********************************************
Function: msa_calibrate
Description: MSAУ׼����
Input: 
Output: 
Return: 
*************************************************/
int msa_calibrate(MSA_HANDLE handle, int z_dir)
{
    int     res = 0;

	
#if MSA_OFFSET_TEMP_SOLUTION
    int     xyz[3]={0};	
    if( is_cali )
        return -1;
    is_cali = 1;

    /* restore original direction if last calibration was done in a wrong direction */
    msa_write_offset(handle, original_offset);
	      
    cycle_read_xyz(handle, &xyz[0], &xyz[1], &xyz[2], 20);	

    res = msa_gsensor_drv.obj[gsensor_mod].calibrate(handle, z_dir); 
    if (res != 0){
          MI_ERR("Calibrate failed !");
          msa_write_offset(handle, original_offset); 
    }else
          bLoad = FILE_EXIST;
    
    is_cali = 0;
#endif /* !MSA_OFFSET_TEMP_SOLUTION */
    return res;
}

#if MSA_AUTO_CALIBRATE
#define STABLE_CHECK_SAMPLE_NUM     10
#define STABLE_CHECK_THRESHOLD      50000
#define AUTO_CALI_THRESHOLD_XY      500
#define AUTO_CALI_THRESHOLD_Z        500

static unsigned char    stable_sample_cnt = 0;
static int              stable_sample_pow_sum[STABLE_CHECK_SAMPLE_NUM] = {0};
static int              stable_sample_sum[3] = {0};

/********************************************
Function: msa_auto_cali_condition_confirm
Description: MSA�Զ�У׼�����жϺ���
Input: 
Output: 
Return: 
*************************************************/
static int msa_auto_cali_condition_confirm(int x, int y, int z, int ave_xyz[3])
{
    int    max = 0, min = 0;
    int    i;
        int x_ok=0,y_ok=0,z_ok=0;   
    
    stable_sample_pow_sum[stable_sample_cnt] = x*x + y*y + z*z;
    stable_sample_sum[0] += x;
    stable_sample_sum[1] += y;
    stable_sample_sum[2] += z;
    stable_sample_cnt++;
    
    MI_MSG("---stable_sample_cnt = %d", stable_sample_cnt);
    
    if( stable_sample_cnt < STABLE_CHECK_SAMPLE_NUM )
        return -1;
    stable_sample_cnt = 0;
    
    max = stable_sample_pow_sum[0];
    min = stable_sample_pow_sum[0];
    stable_sample_pow_sum[0] = 0;
    for(i = 1; i < STABLE_CHECK_SAMPLE_NUM; i++){
        if( stable_sample_pow_sum[i] > max )
            max = stable_sample_pow_sum[i];
        if( stable_sample_pow_sum[i] < min )
            min = stable_sample_pow_sum[i];
        stable_sample_pow_sum[i] = 0;
    }
    MI_MSG("---max = %d; min = %d", max, min);

    ave_xyz[0] = stable_sample_sum[0]/STABLE_CHECK_SAMPLE_NUM;
    stable_sample_sum[0] = 0;
    ave_xyz[1] = stable_sample_sum[1]/STABLE_CHECK_SAMPLE_NUM;
    stable_sample_sum[1] = 0;
    ave_xyz[2] = stable_sample_sum[2]/STABLE_CHECK_SAMPLE_NUM;
    stable_sample_sum[2] = 0;
            
        MI_MSG("ave_x = %d, ave_y = %d, ave_z = %d", ave_xyz[0], ave_xyz[1], ave_xyz[2]);
        x_ok =  (abs(ave_xyz[0]) < AUTO_CALI_THRESHOLD_XY) ? 1:0;
        y_ok =  (abs(ave_xyz[1]) < AUTO_CALI_THRESHOLD_XY) ? 1:0;
        z_ok =  (abs(abs(ave_xyz[2])-1024) < AUTO_CALI_THRESHOLD_Z) ? 1:0;

        if( (abs(max-min) > STABLE_CHECK_THRESHOLD) ||((x_ok + y_ok + z_ok) < 2) ) {
            return -1;
        }

        return 0;
}

/********************************************
Function: msa_auto_calibrate
Description: MSA�Զ�У׼����
Input: 
Output: 
Return: 
*************************************************/
static int msa_auto_calibrate(MSA_HANDLE handle, int x, int y, int z)
{
    int     res = 0;
    int     xyz[3] = {0};

    if((gsensor_chip_info.mems== MEMS_RTO3)
	||(gsensor_chip_info.reg_value == 0x4B)
	||(gsensor_chip_info.reg_value == 0x8C)
	||(gsensor_chip_info.reg_value == 0xCA))
    	return -1;

    if( is_cali )
        return -1;
    is_cali = 1;
   
#if MSA_SUPPORT_FAST_AUTO_CALI
    if(msa_gsensor_drv.method->support_fast_auto_cali()){
        cycle_read_xyz(handle,&xyz[0],&xyz[1],&xyz[2],5);
    }
    else{
        if( msa_auto_cali_condition_confirm(x, y, z, xyz) ){
            res = -1;
            goto EXIT;
        }   
    }
#else   
    if( msa_auto_cali_condition_confirm(x, y, z, xyz) ){
        res = -1;
        goto EXIT;
    }   
#endif

    msa_write_offset(handle, original_offset);

    res = msa_gsensor_drv.obj[gsensor_mod].auto_calibrate(handle, xyz); 
    if (res != 0){
         MI_ERR("Calibrate failed !");
         msa_write_offset(handle, original_offset);
    }else
            bLoad = FILE_EXIST;
    
EXIT:    
    is_cali = 0;

    return res;
}
#endif /* !MSA_AUTO_CALIBRATE */


/********************************************
Function: NSA_interrupt_ops
Description: MSA�жϲ���
Input: 
Output: 
Return: 
*************************************************/

static int NSA_interrupt_ops(MSA_HANDLE handle, msa_int_ops_t *ops)
{   
    switch(ops->type)
    {
        case INTERRUPT_OP_INIT:
        
            /* latch */
            msa_register_mask_write(handle, NSA_REG_INT_LATCH, 0x0f, ops->data.init.latch);
            /* active level & output mode */
            msa_register_mask_write(handle, NSA_REG_INT_PIN_CONFIG, 0x0f, ops->data.init.level|(ops->data.init.pin_mod<<1)|(ops->data.init.level<<2)|(ops->data.init.pin_mod<<3));
            
            break;
            
        case INTERRUPT_OP_ENABLE:
            switch( ops->data.int_src )
            { 
                case INTERRUPT_ACTIVITY:
                    /* Enable active interrupt */
                    msa_register_mask_write(handle, NSA_REG_INTERRUPT_SETTINGS1, 0x07, 0x07);                    
                    break;
                case INTERRUPT_CLICK:
                    /* Enable single and double tap detect */
                    msa_register_mask_write(handle, NSA_REG_INTERRUPT_SETTINGS1, 0x30, 0x30);                   
                    break;
            }
            break;
            
        case INTERRUPT_OP_CONFIG:
            
                            
            switch( ops->data.cfg.int_src )
            {
                case INTERRUPT_ACTIVITY:
                         
                    msa_register_write(handle, NSA_REG_ACTIVE_THRESHOLD, ops->data.cfg.int_cfg.act.threshold);
                    msa_register_mask_write(handle, NSA_REG_ACTIVE_DURATION, 0x03, ops->data.cfg.int_cfg.act.duration);
                    
                    /* Int mapping */
                    if(ops->data.cfg.pin == INTERRUPT_PIN1) {  
                        msa_register_mask_write(handle, NSA_REG_INTERRUPT_MAPPING1, (1<<2), (1<<2));
                    }
                    else if(ops->data.cfg.pin == INTERRUPT_PIN2) {
                        msa_register_mask_write(handle, NSA_REG_INTERRUPT_MAPPING3, (1<<2), (1<<2));
                    }
                    break;
                    
                case INTERRUPT_CLICK:
                    
                    msa_register_mask_write(handle, NSA_REG_TAP_THRESHOLD, 0x1f, ops->data.cfg.int_cfg.clk.threshold);
                    msa_register_mask_write(handle, NSA_REG_TAP_DURATION, (0x03<<5)|(0x07), (ops->data.cfg.int_cfg.clk.quiet_time<<7)|(ops->data.cfg.int_cfg.clk.click_time<<6)|(ops->data.cfg.int_cfg.clk.window));
                    
                    if(ops->data.cfg.pin == INTERRUPT_PIN1) {
                        msa_register_mask_write(handle, NSA_REG_INTERRUPT_MAPPING1, 0x30, 0x30);
                    }
                    else if(ops->data.cfg.pin == INTERRUPT_PIN2) {
                        msa_register_mask_write(handle, NSA_REG_INTERRUPT_MAPPING3, 0x30, 0x30);
                    }
                    break;
            }
            break;
            
        case INTERRUPT_OP_DISABLE:
            switch( ops->data.int_src )
            {
                case INTERRUPT_ACTIVITY:
                    /* Enable active interrupt */
                    msa_register_mask_write(handle, NSA_REG_INTERRUPT_SETTINGS1, 0x07, 0x00);
                    break;
                    
                case INTERRUPT_CLICK:
                    /* Enable single and double tap detect */
                    msa_register_mask_write(handle, NSA_REG_INTERRUPT_SETTINGS1, 0x30, 0x00);
                    break;
            }
            break;
            
        default:
            MI_ERR("Unsupport operation !");
    }
    return 0;
}

/********************************************
Function: msa_interrupt_ops
Description: MSA�жϲ�����־
Input: 
Output: 
Return: 
*************************************************/

int msa_interrupt_ops(MSA_HANDLE handle, msa_int_ops_t *ops)
{
    int res = 0;
    
    res = msa_gsensor_drv.obj[gsensor_mod].int_ops(handle, ops);
    return res;
}

#if FILTER_AVERAGE_ENHANCE
/********************************************
Function: msa_get_filter_param
Description: MSA��ȡ��������
Input: 
Output: 
Return: 
*************************************************/

int msa_get_filter_param(struct msa_filter_param_s* param){
    if (param == 0){
        MI_ERR("Invalid param!");
        return -1;
    }

    param->filter_param_h = core_ctx.tFac[0].filter_param_h;
    param->filter_param_l = core_ctx.tFac[0].filter_param_l;
    param->filter_threhold = core_ctx.tFac[0].filter_threhold;

    MI_MSG("FILTER param is get: filter_param_h = %d, filter_param_l = %d, filter_threhold = %d", param->filter_param_h, param->filter_param_l, param->filter_threhold);
    
    return 0;
}

/********************************************
Function: msa_get_filter_param
Description: MSA���ù�������
Input: 
Output: 
Return: 
*************************************************/

int msa_set_filter_param(struct msa_filter_param_s* param){

    if (param == 0){
        MI_ERR("Invalid param!");
        return -1;
    }

    MI_MSG("FILTER param is set: filter_param_h = %d, filter_param_l = %d, filter_threhold = %d", param->filter_param_h, param->filter_param_l, param->filter_threhold);

    core_ctx.tFac[1].filter_param_l = core_ctx.tFac[2].filter_param_l = core_ctx.tFac[0].filter_param_l = param->filter_param_l;
    core_ctx.tFac[1].filter_param_h =core_ctx.tFac[2].filter_param_h = core_ctx.tFac[0].filter_param_h  = param->filter_param_h;
    core_ctx.tFac[1].filter_threhold = core_ctx.tFac[2].filter_threhold =core_ctx.tFac[0].filter_threhold = param->filter_threhold;

    return 0;
}
#endif //#if FILTER_AVERAGE_ENHANCE

/********************************************
Function: msa_get_enable
Description: MSA��ȡENABLE��־
Input: 
Output: 
Return: 
*************************************************/

int msa_get_enable(MSA_HANDLE handle, char *enable)
{
    unsigned char              reg_data;    
    int             res = 0;
 
    res = msa_register_read(handle, msa_gsensor_drv.obj[gsensor_mod].power.addr, &reg_data);
    if(res != 0) {
        return res;
    }
    
     *enable = ( reg_data & msa_gsensor_drv.obj[gsensor_mod].power.mask ) ? 0 : 1;

    return res;
}

/********************************************
Function: msa_set_enable
Description: MSA����ENABLE��־
Input: 
Output: 
Return: 
*************************************************/
int msa_set_enable(MSA_HANDLE handle, char enable)
{
    int             res = 0;
    unsigned char              reg_data = 0x40;


    if(!enable) {
        reg_data = msa_gsensor_drv.obj[gsensor_mod].power.value;
    }
    
    res = msa_register_mask_write(handle, msa_gsensor_drv.obj[gsensor_mod].power.addr, msa_gsensor_drv.obj[gsensor_mod].power.mask, reg_data);

    return res;
}


/********************************************
Function: NSA_get_reg_data
Description: MSA��ȡ�Ĵ�������
Input: 
Output: 
Return: 
*************************************************/
static int NSA_get_reg_data(MSA_HANDLE handle, char *buf)
{
    int                 count = 0;
    int                 i;
    unsigned char       val;
        
    count += msa_gsensor_drv.method->mysprintf(buf+count, "---------start---------");
    for (i = 0; i <= 0xd2; i++){
        if(i%16 == 0)
            count += msa_gsensor_drv.method->mysprintf(buf+count, "\n%02x\t", i);
        msa_register_read(handle, i, &val); 
        count += msa_gsensor_drv.method->mysprintf(buf+count, "%02X ", val);
    }

    count += msa_gsensor_drv.method->mysprintf(buf+count, "\n--------end---------\n");    
    return count;
}

/********************************************
Function: msa_get_reg_data
Description: MSA��ȡ�Ĵ�������
Input: 
Output: 
Return: 
*************************************************/
int msa_get_reg_data(MSA_HANDLE handle, char *buf)
{     
    return msa_gsensor_drv.obj[gsensor_mod].get_reg_data(handle, buf);
}

/********************************************
Function: msa_set_odr
Description: MSA����ODRƵ��
Input: 
Output: 
Return: 
*************************************************/

int msa_set_odr(MSA_HANDLE handle, int delay)
{
    int     res = 0;
    int     odr = 0;

    if(delay <= 5)
    {
       odr = MSA_ODR_200HZ;
    }
    else if(delay <= 10)
    {
       odr = MSA_ODR_100HZ;
    }
    else
    {
       odr = MSA_ODR_50HZ;
    }       

    res = msa_register_mask_write(handle, msa_gsensor_drv.obj[gsensor_mod].odr_sect[odr].addr, 
            msa_gsensor_drv.obj[gsensor_mod].odr_sect[odr].mask,msa_gsensor_drv.obj[gsensor_mod].odr_sect[odr].value);
    if(res != 0) {
        return res;
    } 

    return res; 
}

/********************************************
Function: msa_soft_reset
Description: MSA�����λ
Input: 
Output: 
Return: 
*************************************************/
static int msa_soft_reset(MSA_HANDLE handle)
{
    int             res = 0;
    unsigned char   reg_data;

    reg_data = msa_gsensor_drv.obj[gsensor_mod].soft_reset.value;
    res = msa_register_mask_write(handle, msa_gsensor_drv.obj[gsensor_mod].soft_reset.addr, msa_gsensor_drv.obj[gsensor_mod].soft_reset.mask, reg_data);
    msa_gsensor_drv.method->msdelay(5);
    
    return res;
}

/********************************************
Function: msa_module_detect
Description: MSA�ͺż��
Input: 
Output: 
Return: 
*************************************************/
int msa_module_detect(PLAT_HANDLE handle)
{
    int             i, res = 0;
    unsigned char   cid, mid;
    int             is_find = -1;

    /* Probe gsensor module */  
    for(i=0;i<sizeof(msa_gsensor)/sizeof(msa_gsensor[0]);i++) {
        res = msa_register_read(handle, msa_gsensor[0].chip_id.addr, &cid);
        if(res != 0) 
        {
            return res;
        }
        
        cid &= msa_gsensor[0].chip_id.mask;
        if(msa_gsensor[0].chip_id.value == cid) 
        	{
            res = msa_register_read(handle, msa_gsensor[0].mod_id.addr, &mid);
            if(res != 0) 
            	{
                return res;
              }

            mid &= msa_gsensor[0].mod_id.mask;
            if( msa_gsensor[0].mod_id.value == mid )
            {
                MI_MSG("Found Gsensor MSA !");
                gsensor_mod = 0;
                is_find =0;
                break;
            }
           }
        }
        gsensor_mod = 0;
        is_find =0;
    return is_find;
}

/********************************************
Function: msa_parse_chip_info
Description: MSAоƬ����Ϣ����
Input: 
Output: 
Return: 
*************************************************/
int msa_parse_chip_info(PLAT_HANDLE handle){
    unsigned char i=0,tmp=0;   
    unsigned char reg_value = -1,reg_value1 = -1;
    char res=-1;

    if(-1 == gsensor_mod)
        return res; 

    res = msa_register_read(handle, NSA_REG_CHIP_INFO, &reg_value);
    if(res != 0) {
        return res;
    }

  gsensor_chip_info.reg_value    = reg_value;               

    if(!(reg_value&0xc0)){
        gsensor_chip_info.asic = ASIC_2511;
        gsensor_chip_info.mems= MEMS_T9;
        gsensor_chip_info.package= PACKAGE_NONE;
        
        for(i=0;i<sizeof(msa_chip_info_list)/sizeof(msa_chip_info_list[0]);i++){
                if(reg_value == msa_chip_info_list[i].reg_value){
                    gsensor_chip_info.package = msa_chip_info_list[i].package;
                    gsensor_chip_info.asic= msa_chip_info_list[i].asic;                  
                    gsensor_chip_info.mems= msa_chip_info_list[i].mems;              
                    break;
                }                   
        }
    }
    else{
        gsensor_chip_info.asic = ASIC_2512B;
        gsensor_chip_info.mems= MEMS_T9;
        gsensor_chip_info.package= PACKAGE_NONE;
        
        gsensor_chip_info.package = (package_type)((reg_value&0xc0)>>6);
            
        if((reg_value&0x38)>>3 == 0x01)
            gsensor_chip_info.asic =ASIC_2512B;

            res = msa_register_read(handle, NSA_REG_MEMS_OPTION, &reg_value);
            if(res != 0) {
                    return res;
            }

            res = msa_register_read(handle, NSA_REG_CHIP_INFO_SECOND, &reg_value1);
            if(res != 0) {
                return res;
            }			
#if YZ_CROSS_TALK_ENABLE 
        if(reg_value1&0x10)
         yzcross = -(reg_value1&0x0f);
        else
         yzcross = (reg_value1&0x0f); 
#endif 
            tmp= ((reg_value&0x01)<<2) |((reg_value1&0xc0)>>6); 

        if(tmp == 0x00){
           if(reg_value&0x80)
               gsensor_chip_info.mems =MEMS_TV03;
           else
               gsensor_chip_info.mems =MEMS_T9;
        }else if(tmp == 0x01){
               gsensor_chip_info.mems =MEMS_RTO3;
        }				
	}

    return 0;
}


/********************************************
Function: msa_install_general_ops
Description: MSA���س������
Input: 
Output: 
Return: 
*************************************************/
int msa_install_general_ops(struct general_op_s *ops)
{    
    if(0 == ops){
        return -1;
    }
    
    msa_gsensor_drv.method = ops;
    return 0;
}

/********************************************
Function: msa_core_init
Description: MSA�ں˳�ʼ��
Input: 
Output: 
Return: 
*************************************************/
MSA_HANDLE msa_core_init(PLAT_HANDLE handle)
{
    int             res = 0;

#if FILTER_AVERAGE_ENHANCE 
int i =0;
#endif

    msa_gsensor_drv.obj = msa_gsensor;

    if(gsensor_mod < 0){
        res = msa_module_detect(handle);
        if(res) {
         MI_ERR("Can't find msa gsensor!!"); 
            return 0;
        }

        /* No msaamems gsensor instance found */
        if(gsensor_mod < 0) {   
            return 0;
        }
    }

    MI_MSG("Probe gsensor module: %s", msa_gsensor[gsensor_mod].asic);

#if FILTER_AVERAGE_ENHANCE 
    /* configure default filter param */    
    for (i = 0; i < 3;i++){     
        core_ctx.tFac[i].filter_param_l = 2;
        core_ctx.tFac[i].filter_param_h = 8;
        core_ctx.tFac[i].filter_threhold = 60;

        core_ctx.tFac[i].refN_l = 0;
        core_ctx.tFac[i].refN_h = 0;
    }    
#endif

    res = msa_chip_resume(handle);
    if(res) {
                MI_ERR("chip resume fail!!\n");
                return 0;
    }

    return handle;
}


/********************************************
Function: msa_chip_resume
Description: MSA�ָ�
Input: 
Output: 
Return: 
*************************************************/
int msa_chip_resume(MSA_HANDLE handle)
{
    int     res = 0;
    unsigned char      reg_data;
    unsigned char      i = 0;
    
    res = msa_soft_reset(handle); 
    if(res) {
        MI_ERR("Do softreset failed !");
        return res;
    }
    
    for(i=0;i<MSA_INIT_SECT_LEN;i++) {
        if( msa_gsensor_drv.obj[gsensor_mod].init_sect[i].addr < 0 ) {
            break;
        }

        reg_data = msa_gsensor_drv.obj[gsensor_mod].init_sect[i].value;
        res = msa_register_mask_write(handle, msa_gsensor_drv.obj[gsensor_mod].init_sect[i].addr, msa_gsensor_drv.obj[gsensor_mod].init_sect[i].mask, reg_data);
        if(res != 0) {
            return res;
        } 
    }

    msa_gsensor_drv.method->msdelay(10);             

    if(gsensor_type<0){
        gsensor_type=msa_parse_chip_info(handle);
      
		if(gsensor_type<0){
			MI_ERR("Can't parse msa gsensor chipinfo!!");	
			return -1; 
		}
	}

    if(gsensor_chip_info.asic==ASIC_2512B){
        
        reg_data = msa_gsensor_drv.method->get_address(handle);


        if(reg_data ==0x26 ||reg_data ==0x4c){

            msa_register_mask_write(handle,NSA_REG_SENS_COMP,0xc0,0x00);
        }
    }

    
#if MSA_OFFSET_TEMP_SOLUTION
    res = msa_read_offset(handle, original_offset);
    if (res != 0){
        MI_ERR("Read offset failed !");
        return res;    
    }
    
    bLoad = FILE_CHECKING;
    readOffsetCnt = 0;    
    readsubfileCnt =0;
    manual_load_cali_file(handle);
#endif

    return res;
}


/********************************************
Function: msa_get_primary_offset
Description: MSA��ȡ��Ҫ��У׼����
Input: 
Output: 
Return: 
*************************************************/
int msa_get_primary_offset(MSA_HANDLE handle,int *x,int *y,int *z){
    int     res = 0;
    unsigned char      reg_data;
    unsigned char      i = 0;
    unsigned char      offset[9]={0};

    res = msa_read_offset(handle, offset);
    if (res != 0){
        MI_ERR("Read offset failed !");
        return -1;    
    }
    
    res = msa_soft_reset(handle); 
    if(res) {
        MI_ERR("Do softreset failed !");
        return -1;
    }
    
    for(i=0;i<MSA_INIT_SECT_LEN;i++) {
        if( msa_gsensor_drv.obj[gsensor_mod].init_sect[i].addr < 0 ) {
            break;
        }

        reg_data = msa_gsensor_drv.obj[gsensor_mod].init_sect[i].value;
        res = msa_register_mask_write(handle, msa_gsensor_drv.obj[gsensor_mod].init_sect[i].addr, msa_gsensor_drv.obj[gsensor_mod].init_sect[i].mask, reg_data);
        if(res != 0) {
            MI_ERR("Write register[0x%x] error!",msa_gsensor_drv.obj[gsensor_mod].init_sect[i].addr);
            goto EXIT;
        } 
    }

    msa_gsensor_drv.method->msdelay(100);        

    res = cycle_read_xyz(handle, x, y, z, 20);
    if (res){
            MI_ERR("i2c block read failed\n");
            goto EXIT;
    }

    msa_write_offset(handle, offset);

	if((gsensor_chip_info.reg_value == 0x4B)
		||(gsensor_chip_info.reg_value == 0x8C)
   		||(gsensor_chip_info.reg_value == 0xCA))
	{
		*z = 0;
	}
	
    return 0; 

EXIT:
    msa_write_offset(handle, offset);
    return -1;
}
