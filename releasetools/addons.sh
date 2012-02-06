# This script is included in releasetools/addons
# It is the final build step (after OTA package)

DEVICE_OUT=$ANDROID_BUILD_TOP/out/target/product/olympus
DEVICE_TOP=$ANDROID_BUILD_TOP/device/motorola/olympus
VENDOR_TOP=$ANDROID_BUILD_TOP/vendor/motorola/olympus

echo "addons.sh: $1"

# remove dummy kernel module (created by kernel repo)
rm -f $REPACK/ota/system/lib/modules/dummy.ko

cat $DEVICE_TOP/releasetools/updater-addons > $REPACK/ota/META-INF/com/google/android/updater-script
cp $DEVICE_OUT/boot.img $REPACK/ota/
cp $DEVICE_OUT/recovery.img $REPACK/ota/


if [ "$1" = "recovery" ]; then
	cat $DEVICE_TOP/releasetools/updater-addons-recovery > $REPACK/ota/META-INF/com/google/android/updater-script
	rm -rf $REPACK/ota/system
	rm $REPACK/ota/boot.img
	OUTFILE=$OUT/recovery.zip
fi

