/*
 * Simple synchronous userspace interface to SPI devices
 *
 * Copyright (C) 2006 SWAPP
 *  Andrea Paterniani <a.paterniani@swapp-eng.it>
 * Copyright (C) 2007 David Brownell (simplification, cleanup)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/spi/spi.h>

#include <asm/uaccess.h>
#include "sunwavecorp.h"
#include <linux/wakelock.h>
#include <linux/delay.h>

#define __devexit
#define __devinitdata
#define __devinit
#define __devexit_p

#ifdef _DRV_TAG_
#undef _DRV_TAG_
#endif
#define _DRV_TAG_ "corp"
//ruanbanmao add, 2016-04-21
#include <linux/platform_device.h>

/*
 * This supports access to SPI devices using normal userspace I/O calls.
 * Note that while traditional UNIX/POSIX I/O semantics are half duplex,
 * and often mask message boundaries, full SPI support requires full duplex
 * transfers.  There are several kinds of internal message boundaries to
 * handle chipselect management and other protocol options.
 *
 * SPI has a character major number assigned.  We allocate minor numbers
 * dynamically using a bitmask.  You must use hotplug tools, such as udev
 * (or mdev with busybox) to create and destroy the /dev/sunwave_devB.C device
 * nodes, since there is no fixed association of minor numbers with any
 * particular SPI bus or device.
 */



/* Bit masks for spi_device.mode management.  Note that incorrect
 * settings for some settings can cause *lots* of trouble for other
 * devices on a shared bus:
 *
 *  - CS_HIGH ... this device will be active when it shouldn't be
 *  - 3WIRE ... when active, it won't behave as it should
 *  - NO_CS ... there will be no explicit message boundaries; this
 *  is completely incompatible with the shared bus model
 *  - READY ... transfers may proceed when they shouldn't.
 *
 * REVISIT should changing those flags be privileged?
 */


//#define SUNWAVE_VERSION "V05"
#define SUNWAVE_DEVICE_VERSION_LEN 20

static LIST_HEAD(sunwave_device_list);
static DEFINE_MUTEX(sunwave_device_list_lock);
static sunwave_sensor_t* g_sunwave_sensor;

u8 suspend_flag = 0;
sunwave_sensor_t* get_current_sunwave(void)
{
    return g_sunwave_sensor;
}
EXPORT_SYMBOL_GPL(get_current_sunwave);

static unsigned bufsiz = 4096;
module_param(bufsiz, uint, S_IRUGO);
MODULE_PARM_DESC(bufsiz, "data bytes in biggest supported SPI message");


//ruanbanmao add for driver version detect, 2016-04-21, begin
#ifdef SUNWAVE_VERSION
struct sunwave_device_info {
    //char   vendor[SUNWAVE_DEVICE_VENDOR_LEN];             // device vendor:such as:sumsung
    char     version[SUNWAVE_DEVICE_VERSION_LEN];           // device version:such as:v1.01
};

struct sunwave_device_info dev_info[2] = {{SUNWAVE_VERSION}, {0}};
static ssize_t sunwave_device_version(struct device* dev, struct device_attribute* attr, char* buf_version)
{
    int len = 0;
    len += snprintf(buf_version + len, SUNWAVE_DEVICE_VERSION_LEN, "%s\n", dev_info[0].version);
    return len;
}

DEVICE_ATTR(version, S_IWUSR | S_IRUGO, sunwave_device_version, NULL);

static struct device_attribute* sunwave_device_attr_list[] = {
    &dev_attr_version,
};
#endif
//ruanbanmao add for driver version detect, 2016-04-21, end
/*-------------------------------------------------------------------------*/

/*
 * We can't use the standard synchronous wrappers for file I/O; we
 * need to protect against async removal of the underlying spi_device.
 */
static void sunwave_dev_complete(void* arg)
{
    complete(arg);
}

