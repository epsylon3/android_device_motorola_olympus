# This script is included in releasetools/addons
# It is the final build step (after OTA package)

# To remember :
# DEVICE_OUT=$ANDROID_BUILD_TOP/out/target/product/olympus
# DEVICE_TOP=$ANDROID_BUILD_TOP/device/motorola/olympus
# VENDOR_TOP=$ANDROID_BUILD_TOP/vendor/motorola/olympus

echo "addons.sh: $1"

if [ -z "$1" ]; then
	echo "addons.sh: error ! no target specified"
	exit 1
fi

# remove dummy kernel module (created by kernel repo)
rm -f $REPACK/ota/system/lib/modules/dummy.ko

if [ "$1" = "recovery" ]; then
	cat $DEVICE_TOP/releasetools/updater-addons-recovery > $REPACK/ota/META-INF/com/google/android/updater-script
	rm -rf $REPACK/ota/system
	cp $DEVICE_OUT/recovery.img $REPACK/ota/
	OUTFILE=$OUT/recovery.zip
fi

if [ "$1" = "kernel" ]; then
        cat $DEVICE_TOP/releasetools/updater-addons-kernel > $REPACK/ota/META-INF/com/google/android/updater-script
        cp $DEVICE_OUT/boot.img $REPACK/ota/
        OUTFILE=$OUT/kernel.zip
fi

if [ "$1" = "bootmenu" ]; then

	cd $REPACK/ota
	unzip -q $OTAPACKAGE "system/bootmenu/*" "system/bin/*"

	cat $DEVICE_TOP/releasetools/updater-addons-bootmenu > $REPACK/ota/META-INF/com/google/android/updater-script

	#atrix bootmenu require a custom kernel to allow the framebuffer
	cp $DEVICE_OUT/boot.img $REPACK/ota/
	cat $DEVICE_TOP/releasetools/updater-addons-kernel >> $REPACK/ota/META-INF/com/google/android/updater-script

	mv $REPACK/ota/system/bin/bootmenu $REPACK/ota/system/bootmenu/binary/bootmenu
	cp $DEVICE_TOP/bootmenu/binary/busybox $REPACK/ota/system/bootmenu/binary/
	rm -r -f $REPACK/ota/system/bin
	mkdir -p $REPACK/ota/system/bin
	mv $REPACK/ota/system/bootmenu/binary/bootmenu $REPACK/ota/system/bin/bootmenu
	# cp $DEVICE_OUT/system/bin/toolbox $REPACK/ota/system/bin/

	mkdir -p $REPACK/ota/system/bootmenu/2nd-init
	cp $DEVICE_OUT/root/init $REPACK/ota/system/bootmenu/2nd-init/
	cp $DEVICE_OUT/root/*.rc $REPACK/ota/system/bootmenu/2nd-init/
	rm -f $REPACK/ota/system/bootmenu/2nd-init/*.goldfish.rc

	cp $DEVICE_TOP/init $REPACK/ota/system/bootmenu/2nd-init/

	mkdir -p $REPACK/ota/system/bootmenu/2nd-boot
	cp $REPACK/ota/system/bootmenu/binary/2nd-init $REPACK/ota/system/bootmenu/binary/2nd-boot
	cp $DEVICE_OUT/root/init $REPACK/ota/system/bootmenu/2nd-boot/
	cp $DEVICE_OUT/root/*.rc $REPACK/ota/system/bootmenu/2nd-boot/
	rm -f $REPACK/ota/system/bootmenu/2nd-init/*.goldfish.rc

	OUTFILE=$OUT/bootmenu_stock.zip
fi

