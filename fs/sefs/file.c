/**
 * Sefs: Linux filesystem encryption layer
 *
 * Copyright (C) 1997-2004 Erez Zadok
 * Copyright (C) 2001-2004 Stony Brook University
 * Copyright (C) 2004-2007 International Business Machines Corp.
 *   Author(s): Michael A. Halcrow <mhalcrow@us.ibm.com>
 *   		Michael C. Thompson <mcthomps@us.ibm.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <linux/file.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/mount.h>
#include <linux/pagemap.h>
#include <linux/security.h>
#include <linux/compat.h>
#include <linux/fs_stack.h>
#include <linux/aio.h>
#include "sefs_kernel.h"
#include "security.h"

/**
 * sefs_read_update_atime
 *
 * generic_file_read updates the atime of upper layer inode.  But, it
 * doesn't give us a chance to update the atime of the lower layer
 * inode.  This function is a wrapper to generic_file_read.  It
 * updates the atime of the lower level inode if generic_file_read
 * returns without any errors. This is to be used only for file reads.
 * The function to be used for directory reads is sefs_read.
 */
static ssize_t sefs_read_update_atime(struct kiocb *iocb,
				struct iov_iter *to)
{
	ssize_t rc;
	struct path *path;
	struct file *file = iocb->ki_filp;

	rc = generic_file_read_iter(iocb, to);
	/*
	 * Even though this is a async interface, we need to wait
	 * for IO to finish to update atime
	 */
	if (-EIOCBQUEUED == rc)
		rc = wait_on_sync_kiocb(iocb);
	if (rc >= 0) {
		path = sefs_dentry_to_lower_path(file->f_path.dentry);
		touch_atime(path);
	}
	return rc;
}

struct sefs_getdents_callback {
	struct dir_context ctx;
	struct dir_context *caller;
	struct super_block *sb;
	int filldir_called;
	int entries_written;
};

/* Inspired by generic filldir in fs/readdir.c */
static int
sefs_filldir(void *dirent, const char *lower_name, int lower_namelen,
		 loff_t offset, u64 ino, unsigned int d_type)
{
	struct sefs_getdents_callback *buf =
	    (struct sefs_getdents_callback *)dirent;
	size_t name_size;
	char *name;
	int rc;

	buf->filldir_called++;
	rc = sefs_decode_and_decrypt_filename(&name, &name_size,
						  buf->sb, lower_name,
						  lower_namelen);
	if (rc) {
		printk(KERN_ERR "%s: Error attempting to decode and decrypt "
		       "filename [%s]; rc = [%d]\n", __func__, lower_name,
		       rc);
		goto out;
	}
	buf->caller->pos = buf->ctx.pos;
	rc = !dir_emit(buf->caller, name, name_size, ino, d_type);
	kfree(name);
	if (!rc)
		buf->entries_written++;
out:
	return rc;
}

/**
 * sefs_readdir
 * @file: The Sefs directory file
 * @ctx: The actor to feed the entries to
 */
static int sefs_readdir(struct file *file, struct dir_context *ctx)
{
	int rc;
	struct file *lower_file;
	struct inode *inode = file_inode(file);
	struct sefs_getdents_callback buf = {
		.ctx.actor = sefs_filldir,
		.caller = ctx,
		.sb = inode->i_sb,
	};
	lower_file = sefs_file_to_lower(file);
	lower_file->f_pos = ctx->pos;
	rc = iterate_dir(lower_file, &buf.ctx);
	ctx->pos = buf.ctx.pos;
	if (rc < 0)
		goto out;
	if (buf.filldir_called && !buf.entries_written)
		goto out;
	if (rc >= 0)
		fsstack_copy_attr_atime(inode,
					file_inode(lower_file));
out:
	return rc;
}

struct kmem_cache *sefs_file_info_cache;

