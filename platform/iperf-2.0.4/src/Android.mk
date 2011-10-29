LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=\
	src/Extractor.c \
	src/Locale.c	\
	src/ReportCSV.c		\
	src/ReportDefault.c	\
	src/Reporter.c	\
	src/SocketAddr.c	\
	src/gnu_getopt.c	\
	src/gnu_getopt_long.c \
	src/service.c	\
	src/sockets.c	\
	src/stdio.c	\
	src/tcp_window_size.c\
	src/Client.cpp \
	src/Launch.cpp \
	src/List.cpp \
	src/Listener.cpp	\
	src/PerfSocket.cpp\
	src/Server.cpp \
	src/Settings.cpp \
	src/main.cpp



LOCAL_CFLAGS:=-O2 -g  -DHAVE_CONFIG_H


LOCAL_C_INCLUDES+=\
	external/iperf-2.0.4/include\


#LOCAL_SHARED_LIBRARIES+=libssl

#LOCAL_STATIC_LIBRARIES+=libpcap


LOCAL_MODULE_TAGS:=tests


LOCAL_MODULE:=iperf


include $(BUILD_EXECUTABLE)