static ssize_t
sunwave_dev_sync(sunwave_sensor_t* sunwave_dev, struct spi_message* message)
{
    DECLARE_COMPLETION_ONSTACK(done);
    int status;
    message->complete = sunwave_dev_complete;
    message->context = &done;
    spin_lock_irq(&sunwave_dev->spi_lock);

    if (sunwave_dev->spi == NULL) {
        sw_err("spi device is NULL");
        status = -ESHUTDOWN;
    }
    else {
        status = spi_async(sunwave_dev->spi, message);
    }

    spin_unlock_irq(&sunwave_dev->spi_lock);

    if (status == 0) {
        wait_for_completion(&done);
        status = message->status;

        if (status == 0) {
            status = message->actual_length;
        }
        else {
            sw_err("spi async return error %d", status);
        }
    }
    else {
        sw_err("spi async error %d", status);
    }

    return status;
}

static inline ssize_t
sunwave_dev_sync_write(sunwave_sensor_t* sunwave_dev, size_t len)
{
    struct spi_transfer t = {
        .tx_buf     = sunwave_dev->buffer,
        .len        = len,
    };
    struct spi_message  m;
    spi_message_init(&m);
    spi_message_add_tail(&t, &m);
    return sunwave_dev_sync(sunwave_dev, &m);
}

static inline ssize_t
sunwave_dev_sync_read(sunwave_sensor_t* sunwave_dev, size_t len)
{
    struct spi_transfer t = {
        .tx_buf     = sunwave_dev->buffer,
        .rx_buf     = sunwave_dev->buffer,
        .len        = len,
    };
    struct spi_message  m;
    spi_message_init(&m);
    spi_message_add_tail(&t, &m);
    return sunwave_dev_sync(sunwave_dev, &m);
}
static inline ssize_t
sunwave_dev_wr(sunwave_sensor_t* sunwave_dev, u8* buf, u16 buflen)
{
    if (sunwave_dev->finger && sunwave_dev->finger->write_then_read == 0) {
        struct spi_transfer t = {
            .tx_buf     = buf,
            .rx_buf     = buf,
            .len        = buflen,
        };
        struct spi_message  m;
        spi_message_init(&m);
        spi_message_add_tail(&t, &m);
        return sunwave_dev_sync(sunwave_dev, &m);
    }

    //sunwave_dev->finger->write_then_read==1
    return spi_write_then_read(sunwave_dev->spi, buf, 8, buf + 8, buflen - 8);
}

/*-------------------------------------------------------------------------*/
/* Read-only message with current device setup */
static ssize_t
sunwave_dev_read(struct file* filp, char __user* buf, size_t count, loff_t* f_pos)
{
    sunwave_sensor_t*    sunwave_dev;
    ssize_t         status = 0;

    /* chipselect only toggles at start or end of operation */
    if (count > bufsiz) {
        return -EMSGSIZE;
    }

    sunwave_dev = filp->private_data;
    mutex_lock(&sunwave_dev->buf_lock);
    memset(sunwave_dev->buffer, 0, bufsiz);

    if (copy_from_user(sunwave_dev->buffer, buf, count)) {
        status = -EMSGSIZE;
        sw_err("cpoy from user");
        goto cpy_error;
    }

    if (sunwave_dev->finger && sunwave_dev->finger->write_then_read == 0) {
        status = sunwave_dev_sync_read(sunwave_dev, count);

        if (status > 0) {
            unsigned long   missing;
            missing = copy_to_user(buf, sunwave_dev->buffer, status);
            sw_dbg("ret %x", (buf[status - 2] << 8) | buf[status - 1]);

            if (missing == status) {
                status = -EFAULT;
				sw_dbg("ret = -EFAULT");
            }
            else {
                status = status - missing;
				sw_dbg("in else ret = %d\n", status);
            }
        }
    }
    else {
        status = spi_write_then_read(sunwave_dev->spi, sunwave_dev->buffer, 8, sunwave_dev->buffer + 8, count - 8);

        if (status > 0) {
            unsigned long   missing;
            missing = copy_to_user(buf + 8, sunwave_dev->buffer + 8, count - 8);

            if (missing == status) {
                status = -EFAULT;
            }
            else {
                status = status - missing;
            }
        }
    }

cpy_error:
    mutex_unlock(&sunwave_dev->buf_lock);
    return status;
}