static int read_or_initialize_metadata(struct dentry *dentry)
{
	struct inode *inode = dentry->d_inode;
	struct sefs_mount_crypt_stat *mount_crypt_stat;
	struct sefs_crypt_stat *crypt_stat;
	int rc;

	crypt_stat = &sefs_inode_to_private(inode)->crypt_stat;
	mount_crypt_stat = &sefs_superblock_to_private(
						inode->i_sb)->mount_crypt_stat;
	mutex_lock(&crypt_stat->cs_mutex);

	if (crypt_stat->flags & SEFS_POLICY_APPLIED &&
	    crypt_stat->flags & SEFS_KEY_VALID) {
		rc = 0;
		goto out;
	}

	rc = sefs_read_metadata(dentry);
	if (!rc)
		goto out;

	if (mount_crypt_stat->flags & SEFS_PLAINTEXT_PASSTHROUGH_ENABLED) {
		crypt_stat->flags &= ~(SEFS_I_SIZE_INITIALIZED
				       | SEFS_ENCRYPTED);
		rc = 0;
		goto out;
	}

	if (!(mount_crypt_stat->flags & SEFS_XATTR_METADATA_ENABLED) &&
	    !i_size_read(sefs_inode_to_lower(inode))) {
		rc = sefs_initialize_file(dentry, inode);
		if (!rc)
			goto out;
	}

	rc = -EIO;
out:
	mutex_unlock(&crypt_stat->cs_mutex);
	return rc;
}

/**
 * sefs_open
 * @inode: inode speciying file to open
 * @file: Structure to return filled in
 *
 * Opens the file specified by inode.
 *
 * Returns zero on success; non-zero otherwise
 */
static int sefs_open(struct inode *inode, struct file *file)
{
	int rc = 0;
	struct sefs_crypt_stat *crypt_stat = NULL;
	struct dentry *sefs_dentry = file->f_path.dentry;
	/* Private value of sefs_dentry allocated in
	 * sefs_lookup() */
	struct sefs_file_info *file_info;

	if(efs_check(current->pid) == -1)
		return -ENOMEM;

	/* Released in sefs_release or end of function if failure */
	file_info = kmem_cache_zalloc(sefs_file_info_cache, GFP_KERNEL);
	sefs_set_file_private(file, file_info);
	if (!file_info) {
		sefs_printk(KERN_ERR,
				"Error attempting to allocate memory\n");
		rc = -ENOMEM;
		goto out;
	}
	crypt_stat = &sefs_inode_to_private(inode)->crypt_stat;
	mutex_lock(&crypt_stat->cs_mutex);
	if (!(crypt_stat->flags & SEFS_POLICY_APPLIED)) {
		sefs_printk(KERN_DEBUG, "Setting flags for stat...\n");
		/* Policy code enabled in future release */
		crypt_stat->flags |= (SEFS_POLICY_APPLIED
				      | SEFS_ENCRYPTED);
	}
	mutex_unlock(&crypt_stat->cs_mutex);
	rc = sefs_get_lower_file(sefs_dentry, inode);
	if (rc) {
		printk(KERN_ERR "%s: Error attempting to initialize "
			"the lower file for the dentry with name "
			"[%pd]; rc = [%d]\n", __func__,
			sefs_dentry, rc);
		goto out_free;
	}
	if ((sefs_inode_to_private(inode)->lower_file->f_flags & O_ACCMODE)
	    == O_RDONLY && (file->f_flags & O_ACCMODE) != O_RDONLY) {
		rc = -EPERM;
		printk(KERN_WARNING "%s: Lower file is RO; Sefs "
		       "file must hence be opened RO\n", __func__);
		goto out_put;
	}
	sefs_set_file_lower(
		file, sefs_inode_to_private(inode)->lower_file);
	if (S_ISDIR(sefs_dentry->d_inode->i_mode)) {
		sefs_printk(KERN_DEBUG, "This is a directory\n");
		mutex_lock(&crypt_stat->cs_mutex);
		crypt_stat->flags &= ~(SEFS_ENCRYPTED);
		mutex_unlock(&crypt_stat->cs_mutex);
		rc = 0;
		goto out;
	}
	rc = read_or_initialize_metadata(sefs_dentry);
	if (rc)
		goto out_put;
	sefs_printk(KERN_DEBUG, "inode w/ addr = [0x%p], i_ino = "
			"[0x%.16lx] size: [0x%.16llx]\n", inode, inode->i_ino,
			(unsigned long long)i_size_read(inode));
	goto out;
out_put:
	sefs_put_lower_file(inode);
out_free:
	kmem_cache_free(sefs_file_info_cache,
			sefs_file_to_private(file));
out:
	return rc;
}

