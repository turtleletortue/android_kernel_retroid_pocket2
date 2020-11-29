#
# for USB OTG silicon based on Mentor Graphics INVENTRA designs
#
ccflags-$(CONFIG_USB_MTK_HDRC) += -I$(srctree)/drivers/misc/mediatek/usb20
#ccflags-y += -I$(srctree)/drivers/misc/mediatek/usb20
obj-$(CONFIG_USB_MTK_HDRC)	:= usb20.o usb20_phy.o
obj-$(CONFIG_USB_MTK_HDRC)  += usb20_host.o

#obj-y := usb20.o usb20_phy.o
#obj-y += usb20_host.o

obj-$(CONFIG_USBIF_COMPLIANCE)   += usb20_otg_if.o
#obj-y += usb20_otg_if.o