/* Write-only message with current device setup */
static ssize_t
sunwave_dev_write(struct file* filp, const char __user* buf,
                  size_t count, loff_t* f_pos)
{
    sunwave_sensor_t*    sunwave_dev;
    ssize_t         status = 0;
    unsigned long       missing;

    /* chipselect only toggles at start or end of operation */
    if (count > bufsiz) {
        sw_err("write length lage than buff") ;
        return -EMSGSIZE;
    }

    sunwave_dev = filp->private_data;
    mutex_lock(&sunwave_dev->buf_lock);
    memset(sunwave_dev->buffer, 0, bufsiz);
    missing = copy_from_user(sunwave_dev->buffer, buf, count);

    if (missing == 0) {
        status = sunwave_dev_sync_write(sunwave_dev, count);
    }
    else {
        sw_err("copy from user") ;
        status = -EFAULT;
    }

    mutex_unlock(&sunwave_dev->buf_lock);
    return status;
}




static int sunwave_dev_message(sunwave_sensor_t* sunwave_dev,
                               struct spi_ioc_transfer* u_xfers, unsigned n_xfers)
{
    struct spi_message  msg;
    struct spi_transfer* k_xfers;
    struct spi_transfer* k_tmp;
    struct spi_ioc_transfer* u_tmp;
    unsigned        n, total;
    u8*          buf;
    int         status = -EFAULT;
    spi_message_init(&msg);
    k_xfers = kcalloc(n_xfers, sizeof(*k_tmp), GFP_KERNEL);

    if (k_xfers == NULL) {
        return -ENOMEM;
    }

    /* Construct spi_message, copying any tx data to bounce buffer.
     * We walk the array of user-provided transfers, using each one
     * to initialize a kernel version of the same transfer.
     */
    buf = sunwave_dev->buffer;
    total = 0;

    for (n = n_xfers, k_tmp = k_xfers, u_tmp = u_xfers;
         n;
         n--, k_tmp++, u_tmp++) {
        k_tmp->len = u_tmp->len;
        total += k_tmp->len;

        if (total > bufsiz) {
            status = -EMSGSIZE;
            goto done;
        }

        if (u_tmp->rx_buf) {
            k_tmp->rx_buf = buf;

            if (!access_ok(VERIFY_WRITE, (u8 __user*)
                           (uintptr_t) u_tmp->rx_buf,
                           u_tmp->len)) {
                goto done;
            }
        }

        if (u_tmp->tx_buf) {
            k_tmp->tx_buf = buf;

            if (copy_from_user(buf, (const u8 __user*)
                               (uintptr_t) u_tmp->tx_buf,
                               u_tmp->len)) {
                goto done;
            }
        }

        buf += k_tmp->len;
        k_tmp->cs_change = !!u_tmp->cs_change;
        k_tmp->bits_per_word = u_tmp->bits_per_word;
        k_tmp->delay_usecs = u_tmp->delay_usecs;
        k_tmp->speed_hz = u_tmp->speed_hz;
#ifdef VERBOSE
        dev_dbg(&sunwave_dev->spi->dev,
                "  xfer len %zd %s%s%s%dbits %u usec %uHz\n",
                u_tmp->len,
                u_tmp->rx_buf ? "rx " : "",
                u_tmp->tx_buf ? "tx " : "",
                u_tmp->cs_change ? "cs " : "",
                u_tmp->bits_per_word ? : sunwave_dev->spi->bits_per_word,
                u_tmp->delay_usecs,
                u_tmp->speed_hz ? : sunwave_dev->spi->max_speed_hz);
#endif
        spi_message_add_tail(k_tmp, &msg);
    }

    status = sunwave_dev_sync(sunwave_dev, &msg);

    if (status < 0) {
        goto done;
    }

    /* copy any rx data out of bounce buffer */
    buf = sunwave_dev->buffer;

    for (n = n_xfers, u_tmp = u_xfers; n; n--, u_tmp++) {
        if (u_tmp->rx_buf) {
            if (__copy_to_user((u8 __user*)
                               (uintptr_t) u_tmp->rx_buf, buf,
                               u_tmp->len)) {
                status = -EFAULT;
                goto done;
            }
        }

        buf += u_tmp->len;
    }

    status = total;
done:
    kfree(k_xfers);
    return status;
}

