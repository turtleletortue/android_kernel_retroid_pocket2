subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/accdet/$(MTK_PLATFORM)
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat/

obj-$(CONFIG_MTK_ACCDET) := accdet_drv.o
obj-$(CONFIG_MTK_ACCDET) += $(subst ",,$(CONFIG_MTK_PLATFORM))/
obj-$(CONFIG_MTK_ACCDET_POCKET1) := accdet_drv.o
obj-$(CONFIG_MTK_ACCDET_POCKET1) += $(subst ",,$(CONFIG_MTK_PLATFORM))/

