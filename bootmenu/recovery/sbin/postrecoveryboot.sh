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
rm sdcard 2>/dev/null
mkdir sdcard

## ???

# busybox kill $(busybox ps | busybox grep adbd)
# killall adbd
# echo msc_adb > /dev/usb_device_mode
# kill $(ps | grep /sbin/adbd)
#killall adbd

echo msc_adb > /dev/usb_device_mode

sync

# upload firmware to touch screen
for i in $(seq 1 100); do
    if [ -b /dev/block/mmcblk0p12 ]; then
        break;
    fi
    usleep 50000
done

# stop warning about dvfs on dmesg
echo 0 > /proc/sys/kernel/hung_task_timeout_secs

# check for system sanity
e2fsck -n /dev/block/mmcblk0p12 > /dev/null 2>&1
SANE=$?

if [ "$SANE" = "0" ] ; then
    mount /dev/block/mmcblk0p12 /system
    cp -a /system/etc/touchpad /etc/
    /system/bin/touchpad -a
    umount /system
    rm /etc/touchpad -Rf
fi

