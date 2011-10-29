LOCAL_PATH := $(call my-dir)

#
# copy brcm binaries into android
#
ifeq ($(TARGET_MODEL), Apollo)
PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/bin/Apollo/btld:system/bin/btld \
	$(LOCAL_PATH)/bin/Apollo/BCM4329B1_002.002.023.0417.0420.hcd:system/bin/BCM4329B1_002.002.023.0417.0420.hcd
endif
ifeq ($(TARGET_MODEL), Aries)
PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/bin/Aries/btld:system/bin/btld \
	$(LOCAL_PATH)/bin/Aries/BCM4329B1_002.002.023.0417.0418.hcd:system/bin/BCM4329B1_002.002.023.0417.0418.hcd
endif
ifeq ($(TARGET_MODEL), AriesQ)
PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/bin/AriesQ/btld:system/bin/btld \
	$(LOCAL_PATH)/bin/AriesQ/BCM4329B1_002.002.023.0237.0270_26M.hcd:system/bin/BCM4329B1_002.002.023.0237.0270_26M.hcd
endif
ifeq ($(TARGET_MODEL), SPH-D700)
PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/bin/SPH-D700/btld:system/bin/btld \
	$(LOCAL_PATH)/bin/SPH-D700/BCM4329B1_002.002.023.0313.0331.hcd:system/bin/BCM4329B1_002.002.023.0313.0331.hcd
endif
ifeq ($(TARGET_MODEL), SPH-M910)
PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/bin/SPH-M910/btld:system/bin/btld \
	$(LOCAL_PATH)/bin/SPH-M910/BCM4329B1_002.002.023.0417.0421.hcd:system/bin/BCM4329B1_002.002.023.0417.0421.hcd
endif
ifeq ($(TARGET_MODEL), SWD-S500)
PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/bin/SPH-M910/btld:system/bin/btld \
	$(LOCAL_PATH)/bin/SPH-M910/BCM4329B1_002.002.023.0237.0270_26M.hcd:system/bin/BCM4329B1_002.002.023.0237.0270_26M.hcd
endif
ifeq ($(BOARD_HAVE_BLUETOOTH),true)
    include $(all-subdir-makefiles)
endif
