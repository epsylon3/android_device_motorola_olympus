#!/sbin/sh

sleep 5

for i in $(seq 1 10)
do
    TMP=$(mount | grep /tmp)
    if [ -z "$TMP" ]
    then
        break
    fi
    umount -l /tmp
    sleep 1
done

mount -o remount,rw rootfs /
rm -r /tmp
mkdir -p tmp
touch /tmp/recovery.log
rm sdcard
mkdir sdcard

## ???

# busybox kill $(busybox ps | busybox grep adbd)
# killall adbd
# echo msc_adb > /dev/usb_device_mode
# kill $(ps | grep /sbin/adbd)
#killall adbd

echo msc_adb > /dev/usb_device_mode

sync

# /sbin/adbd recovery &

mount /dev/block/mmcblk0p12 /system

# upload firmware to touch screen
for i in $(seq 1 100); do
    if [ -f /system/build.prop ]; then
        break;
    fi
    usleep 500000
done
cp -a /system/etc/touchpad /etc/
/system/bin/touchpad -a
umount /system
rm -Rf /etc/touchpad

