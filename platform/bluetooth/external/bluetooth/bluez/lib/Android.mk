LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	bluetooth.c \
	sdp.c \
	hci.c

LOCAL_C_INCLUDES+= \
	 $(LOCAL_PATH)/../include/

LOCAL_MODULE:=libbluetooth

LOCAL_CFLAGS+=-O3

ifeq ($(BT_ALT_STACK),true)
LOCAL_CFLAGS += -DBT_ALT_STACK
endif

include $(BUILD_SHARED_LIBRARY)