static long
sunwave_dev_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
{
    int         err = 0;
    int         retval = 0;
    sunwave_sensor_t*    sunwave_dev;
    struct spi_device*   spi;
    u32         tmp;
    unsigned        n_ioc;
    struct spi_ioc_transfer* ioc;

    /* Check type and command number */
    if (_IOC_TYPE(cmd) != SPI_IOC_MAGIC || _IOC_TYPE(cmd) !=  SUNWAVE_IOC_MAGIC) {
        return -ENOTTY;
    }

    /* Check access direction once here; don't repeat below.
     * IOC_DIR is from the user perspective, while access_ok is
     * from the kernel perspective; so they look reversed.
     */
    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok(VERIFY_WRITE,
                         (void __user*)arg, _IOC_SIZE(cmd));

    if (err == 0 && _IOC_DIR(cmd) & _IOC_WRITE)
        err = !access_ok(VERIFY_READ,
                         (void __user*)arg, _IOC_SIZE(cmd));

    if (err) {
        return -EFAULT;
    }

    /* guard against device removal before, or while,
     * we issue this ioctl.
     */
    sunwave_dev = filp->private_data;
    spin_lock_irq(&sunwave_dev->spi_lock);
    spi = spi_dev_get(sunwave_dev->spi);
    spin_unlock_irq(&sunwave_dev->spi_lock);

    if (spi == NULL) {
        return -ESHUTDOWN;
    }

    /* use the buffer lock here for triple duty:
     *  - prevent I/O (from us) so calling spi_setup() is safe;
     *  - prevent concurrent SPI_IOC_WR_* from morphing
     *    data fields while SPI_IOC_RD_* reads them;
     *  - SPI_IOC_MESSAGE needs the buffer locked "normally".
     */
    mutex_lock(&sunwave_dev->buf_lock);

    switch (cmd) {
        /* read requests */
        case SPI_IOC_RD_MODE:
            retval = __put_user(spi->mode & SPI_MODE_MASK,
                                (__u8 __user*)arg);
            break;

        case SPI_IOC_RD_LSB_FIRST:
            retval = __put_user((spi->mode & SPI_LSB_FIRST) ?  1 : 0,
                                (__u8 __user*)arg);
            break;

        case SPI_IOC_RD_BITS_PER_WORD:
            retval = __put_user(spi->bits_per_word, (__u8 __user*)arg);
            break;

        case SPI_IOC_RD_MAX_SPEED_HZ:
            retval = __put_user(spi->max_speed_hz, (__u32 __user*)arg);
            break;

        /* write requests */
        case SPI_IOC_WR_MODE:
            retval = __get_user(tmp, (u8 __user*)arg);

            if (retval == 0) {
                u8  save = spi->mode;

                if (tmp & ~SPI_MODE_MASK) {
                    retval = -EINVAL;
                    break;
                }

                tmp |= spi->mode & ~SPI_MODE_MASK;
                spi->mode = (u8)tmp;
                retval = spi_setup(spi);

                if (retval < 0) {
                    spi->mode = save;
                }
                else {
                    dev_dbg(&spi->dev, "spi mode %02x\n", tmp);
                }
            }

            break;

        case SPI_IOC_WR_LSB_FIRST:
            retval = __get_user(tmp, (__u8 __user*)arg);

            if (retval == 0) {
                u8  save = spi->mode;

                if (tmp) {
                    spi->mode |= SPI_LSB_FIRST;
                }
                else {
                    spi->mode &= ~SPI_LSB_FIRST;
                }

                retval = spi_setup(spi);

                if (retval < 0) {
                    spi->mode = save;
                }
                else
                    dev_dbg(&spi->dev, "%csb first\n",
                            tmp ? 'l' : 'm');
            }

            break;

        case SPI_IOC_WR_BITS_PER_WORD:
            retval = __get_user(tmp, (__u8 __user*)arg);

            if (retval == 0) {
                u8  save = spi->bits_per_word;
                spi->bits_per_word = tmp;
                retval = spi_setup(spi);

                if (retval < 0) {
                    spi->bits_per_word = save;
                }
                else {
                    dev_dbg(&spi->dev, "%d bits per word\n", tmp);
                }
            }

            break;

        case SPI_IOC_WR_MAX_SPEED_HZ:
            retval = __get_user(tmp, (__u32 __user*)arg);

            if (retval == 0) {
                u32 save = spi->max_speed_hz;
                spi->max_speed_hz = tmp;

                if (sunwave_dev->finger && sunwave_dev->finger->speed) {
                    sunwave_dev->finger->speed(&sunwave_dev->spi, tmp);
                }

                retval = spi_setup(spi);

                if (retval < 0) {
                    spi->max_speed_hz = save;
                }
                else {
                    dev_dbg(&spi->dev, "%d Hz (max)\n", tmp);
                }
            }

            break;

        case SUNWAVE_IOC_SET_SENSOR_STATUS:
            break;

        case SUNWAVE_IOC_GET_SENSOR_STATUS:
            break;

        case SUNWAVE_IOC_IS_FINGER_ON:
            break;

        case SUNWAVE_WRITE_READ_DATA: {
            struct sunwave_rw_operate  operate;

            if (sunwave_dev->buffer == NULL) {
                return -ENOMEM;
            }

            if (__copy_from_user(&operate, (void __user*)arg, sizeof(struct sunwave_rw_operate))) {
                return -EFAULT;
            }

            if (operate.len > bufsiz) {
                return -EFAULT;
            }

            if (__copy_from_user(sunwave_dev->buffer, (void __user*)operate.buf,  operate.len)) {
                return -EFAULT;
            }

            retval = sunwave_dev_wr(sunwave_dev, sunwave_dev->buffer, operate.len);

            if (retval < 0) {
                return retval;
            }

            retval = __copy_to_user((__u8 __user*)operate.buf,  sunwave_dev->buffer, operate.len);

            if (retval < 0) {
                dev_dbg(&spi->dev, "SUNWAVE_FP_READ_DATA: error copying to user\n");
            }
        }
        break;

        case SUNWAVE_IOC_RST_SENSOR:
            if (sunwave_dev->finger && sunwave_dev->finger->reset) {
                sunwave_dev->finger->reset(&sunwave_dev->spi);
            }

            break;

        case SUNWAVE_SET_INTERRUPT_WAKE_STATUS:
            break;

        case SUNWAVE_KEY_REPORT: {
            char key[3];

            if (__copy_from_user(key, (void __user*)arg, 2)) {
                return -EFAULT;
            }

            sunwave_key_report(sunwave_dev, key[1], key[0] ? 1 : 0);
        }
        break;

        case SUNWAVE_WAKEUP_SYSTEM:
            sunwave_wakeupSys(sunwave_dev);
            break;

        case SUNWAVE_IOC_ATTRIBUTE:
            retval = __put_user(sunwave_dev->finger->attribute, (__u32 __user*)arg);
            break;

        default:

            /* segmented and/or full-duplex I/O request */
            if (_IOC_NR(cmd) != _IOC_NR(SPI_IOC_MESSAGE(0))
                || _IOC_DIR(cmd) != _IOC_WRITE) {
                retval = -ENOTTY;
                break;
            }

            tmp = _IOC_SIZE(cmd);

            if ((tmp % sizeof(struct spi_ioc_transfer)) != 0) {
                retval = -EINVAL;
                break;
            }

            n_ioc = tmp / sizeof(struct spi_ioc_transfer);

            if (n_ioc == 0) {
                break;
            }

            /* copy into scratch area */
            ioc = kmalloc(tmp, GFP_KERNEL);

            if (!ioc) {
                retval = -ENOMEM;
                break;
            }

            if (__copy_from_user(ioc, (void __user*)arg, tmp)) {
                kfree(ioc);
                retval = -EFAULT;
                break;
            }

            /* translate to spi_message, execute */
            retval = sunwave_dev_message(sunwave_dev, ioc, n_ioc);
            kfree(ioc);
            break;
    }

    mutex_unlock(&sunwave_dev->buf_lock);
    spi_dev_put(spi);
    return retval;
}

