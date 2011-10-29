# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

L_DEFS := -DHAVE_CONFIG_H
L_CFLAGS := $(L_DEFS)



OBJS = compat/signal.c \
		compat/error.c \
		compat/delay.cpp \
		compat/gettimeofday.c \
		compat/inet_ntop.c \
		compat/inet_pton.c \
		compat/Thread.c \
		compat/snprintf.c \
		compat/string.c \
		src/Extractor.c \
		src/Locale.c \
		src/service.c \
		src/Launch.cpp \
		src/Client.cpp \
		src/List.cpp \
		src/Listener.cpp \
		src/PerfSocket.cpp \
		src/ReportCSV.c \
		src/ReportDefault.c \
		src/Reporter.c \
		src/Server.cpp \
		src/Settings.cpp \
		src/SocketAddr.c \
		src/gnu_getopt.c \
		src/gnu_getopt_long.c \
		src/sockets.c \
		src/stdio.c \
		src/tcp_window_size.c \
		src/main.cpp

iperf_SOURCES := $(OBJS)

include $(CLEAR_VARS)
LOCAL_MODULE := iperf
#LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
#LOCAL_MODULE_TAGS := tests eng
LOCAL_SHARED_LIBRARIES := libc
LOCAL_CFLAGS := $(L_CFLAGS)
LOCAL_C_INCLUDES += \
		$(LOCAL_PATH)/compat/ \
		$(LOCAL_PATH)/src/ \
		$(LOCAL_PATH)/include/ \
		$(LOCAL_PATH)/
LOCAL_SRC_FILES := $(iperf_SOURCES)
include $(BUILD_EXECUTABLE)