static int sefs_flush(struct file *file, fl_owner_t td)
{
	struct file *lower_file = sefs_file_to_lower(file);

	if (lower_file->f_op->flush) {
		filemap_write_and_wait(file->f_mapping);
		return lower_file->f_op->flush(lower_file, td);
	}

	return 0;
}

static int sefs_release(struct inode *inode, struct file *file)
{
	sefs_put_lower_file(inode);
	kmem_cache_free(sefs_file_info_cache,
			sefs_file_to_private(file));
	return 0;
}

static int
sefs_fsync(struct file *file, loff_t start, loff_t end, int datasync)
{
	int rc;

	rc = filemap_write_and_wait(file->f_mapping);
	if (rc)
		return rc;

	return vfs_fsync(sefs_file_to_lower(file), datasync);
}

static int sefs_fasync(int fd, struct file *file, int flag)
{
	int rc = 0;
	struct file *lower_file = NULL;

	lower_file = sefs_file_to_lower(file);
	if (lower_file->f_op->fasync)
		rc = lower_file->f_op->fasync(fd, lower_file, flag);
	return rc;
}

static long
sefs_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct file *lower_file = sefs_file_to_lower(file);
	long rc = -ENOTTY;

	if (!lower_file->f_op->unlocked_ioctl)
		return rc;

	switch (cmd) {
	case FITRIM:
	case FS_IOC_GETFLAGS:
	case FS_IOC_SETFLAGS:
	case FS_IOC_GETVERSION:
	case FS_IOC_SETVERSION:
		rc = lower_file->f_op->unlocked_ioctl(lower_file, cmd, arg);
		fsstack_copy_attr_all(file_inode(file), file_inode(lower_file));

		return rc;
	default:
		return rc;
	}
}

#ifdef CONFIG_COMPAT
static long
sefs_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct file *lower_file = sefs_file_to_lower(file);
	long rc = -ENOIOCTLCMD;

	if (!lower_file->f_op->compat_ioctl)
		return rc;

	switch (cmd) {
	case FITRIM:
	case FS_IOC32_GETFLAGS:
	case FS_IOC32_SETFLAGS:
	case FS_IOC32_GETVERSION:
	case FS_IOC32_SETVERSION:
		rc = lower_file->f_op->compat_ioctl(lower_file, cmd, arg);
		fsstack_copy_attr_all(file_inode(file), file_inode(lower_file));

		return rc;
	default:
		return rc;
	}
}
#endif

const struct file_operations sefs_dir_fops = {
	.iterate = sefs_readdir,
	.read = generic_read_dir,
	.unlocked_ioctl = sefs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = sefs_compat_ioctl,
#endif
	.open = sefs_open,
	.flush = sefs_flush,
	.release = sefs_release,
	.fsync = sefs_fsync,
	.fasync = sefs_fasync,
	.splice_read = generic_file_splice_read,
	.llseek = default_llseek,
};

const struct file_operations sefs_main_fops = {
	.llseek = generic_file_llseek,
	.read = new_sync_read,
	.read_iter = sefs_read_update_atime,
	.write = new_sync_write,
	.write_iter = generic_file_write_iter,
	.iterate = sefs_readdir,
	.unlocked_ioctl = sefs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = sefs_compat_ioctl,
#endif
	.mmap = generic_file_mmap,
	.open = sefs_open,
	.flush = sefs_flush,
	.release = sefs_release,
	.fsync = sefs_fsync,
	.fasync = sefs_fasync,
	.splice_read = generic_file_splice_read,
};
