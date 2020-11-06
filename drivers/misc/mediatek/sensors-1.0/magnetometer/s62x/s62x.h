/*****************************************************************************
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 *****************************************************************************/

/* s62x.h - s62x compass include file
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef S62X_H
#define S62X_H

#include <linux/ioctl.h>

/* IOCTLs for Msensor misc. device library */
#define MSENSOR						   0x83
/* IOCTLs for AKM library */
#define ECS_IOCTL_WRITE                 _IOW(MSENSOR, 0x0b, char*)
#define ECS_IOCTL_READ                  _IOWR(MSENSOR, 0x0c, char*)
#define ECS_IOCTL_RESET	        _IO(MSENSOR, 0x0d)	/* NOT used in AK8975 */
#define ECS_IOCTL_SET_MODE              _IOW(MSENSOR, 0x0e, short)
#define ECS_IOCTL_GETDATA               _IOR(MSENSOR, 0x0f, char[SENSOR_DATA_SIZE])
#define ECS_IOCTL_SET_YPR               _IOW(MSENSOR, 0x10, short[12])
#define ECS_IOCTL_GET_OPEN_STATUS       _IOR(MSENSOR, 0x11, int)
#define ECS_IOCTL_GET_CLOSE_STATUS      _IOR(MSENSOR, 0x12, int)
#define ECS_IOCTL_GET_OSENSOR_STATUS	_IOR(MSENSOR, 0x13, int)
#define ECS_IOCTL_GET_DELAY             _IOR(MSENSOR, 0x14, short)
#define ECS_IOCTL_GET_PROJECT_NAME      _IOR(MSENSOR, 0x15, char[64])
#define ECS_IOCTL_GET_MATRIX            _IOR(MSENSOR, 0x16, short [4][3][3])
#define	ECS_IOCTL_GET_LAYOUT			_IOR(MSENSOR, 0x17, int[3])

#define ECS_IOCTL_GET_OUTBIT		_IOR(MSENSOR, 0x23, char)
#define ECS_IOCTL_GET_ACCEL		_IOR(MSENSOR, 0x24, short[3])

/* #define ECS_IOCTL_GET_INFO			_IOR(MSENSOR, 0x27, unsigned char[AKM_SENSOR_INFO_SIZE]) */
#define ECS_IOCTL_GET_CONF			_IOR(MSENSOR, 0x28, unsigned char[AKM_SENSOR_CONF_SIZE])
#define ECS_IOCTL_SET_YPR_09911               _IOW(MSENSOR, 0x29, int[26])
#define ECS_IOCTL_GET_DELAY_09911             _IOR(MSENSOR, 0x30, int64_t[3])
#define	ECS_IOCTL_GET_LAYOUT_09911			_IOR(MSENSOR, 0x31, char)

#endif