#ifdef CONFIG_COMPAT
static long
sunwave_dev_compat_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
{
    return sunwave_dev_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#else
#define sunwave_dev_compat_ioctl NULL
#endif /* CONFIG_COMPAT */

static int sunwave_dev_open(struct inode* inode, struct file* filp)
{
    sunwave_sensor_t*    sunwave_dev;
    int         status = -ENXIO;
    mutex_lock(&sunwave_device_list_lock);
    list_for_each_entry(sunwave_dev, &sunwave_device_list, device_entry) {
        if (sunwave_dev->devt == inode->i_rdev) {
            status = 0;
            break;
        }
    }

    /* suport users: service and test */
    if (sunwave_dev->users > 1) {
        mutex_unlock(&sunwave_device_list_lock);
        pr_debug(KERN_ERR "Spi sensor: %s: Too many users\n", __func__);
        return -EPERM;
    }

    if (status == 0) {
        if (!sunwave_dev->buffer) {
            sunwave_dev->buffer = kmalloc(bufsiz, GFP_KERNEL);

            if (!sunwave_dev->buffer) {
                dev_dbg(&sunwave_dev->spi->dev, "open/ENOMEM\n");
                status = -ENOMEM;
            }
        }

        if (status == 0) {
            sunwave_dev->users++;
            filp->private_data = sunwave_dev;
            nonseekable_open(inode, filp);
        }
    }
    else {
        pr_debug("sunwave_dev: nothing for minor %d\n", iminor(inode));
    }

    mutex_unlock(&sunwave_device_list_lock);

    if (status == 0) {
        if (sunwave_dev->finger && sunwave_dev->finger->reset) {
            sunwave_dev->finger->reset(&sunwave_dev->spi);
        }
    }

    g_sunwave_sensor = sunwave_dev;
    return status;
}

static int sunwave_dev_release(struct inode* inode, struct file* filp)
{
    sunwave_sensor_t*    sunwave_dev;
    int         status = 0;
    mutex_lock(&sunwave_device_list_lock);
    sunwave_dev = filp->private_data;
    filp->private_data = NULL;
    /* last close? */
    sunwave_dev->users--;

    if (!sunwave_dev->users) {
        int     dofree;
        kfree(sunwave_dev->buffer);
        sunwave_dev->buffer = NULL;
        /* ... after we unbound from the underlying device? */
        spin_lock_irq(&sunwave_dev->spi_lock);
        dofree = (sunwave_dev->spi == NULL);
        spin_unlock_irq(&sunwave_dev->spi_lock);

        if (dofree) {
            kfree(sunwave_dev);
        }
    }

    mutex_unlock(&sunwave_device_list_lock);
    return status;
}

static const struct file_operations sunwave_dev_fops = {
    .owner =    THIS_MODULE,
    /* REVISIT switch to aio primitives, so that userspace
     * gets more complete API coverage.  It'll simplify things
     * too, except for the locking.
     */
    .write =    sunwave_dev_write,
    .read =     sunwave_dev_read,
    .unlocked_ioctl = sunwave_dev_ioctl,
    .compat_ioctl = sunwave_dev_compat_ioctl,
    .open =     sunwave_dev_open,
    .release =  sunwave_dev_release,
    .llseek =   no_llseek,
};

static struct miscdevice sunwave_misc_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "sunwave_fp",
    .fops = &sunwave_dev_fops,
};

