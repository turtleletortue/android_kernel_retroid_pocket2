/*
 * Definitions for qmc6983 magnetic sensor chip.
 */
	 
#ifndef __QMCX983_H__
#define __QMCX983_H__
	 
#include <linux/ioctl.h>  /* For IOCTL macros */

#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

#ifndef QMC_IOCTL_WRITE
/* IOCTLs for Msensor misc. device library */
#define MSENSOR						   0x83
/* IOCTLs for QMC library */
#define QMC_IOCTL_WRITE                 _IOW(MSENSOR, 0x40, char*)
#define QMC_IOCTL_READ                  _IOWR(MSENSOR, 0x41, char*)
#define QMC_IOCTL_SET_RANGE             _IOW(MSENSOR, 0x42, char*)
#define QMC_IOCTL_SET_MODE              _IOW(MSENSOR, 0x43, char*)
#define QMC_IOCTL_READ_SENSORDATA       _IOR(MSENSOR, 0x44, int)
#define QMC_IOCTL_SET_YPR               _IOW(MSENSOR, 0x45, short[28])
#define QMC_IOCTL_GET_OPEN_STATUS       _IOR(MSENSOR, 0x46, int)
#define QMC_IOCTL_GET_CLOSE_STATUS      _IOR(MSENSOR, 0x47, int)
#define QMC_IOC_GET_MFLAG               _IOR(MSENSOR, 0x48, int)
#define QMC_IOC_GET_OFLAG               _IOR(MSENSOR, 0x49, int)
#define QMC_IOCTL_GET_DELAY             _IOR(MSENSOR, 0x4a, short)
#endif

#ifdef CONFIG_COMPAT
#define COMPAT_QMC_IOCTL_WRITE                 _IOW(MSENSOR, 0x40, compat_uptr_t)
#define COMPAT_QMC_IOCTL_READ                  _IOWR(MSENSOR, 0x41, compat_uptr_t)
#define COMPAT_QMC_IOCTL_SET_RANGE             _IOW(MSENSOR, 0x42, compat_uptr_t)
#define COMPAT_QMC_IOCTL_SET_MODE              _IOW(MSENSOR, 0x43, compat_uptr_t)
#define COMPAT_QMC_IOCTL_READ_SENSORDATA       _IOR(MSENSOR, 0x44, compat_int_t)
#define COMPAT_QMC_IOCTL_SET_YPR               _IOW(MSENSOR, 0x45, compat_short_t[28])
#define COMPAT_QMC_IOCTL_GET_OPEN_STATUS       _IOR(MSENSOR, 0x46, compat_int_t)
#define COMPAT_QMC_IOCTL_GET_CLOSE_STATUS      _IOR(MSENSOR, 0x47, compat_int_t)
#define COMPAT_QMC_IOC_GET_MFLAG               _IOR(MSENSOR, 0x48, compat_int_t)
#define COMPAT_QMC_IOC_GET_OFLAG               _IOR(MSENSOR, 0x49, compat_int_t)
#define COMPAT_QMC_IOCTL_GET_DELAY             _IOR(MSENSOR, 0x4a, compat_short_t)
#endif

//extern struct mag_hw* qmcX983_get_cust_mag_hw(void);


/*-------------------------------------------------------------------*/
	 /* Magnetometer registers mapping */

#define QMCX983_SETRESET_FREQ_FAST  1
#define RWBUF_SIZE      16
/* Magnetometer registers */
#define CTL_REG_ONE	0x09  /* Contrl register one */
#define CTL_REG_TWO	0x0a  /* Contrl register two */

/* Output register start address*/
#define OUT_X_REG		0x00

/*Status registers */
#define STA_REG_ONE    0x06
#define STA_REG_TWO    0x0c

/*different from qmc6983,the ratio register*/
#define RATIO_REG		0x0b

 
/************************************************/
/* 	Magnetometer section defines	 	*/
/************************************************/

/* Magnetic Sensor Operating Mode */
#define QMCX983_STANDBY_MODE	0x00
#define QMCX983_CC_MODE		0x01
#define QMCX983_SELFTEST_MODE	0x02
#define QMCX983_RESERVE_MODE	0x03


/* Magnetometer output data rate  */
#define QMCX983_ODR_10		0x00	/* 0.75Hz output data rate */
#define QMCX983_ODR_50		0x01	/* 1.5Hz output data rate */
#define QMCX983_ODR_100		0x02	/* 3Hz output data rate */
#define QMCX983_ODR7_200	0x03	/* 7.5Hz output data rate */


/* Magnetometer full scale  */
#define QMCX983_RNG_2G		0x00
#define QMCX983_RNG_8G		0x01
#define QMCX983_RNG_12G		0x02
#define QMCX983_RNG_20G		0x03

#define RNG_2G		2
#define RNG_8G		8
#define RNG_12G		12
#define RNG_20G		20

/*data output register*/
#define OUT_X_M		0x01
#define OUT_X_L		0x00
#define OUT_Z_M		0x05
#define OUT_Z_L		0x04
#define OUT_Y_M		0x03
#define OUT_Y_L		0x02

#define SET_RATIO_REG   0x0b

/*data output rate HZ*/
#define DATA_OUTPUT_RATE_10HZ 	0x00
#define DATA_OUTPUT_RATE_50HZ 	0x01
#define DATA_OUTPUT_RATE_100HZ 	0x02
#define DATA_OUTPUT_RATE_200HZ 	0x03

/*oversample Ratio */
#define OVERSAMPLE_RATIO_512 	0x00
#define OVERSAMPLE_RATIO_256 	0x01
#define OVERSAMPLE_RATIO_128 	0x02
#define OVERSAMPLE_RATIO_64 	0x03


#define SAMPLE_AVERAGE_8		(0x3 << 5)
#define OUTPUT_RATE_75		(0x6 << 2)
#define MEASURE_NORMAL		0
#define MEASURE_SELFTEST		0x1
#define GAIN_DEFAULT		  (3 << 5)


// conversion of magnetic data (for bmm050) to uT units
// conversion of magnetic data to uT units
#define CONVERT_M			6
#define CONVERT_M_DIV		100			// 6/100 = CONVERT_M
#define CONVERT_O			1
#define CONVERT_O_DIV		100			// 1/64 = CONVERT_O
#define CONVERT_Q16			1
#define CONVERT_Q16_DIV		65536		// 1/64 = CONVERT_Gyro
	 
	 
#endif  /* __QMCX983_H__ */

