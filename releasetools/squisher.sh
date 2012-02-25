# This script is included in squisher
# It is the final build step (after OTA package)

DEVICE_OUT=$ANDROID_BUILD_TOP/out/target/product/olympus
DEVICE_TOP=$ANDROID_BUILD_TOP/device/motorola/olympus
VENDOR_TOP=$ANDROID_BUILD_TOP/vendor/motorola/olympus

echo "squisher.sh..."

# Delete unwanted apps
rm -f $REPACK/ota/system/app/FOTAKill.apk   #HTC stuff
rm -f $REPACK/ota/system/app/RomManager.apk #On market
rm -f $REPACK/ota/system/xbin/irssi         #Big (glibc) and not self compiled, not safe

# these scripts are not required
rm -f $REPACK/ota/system/etc/init.d/03firstboot
rm -f $REPACK/ota/system/etc/init.d/04modules

# remove dummy kernel module (created by kernel repo)
rm -f $REPACK/ota/system/lib/modules/dummy.ko

# add an empty script to prevent logcat errors (moto init.rc)
#touch $REPACK/ota/system/bin/mount_ext3.sh
#chmod +x $REPACK/ota/system/bin/mount_ext3.sh

# xterm definition for putty default settings
if [ ! -f $REPACK/ota/system/etc/terminfo/x/xterm ]; then
    mkdir -p $REPACK/ota/system/etc/terminfo/x
    cp $REPACK/ota/system/etc/terminfo/l/linux $REPACK/ota/system/etc/terminfo/x/xterm
fi

# prebuilt kernel & updater-script
cp -f $DEVICE_TOP/releasetools/updater-script $REPACK/ota/META-INF/com/google/android/updater-script
cat $DEVICE_TOP/releasetools/updater-kernel >> $REPACK/ota/META-INF/com/google/android/updater-script
cp $DEVICE_TOP/releasetools/custom_backup_kernel.txt $REPACK/ota/system/etc/custom_backup_list.txt
cp $DEVICE_OUT/boot.img $REPACK/ota/

# bootmenu tools
cp -R -f -p $DEVICE_TOP/bootmenu/* $REPACK/ota/system/bootmenu/

mkdir -p $REPACK/ota/system/bootmenu/2nd-init
cp $DEVICE_OUT/root/init.rc $REPACK/ota/system/bootmenu/2nd-init/
cp $DEVICE_OUT/root/init.olympus.rc $REPACK/ota/system/bootmenu/2nd-init/
cp $DEVICE_OUT/root/ueventd.rc $REPACK/ota/system/bootmenu/2nd-init/
cp $DEVICE_OUT/root/ueventd.olympus.rc $REPACK/ota/system/bootmenu/2nd-init/

mkdir -p $REPACK/ota/system/bootmenu/2nd-boot
cp $REPACK/ota/system/bootmenu/binary/2nd-init $REPACK/ota/system/bootmenu/binary/2nd-boot
cp $DEVICE_OUT/root/init $REPACK/ota/system/bootmenu/2nd-boot/

cp $DEVICE_OUT/recovery/root/sbin/recovery $REPACK/ota/system/bootmenu/recovery/sbin/

# use the static busybox in bootmenu, this helps a lot on incomplete roms and ics
cp $DEVICE_OUT/utilities/logwrapper $REPACK/ota/system/bootmenu/binary/logwrapper.bin
cp $REPACK/ota/system/bootmenu/binary/logwrapper.bin $REPACK/ota/system/bin/