//ruanbanmao add for driver version auto detect, 2016-04-21, begin.
#ifdef SUNWAVE_VERSION
static struct platform_device version_detect = {
    .name   = "sunwave_version_detect",
    .id     = -1,
};
#endif
//ruanbanmao add for driver version auto detect, 2016-04-21, end.

/*-------------------------------------------------------------------------*/

/* The main reason to have this class is to make mdev/udev create the
 * /dev/sunwave_devB.C character device nodes exposing our userspace API.
 * It also simplifies memory management.
 */

//static struct class* sunwave_dev_class;

/*-------------------------------------------------------------------------*/
static int sunwave_suspend(struct spi_device* spi, pm_message_t mesg)
{
    suspend_flag = 1;
    return 0;
}

static int sunwave_resume(struct spi_device* spi)
{
    return 0;
}

#if __SUNWAVE_DETECT_ID_EN
//add by Sea 20160505
static int sunwave_read_id(sunwave_sensor_t* sunwave_dev)
{
    ssize_t  status = 0;
    unsigned char wbuff[10] = {0};
    int err_cnt = 0;
    int ret = 0;
sunwave_read_id_start:
    msleep(150);
    sunwave_dev->buffer = kzalloc(10, GFP_KERNEL);
    sunwave_dev->buffer[0] = 0x1C;
    sunwave_dev->buffer[1] = 0x1C;
    sunwave_dev->buffer[2] = 0x1C;
    //wake up
    sunwave_dev_sync_write(sunwave_dev, 3);
    memset(sunwave_dev->buffer, 0, 10);
    wbuff[0] = 0x96;
    wbuff[1] = 0x69;
    wbuff[2] = 0x00;
    wbuff[3] = (0x1E >> 8) & 0xff; //ID_SENSOR_ID 0x08  ESD 0x1E
    wbuff[4] = 0x1E & 0xff;
    wbuff[5] = 0x00;
    wbuff[6] = 0x02;
    wbuff[7] = 0x00;
    memcpy(sunwave_dev->buffer, wbuff, 8);

    if (sunwave_dev->finger && sunwave_dev->finger->write_then_read == 0) {
        status = sunwave_dev_sync_read(sunwave_dev, 10);

        if (status <= 0) {
            ret = -1;
            goto sunwave_read_id_err;
        }
    }
    else {
        status = spi_write_then_read(sunwave_dev->spi, sunwave_dev->buffer, 8, sunwave_dev->buffer + 8, 2);

        if (status <= 0) {
            ret = -1;
            goto sunwave_read_id_err;
        }
    }

    int reg = ((sunwave_dev->buffer[8] << 8) | sunwave_dev->buffer[9]);
    sw_dbg("***read sunwave_fp id is %x***\n", reg);
    kfree(sunwave_dev->buffer);
    sunwave_dev->buffer = NULL;

    if (reg != 0xFAA0) {
        err_cnt++;

        if (err_cnt > 2) {
            ret = -2;
            goto sunwave_read_id_err;
        }
        else {
            goto sunwave_read_id_start;
        }
    }

    return 0;
sunwave_read_id_err:
    return -2;
}
#endif
//end

