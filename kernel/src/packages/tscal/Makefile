# Makefile for Application Program

CROSS_COMPILE = /opt/toolchains/arm-eabi-4.2.1/bin/arm-eabi-
LDPATH = /opt/toolchains/arm-eabi-4.2.1/lib

CC	:= $(CROSS_COMPILE)gcc
CPP	:= $(CROSS_COMPILE)g++

UC_MACH_TYPE = $(shell echo '${BOARD_NAME}' | tr '[:lower:]' '[:upper:]') 

CFLAGS	:= -Wall \
		-I/opt/toolchains/arm-eabi-4.2.1/libc/include \
		-I/opt/toolchains/arm-eabi-4.2.1/libc/arch-arm/include \
		-I/opt/toolchains/arm-eabi-4.2.1/libc/kernel/common \
		-I/opt/toolchains/arm-eabi-4.2.1/libc/kernel/arch-arm \
		-I/opt/toolchains/arm-eabi-4.2.1/core/include\
		-fno-exceptions \
		-Wno-multichar \
		-msoft-float \
		-fpic \
		-ffunction-sections \
		-funwind-tables \
		-fstack-protector \
		-march=armv5te \
		-mtune=xscale  \
		-D__ARM_ARCH_5__ \
		-D__ARM_ARCH_5T__ \
		-D__ARM_ARCH_5TE__ \
		-mthumb-interwork \
		-DANDROID \
		-fmessage-length=0 \
		-W \
		-Wall \
		-Wno-unused \
		-DSK_RELEASE \
		-DNDEBUG \
		-O2 \
		-g \
		-Wstrict-aliasing=2 \
		-finline-functions \
		-fno-inline-functions-called-once \
		-fgcse-after-reload \
		-frerun-cse-after-loop \
		-frename-registers \
		-DNDEBUG -UDEBUG \
		-mthumb \
		-Os \
		-fomit-frame-pointer \
		-fno-strict-aliasing \
		-finline-limit=64  \
		-DCREATE_MOUNT_POINTS=0    \
		-DCONFIG_MACH_${UC_MACH_TYPE}    //\
//		-MD

SRCS	:= \
		ts_calibrate.c \
	    fbutils.c \
		font_8x8.c \
		font_8x16.c \
		ts_open.c \
		ts_attach.c \
		ts_read_raw.c


TARGETS	:= $(SRCS:.c=)
OBJS	:= $(SRCS:.c=.o)
PROG	:= tscal
SOBJS	:= $(SSRCS:.c=.o)
LIB		:= $(LDPATH)/liblog.a \
		$(LDPATH)/libcutils.a \
		$(LDPATH)/libc.a \
		$(LDPATH)/gcc/arm-eabi/4.2.1/interwork/libgcc.a

all	: depend $(OBJS) $(SLIB) $(PROG)

%.o	: %.c
	$(CC) $(CFLAGS) -c $<

$(PROG): $(OBJS)
	$(CPP) -nostdlib -Bstatic -Wl,-T,armelf.x \
	-Wl,--gc-sections \
	-o $@ \
	$(LDPATH)/crtbegin_static.o \
	-Wl,--no-undefined \
	$(OBJS) \
	$(LIB) \
	$(LDPATH)/crtend_android.o

clean : 
	rm -f $(PROG) $(TARGETS) .depend $(OBJS) $(SOBJS) $(SLIB) *~ *.d

depend : Makefile $(SRCS)
	$(CC) -M $(CFLAGS) $(SRCS) > .$@

install:
	cp -dpf $(PROG) $(ROOT)/sbin/$(PROG)
	/usr/local/arm/4.3.1-eabi-armv6/usr/bin/arm-linux-strip --strip-unneeded $(ROOT)/sbin/$(PROG)

-include .depend
