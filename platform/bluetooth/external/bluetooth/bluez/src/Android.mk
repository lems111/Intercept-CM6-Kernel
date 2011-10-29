LOCAL_PATH:= $(call my-dir)

# Relative path from current dir to vendor brcm
BRCM_BT_SRC_ROOT_PATH := ../../../../vendor/brcm

# Relative path from <mydroid> to brcm base
BRCM_BT_INC_ROOT_PATH := $(LOCAL_PATH)/../../../../vendor/brcm

#
# libbluetoothd
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	adapter.c \
	agent.c \
	dbus-common.c \
	dbus-hci.c \
	device.c \
	error.c \
	main.c \
	manager.c \
	plugin.c \
	rfkill.c \
	sdpd-request.c \
	sdpd-service.c \
	sdpd-server.c \
	sdpd-database.c \
	security.c \
	storage.c

LOCAL_CFLAGS:= \
	-DVERSION=\"4.47\" \
	-DSTORAGEDIR=\"/data/misc/bluetoothd\" \
	-DCONFIGDIR=\"/etc/bluez\" \
	-DSERVICEDIR=\"/system/bin\" \
	-DPLUGINDIR=\"/system/lib/bluez-plugin\" \
	-DANDROID_SET_AID_AND_CAP \
	-DANDROID_EXPAND_NAME

ifeq ($(BT_ALT_STACK),true)
LOCAL_CFLAGS += -DBT_ALT_STACK
endif

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/../common \
	$(LOCAL_PATH)/../gdbus \
	$(LOCAL_PATH)/../plugins \
	$(call include-path-for, glib) \
	$(call include-path-for, glib)/glib \
	$(call include-path-for, dbus) 

ifeq ($(BRCM_BT_USE_BTL_IF),true)
LOCAL_SRC_FILES += \
	$(BRCM_BT_SRC_ROOT_PATH)/adaptation/dtun/dtunc_bz4/dtun_clnt.c \
	$(BRCM_BT_SRC_ROOT_PATH)/adaptation/dtun/dtunc_bz4/dtun_device.c \
	$(BRCM_BT_SRC_ROOT_PATH)/adaptation/dtun/dtunc_bz4/dtun_obex.c \
	$(BRCM_BT_SRC_ROOT_PATH)/adaptation/dtun/dtunc_bz4/dtun_hcid.c

LOCAL_C_INCLUDES += \
	$(BRCM_BT_INC_ROOT_PATH)/adaptation/dtun/include \
	$(BRCM_BT_INC_ROOT_PATH)/adaptation/dtun/dtunc_bz4

#* thun.lim modified by thun.lim (ariesq & apollo were added)
LOCAL_CFLAGS += -DBRCM_BT_USE_BTL_IF -DBT_USE_BTL_IF
ifeq ($(TARGET_MODEL), Aries)
LOCAL_CFLAGS += -DSWP1_BLUETOOTH_ARIES
endif
ifeq ($(TARGET_MODEL), AriesQ)
LOCAL_CFLAGS += -DGA_BLUETOOTH_ARIES_Q
endif
ifeq ($(TARGET_MODEL), Apollo)
LOCAL_CFLAGS += -DGA_BLUETOOTH_APOLLO
endif
endif


LOCAL_SHARED_LIBRARIES := \
	libdl \
	libbluetooth \
	libdbus \
	libcutils

LOCAL_STATIC_LIBRARIES := \
	libglib_static \
	libbluez-common-static \
	libbuiltinplugin \
	libgdbus_static

LOCAL_MODULE:=libbluetoothd

include $(BUILD_SHARED_LIBRARY)

#
# bluetoothd
#

include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := \
	libbluetoothd

LOCAL_MODULE:=bluetoothd

include $(BUILD_EXECUTABLE)