static int __devinit sunwave_dev_probe(struct spi_device* spi)
{
    sunwave_sensor_t*    sunwave_dev;
    int         status = 0;
    //struct device* dev;
    sw_dbg("probe start");
	sw_info("probe start");
    /* Allocate driver data */
    g_sunwave_sensor = NULL;
    sunwave_dev = kzalloc(sizeof(*sunwave_dev), GFP_KERNEL);

    if (!sunwave_dev) {
        sw_err("memory error");
        return -ENOMEM;
    }

    memset(sunwave_dev, 0, sizeof(sunwave_sensor_t));
    /* Initialize the driver data */
    sunwave_dev->spi = spi;
    sunwavecorp_register_finger(sunwave_dev);

    if (sunwave_dev->finger && sunwave_dev->finger->init) {
        status = sunwave_dev->finger->init(&sunwave_dev->spi);

        if (status < 0) {
            sw_err("init finger gpio %d", status) ;
            return status;
        }
		sw_info("init finger gpio %d", status);
    }

    create_input_device(sunwave_dev);
    spin_lock_init(&sunwave_dev->spi_lock);
    mutex_init(&sunwave_dev->buf_lock);
    INIT_LIST_HEAD(&sunwave_dev->device_entry);
    /* If we can allocate a minor number, hook up this device.
     * Reusing minors is fine so long as udev or mdev is working.
     */
    spi_set_drvdata(spi, sunwave_dev);
    //add by Sea 20160505
#if __SUNWAVE_DETECT_ID_EN
    status = sunwave_read_id(sunwave_dev);

    if (status != 0) {
        sw_err("read id error....remove register\n");
        list_del(&sunwave_dev->device_entry);
        release_input_device(sunwave_dev);
        spi_set_drvdata(spi, NULL);
        g_sunwave_sensor = NULL;
        kfree(sunwave_dev);
        sunwave_dev = NULL;
        return -ENODEV;
    }

#endif
    //end
    status = sunwave_irq_request(sunwave_dev);

    if (status < 0) {
        sw_err("request irq error!\n");
        return status;
    }

    //add by Sea
    device_init_wakeup(&sunwave_dev->spi->dev, 1);
    device_may_wakeup(&sunwave_dev->spi->dev);
    wake_lock_init(&sunwave_dev->wakelock, WAKE_LOCK_SUSPEND, dev_name(&sunwave_dev->spi->dev));
    status = misc_register(&sunwave_misc_dev);

    if (status < 0) {
        sw_err("misc_register error!\n");
        return status;
    }

    mutex_lock(&sunwave_device_list_lock);
    sunwave_dev->devt = MKDEV(MISC_MAJOR, sunwave_misc_dev.minor);
    list_add(&sunwave_dev->device_entry, &sunwave_device_list);
    mutex_unlock(&sunwave_device_list_lock);
    return status;
}

