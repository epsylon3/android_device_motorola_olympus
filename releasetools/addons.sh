# This script is included in releasetools/addons
# It is the final build step (after OTA package)

# To remember :
# DEVICE_OUT=$ANDROID_BUILD_TOP/out/target/product/olympus
# DEVICE_TOP=$ANDROID_BUILD_TOP/device/motorola/olympus
# VENDOR_TOP=$ANDROID_BUILD_TOP/vendor/motorola/olympus

echo "addons.sh: $1"

if [ -z "$1" ]; then
    echo "addons: missing addon type"
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

