#!/bin/bash

###################################
## Set 6410 Environment Variable ##
###################################

#etinum.recovery
ZIMAGE_DIR=./src/kernel/arch/arm/boot
MKZIMAGE_DIR=./tools/mkzimage
ROOT_SKELETON_DIR=./delta/S3C6410/root_skeleton/$1

PRODUCT=vinsq
REV=00
OPTION=-kk

case "$PRODUCT" in
	
	vinsq)
		REALPRODUCT=$PRODUCT
		PRODUCT=SPH-M910
		USE_MKBOOTIMG=yes
		RFS_VERSION=1.3.1
		BASEBAND=cdma
		MODULES="dpram_recovery camera cmm g2d g3d jpeg mfc pp rotator btgpio vibetonz camera dpram bcm4329"
		PACKAGES="devmgr keypad tscal"
		KERNEL_DEF_CONFIG=m910_android_defconfig

	  	;;
	  	
	
	*)
		Usage
		;;
esac 


if [ ! $PWD_DIR ] ; then
	PWD_DIR=$(pwd)
fi

WORK_DIR=$PWD_DIR/src
DELTA_DIR=$PWD_DIR/delta
TOOLS_DIR=$PWD_DIR/tools
OUT_DIR=$PWD_DIR/OUTDIR

ANDROID_DIR=$WORK_DIR/open_src
ANDROID_OUT_DIR=$ANDROID_DIR/out/target/product/$PRODUCT
KERNEL_DIR=$WORK_DIR/kernel
MODULES_DIR=$WORK_DIR/modules
PACKAGES_DIR=$WORK_DIR/packages

ANDROID_FILE=$PWD_DIR/files/eclair-1020.tar.gz
KERNEL_FILE=$PWD_DIR/files/linux-2.6.29.tar.gz

BUILD_SCRIPT=$WORK_DIR/build_$PRODUCT"_"$REV.sh


create_initram_list()
{
	cd $KERNEL_DIR
	
	bash scripts/gen_initramfs_list.sh -u 0 -g 0 $ANDROID_OUT_DIR/root > .initram_list
	if [ $? != 0 ] ; then
		exit $?
	fi
}

build_zimage()
{
	echo *************************************
	echo *           build kernel            *
	echo *************************************

	cd $KERNEL_DIR

	rm -f usr/initramfs_data.cpio.gz

	cp arch/arm/configs/$KERNEL_DEF_CONFIG .config
	make oldconfig
	
	echo make -j$CPU_JOB_NUM
	make -j$CPU_JOB_NUM
	if [ $? != 0 ] ; then
		exit $?
	fi
}

build_kernel()
{
	echo *************************************
	echo *           build kernel            *
	echo *************************************

		build_zimage
		build_modules
		build_packages

		gen_ramdisk
		gen_kernel		

		# make final kernel tar file
		make_kernel_img
	
}

build_modules()
{
	echo *************************************
	echo *           build modules           *
	echo *************************************

	echo rm -rf $ANDROID_OUT_DIR/root/lib/modules
	rm -rf $ANDROID_OUT_DIR/root/lib/modules

	echo mkdir -p $ANDROID_OUT_DIR/root/lib/modules
	mkdir -p $ANDROID_OUT_DIR/root/lib/modules

	for module in $MODULES
	do
		echo cd $MODULES_DIR/$module
		cd $MODULES_DIR/$module
		make KDIR=$KERNEL_DIR PRJROOT=$WORK_DIR && \
		    cp *.ko $ANDROID_OUT_DIR/root/lib/modules/ && \
#		    cp *.ko $ANDROID_OUT_DIR/recovery/root/lib/modules/ && \
		    mkdir -p $OUT_DIR/symbols_$PRODUCT/ && \
		    cp -f *.ko $OUT_DIR/symbols_$PRODUCT//
		if [ "$?" -ne 0 ]; then
		    echo "*ERROR* while building $modules"
		    exit -1
		fi
	done

	/usr/local/arm/4.3.1-eabi-armv6/usr/bin/arm-linux-strip --strip-unneeded $ANDROID_OUT_DIR/root/lib/modules/*.ko
}

build_packages()
{
	echo *************************************
	echo *           build packages           *
	echo *************************************

	for package in $PACKAGES
	do
		echo cd $PACKAGES_DIR/$package
		cd $PACKAGES_DIR/$package
		make KDIR=$KERNEL_DIR && \
		    make ROOT=$ANDROID_OUT_DIR/root RECOVERY_ROOT=$ANDROID_OUT_DIR/recovery/root TARGETNAME=$REALPRODUCT install

		if [ "$?" -ne 0 ]; then
		    echo "*ERROR* while building $package"
		    exit -1
		fi
		
		mkdir -p $OUT_DIR/symbols_$PRODUCT/
		cp -f $package $OUT_DIR/symbols_$PRODUCT/
		    
	done

	# /usr/local/arm/4.3.1-eabi-armv6/usr/bin/arm-linux-strip --strip-unneeded $ANDROID_OUT_DIR/root/lib/packages/*.ko
}
make_kernel_img()
{
	#cd $KERNEL_DIR/arch/arm/boot
	cd $ANDROID_OUT_DIR
	
	cp boot.img zImage
	tar -cvf $OUT_DIR/$PRODUCT.zImage.tar zImage

	mkdir -p $OUT_DIR/symbols_$PRODUCT/
	cp $KERNEL_DIR/vmlinux $OUT_DIR/symbols_$PRODUCT/
          cp $KERNEL_DIR/System.map $OUT_DIR/symbols_$PRODUCT/
}
gen_ramdisk()
{
	cd $PWD_DIR
	# copy root skeleton to android output if it doesn't exist
	mkdir -p $ANDROID_OUT_DIR
	cp -r -u $DELTA_DIR/S3C6410/root_skeleton/$REALPRODUCT/* $ANDROID_OUT_DIR/

	echo "Generating ramdisk.img"
	rm $ANDROID_OUT_DIR/ramdisk.img
	$MKZIMAGE_DIR/mkbootfs $ANDROID_OUT_DIR/root | $MKZIMAGE_DIR/minigzip > $ANDROID_OUT_DIR/ramdisk.img
}

gen_kernel()
{
	cd $PWD_DIR
	$MKZIMAGE_DIR/mkbootimg --kernel $ZIMAGE_DIR/zImage --ramdisk $ANDROID_OUT_DIR/ramdisk.img --output $ANDROID_OUT_DIR/boot.img --base 0x50180000
	echo "Generated boot.img successfully"
	cp $ANDROID_OUT_DIR/boot.img $ANDROID_OUT_DIR/zImage
	
	cd $ANDROID_OUT_DIR
	tar cvf $OUT_DIR/$PRODUCT.zImage.tar zImage
	echo "Compressed zImage to $OUT_DIR"
}


case "$OPTION" in
	-kk)
			build_kernel 
		;;

esac 
echo ""
echo "Total spent time:"
times
	
exit 0