static int __devexit sunwave_dev_remove(struct spi_device* spi)
{
    sunwave_sensor_t*    sunwave_dev = spi_get_drvdata(spi);

    if (sunwave_dev->finger && sunwave_dev->finger->exit) {
        sunwave_dev->finger->exit(&sunwave_dev->spi);
    }

    sunwave_irq_free(sunwave_dev);
    release_input_device(sunwave_dev);
    g_sunwave_sensor = NULL;
    /* make sure ops on existing fds can abort cleanly */
    spin_lock_irq(&sunwave_dev->spi_lock);
    sunwave_dev->spi = NULL;
    spi_set_drvdata(spi, NULL);
    spin_unlock_irq(&sunwave_dev->spi_lock);
    //add by sea
    wake_lock_destroy(&sunwave_dev->wakelock);
    /* prevent new opens */
    mutex_lock(&sunwave_device_list_lock);
    list_del(&sunwave_dev->device_entry);
    //device_destroy(sunwave_dev_class, sunwave_dev->devt);

    if (sunwave_dev->users == 0) {
        kfree(sunwave_dev);
    }

    mutex_unlock(&sunwave_device_list_lock);
    return 0;
}

struct spi_device_id sunwave_id_table = {"sunwave_fp", 0};

#ifdef CONFIG_OF
static struct of_device_id sunwave_of_match[] = {
    { .compatible = "mediatek,sunwave-finger", },
    {}
};

MODULE_DEVICE_TABLE(of, sunwave_of_match);

#endif

static struct spi_driver sunwave_dev_spi_driver = {
    .driver = {
        .name =     "sunwave_fp",
        .owner =    THIS_MODULE,
#ifdef CONFIG_OF
        .of_match_table = sunwave_of_match,
#endif
    },
    .probe =    sunwave_dev_probe,
    .remove =   __devexit_p(sunwave_dev_remove),
    .suspend = sunwave_suspend,
    .resume = sunwave_resume,
    .id_table = &sunwave_id_table,

    /* NOTE:  suspend/resume methods are not necessary here.
     * We don't do anything except pass the requests to/from
     * the underlying controller.  The refrigerator handles
     * most issues; the controller driver handles the rest.
     */
};


/*-------------------------------------------------------------------------*/

static int __init sunwave_dev_init(void)
{
    int status;
    /* Claim our 256 reserved device numbers.  Then register a class
     * that will key udev/mdev to add/remove /dev nodes.  Last, register
     * the driver which manages those device numbers.
     */
    //dev_err(NULL,     "master %s: is invalid. \n",    dev_name ( &pdev->dev ) );
    /**     for add platform resources  if you need
     *      by Jone.Chen
     *   2015.15.25
     */
    //ruanbanmao add for driver auto detect, 2016-04-21, begin.
#ifdef SUNWAVE_VERSION
    platform_device_register(&version_detect);
    status = device_create_file(&(version_detect.dev), sunwave_device_attr_list[0]);

    if (status) {
        sw_err("sunwave_dri_detect_device failed.\n");
    }
    else {
        sw_info("sunwave_dri_detect_device success\n");
    }

#endif
    //ruanbanmao add for driver auto detect, 2016-04-21, end.
    sunwavecorp_register_platform();
    status = spi_register_driver(&sunwave_dev_spi_driver);
    sw_info("spi_register_driver %d\n", status);
    return status;
}
module_init(sunwave_dev_init);

static void __exit sunwave_dev_exit(void)
{
    spi_unregister_driver(&sunwave_dev_spi_driver);
    misc_deregister(&sunwave_misc_dev);
}
module_exit(sunwave_dev_exit);

MODULE_AUTHOR("Jone.Chen, <yuhua8688@tom.com>");
MODULE_DESCRIPTION("User mode SPI device interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:sunwave_fp");

