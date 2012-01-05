# This script is included in squisher
# It is the final build step (after OTA package)

DEVICE_OUT=$ANDROID_BUILD_TOP/out/target/product/olympus
DEVICE_TOP=$ANDROID_BUILD_TOP/device/motorola/olympus
VENDOR_TOP=$ANDROID_BUILD_TOP/vendor/motorola/olympus

# Delete unwanted apps
rm -f $REPACK/ota/system/app/RomManager.apk
rm -f $REPACK/ota/system/xbin/irssi

# these scripts are not required
rm $REPACK/ota/system/etc/init.d/03firstboot
rm $REPACK/ota/system/etc/init.d/04modules

# remove dummy kernel module (created by kernel repo)
rm $REPACK/ota/system/lib/modules/dummy.ko

# add an empty script to prevent logcat errors (moto init.rc)
touch $REPACK/ota/system/bin/mount_ext3.sh
chmod +x $REPACK/ota/system/bin/mount_ext3.sh

mkdir -p $REPACK/ota/system/etc/terminfo/x
cp $REPACK/ota/system/etc/terminfo/l/linux $REPACK/ota/system/etc/terminfo/x/xterm

# prebuilt boot, devtree, logo & updater-script
cp -f $DEVICE_TOP/releasetools/updater-script $REPACK/ota/META-INF/com/google/android/updater-script
if [ -n "$CYANOGEN_RELEASE" ]; then
  cat $DEVICE_TOP/releasetools/updater-kernel >> $REPACK/ota/META-INF/com/google/android/updater-script
  cp $DEVICE_TOP/releasetools/custom_backup_full.txt $REPACK/ota/system/etc/custom_backup_list.txt
else
  cp $DEVICE_TOP/releasetools/custom_backup_list.txt $REPACK/ota/system/etc/custom_backup_list.txt
  rm -f $REPACK/ota/boot.img
fi

mkdir -p $REPACK/ota/system/bootmenu/2nd-init
cp $DEVICE_OUT/root/init $REPACK/ota/system/bootmenu/2nd-init/init
cp $DEVICE_OUT/root/init.rc $REPACK/ota/system/bootmenu/2nd-init/init.rc
cp $DEVICE_OUT/root/init.olympus.rc $REPACK/ota/system/bootmenu/2nd-init/init.olympus.rc
cp $DEVICE_OUT/root/sbin/adbd $REPACK/ota/system/bin/bootmenu/binary/adbd

# use the static busybox in bootmenu, this helps a lot on incomplete roms and ics
cp $DEVICE_OUT/utilities/busybox $REPACK/ota/system/bootmenu/binary/busybox

