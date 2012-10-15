# Copyright (c) 2010 Wind River Systems, Inc.
#
# The right to copy, distribute, modify, or otherwise make use
# of this software may be licensed only pursuant to the terms
# of an applicable Wind River license agreement.

LOCAL_PATH:= $(call my-dir)

uboot_MAKE_CMD := make -C $(LOCAL_PATH) CROSS_COMPILE=$(ANDROID_BUILD_TOP)/$(TARGET_KERNEL_CROSSCOMPILE)

.PHONY: uboot.clean
uboot.clean:
	$(hide) echo "Cleaning u-boot.."
	$(hide) $(uboot_MAKE_CMD) clean

.PHONY: uboot.distclean
uboot.distclean:
	$(hide) echo "Distcleaning u-boot.."
	$(hide) $(uboot_MAKE_CMD) distclean

$(LOCAL_PATH)/include/config.h:
	$(hide) echo "Configuring u-boot with $(TARGET_UBOOT_CONFIG).."
	$(hide) $(uboot_MAKE_CMD) $(TARGET_UBOOT_CONFIG)

.PHONY: uboot.config
uboot.config: $(LOCAL_PATH)/include/config.h

$(LOCAL_PATH)/u-boot.bin: $(LOCAL_PATH)/include/config.h
	$(hide) echo "Building u-boot.."
	$(hide) $(uboot_MAKE_CMD)

$(LOCAL_PATH)/tools/mkimage: $(LOCAL_PATH)/u-boot.bin

.PHONY: uboot
uboot: $(LOCAL_PATH)/u-boot.bin

.PHONY: uboot.rebuild
uboot.rebuild: uboot.clean uboot

ifneq ($(strip $(BUILD_DEFAULT_MKBOOTIMG)),true)


include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional

LOCAL_PREBUILT_EXECUTABLES := tools/mkimage
include $(BUILD_HOST_PREBUILT)

installed_mkimage := $(LOCAL_INSTALLED_MODULE)

$(call dist-for-goals,droid,$(LOCAL_BUILT_MODULE))

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional

LOCAL_PREBUILT_EXECUTABLES := mkbootimg
include $(BUILD_HOST_PREBUILT)

$(LOCAL_INSTALLED_MODULE): $(installed_mkimage)

$(call dist-for-goals,droid,$(LOCAL_BUILT_MODULE))
endif
