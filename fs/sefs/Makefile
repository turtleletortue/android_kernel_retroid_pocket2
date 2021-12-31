#
# Makefile for the Linux Sefs
#

obj-$(CONFIG_ECRYPT_FS) += sefs.o

sefs-y := dentry.o file.o inode.o main.o super.o mmap.o read_write.o \
	      crypto.o keystore.o kthread.o debug.o security.o aes.o crc32.o

sefs-$(CONFIG_ECRYPT_FS_MESSAGING) += messaging.o miscdev.o
