LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := batsys

LOCAL_MODULE_PATH := $(PRODUCT_OUT)/scripts

LOCAL_SRC_FILES := battery_sysfsread.c

include $(BUILD_HOST_EXECUTABLE)
