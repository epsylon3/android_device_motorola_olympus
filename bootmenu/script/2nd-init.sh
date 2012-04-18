#!/system/bootmenu/binary/busybox ash
######## BootMenu Script
######## Execute [2nd-init] Menu

source /system/bootmenu/script/_config.sh

BB="/system/bootmenu/binary/busybox"

######## Main Script

$BB mount -o remount,rw /

$BB rm -f /*.rc
$BB rm -f /*.sh
$BB rmdir /preinstall
$BB cp -f /system/bootmenu/2nd-init/* /
$BB rm /sbin/ueventd && ln -s /init /sbin/ueventd
$BB cp -f /system/bootmenu/binary/adbd /sbin/adbd

ADBD_RUNNING=`$BB ps | $BB grep adbd | $BB grep -v grep`
if [ -z "$ADB_RUNNING" ]; then
    $BB rm -f /sbin/adbd.root
    $BB rm -f /tmp/usbd_current_state
    #delete if is a symlink
    [ -L "/tmp" ] && rm -f /tmp
    $BB mkdir -p /tmp
else
    # well, not beautiful but do the work
    # to keep current usbd state
    if [ -L "/tmp" ]; then
        mv /tmp/usbd_current_state / 2>/dev/null
        $BB rm -f /tmp
        $BB mkdir -p /tmp
        mv /usbd_current_state /tmp/ 2>/dev/null
    fi
fi

$BB mkdir -p /sd-ext

## unmount devices
sync
umount /acct
umount /dev/cpuctl
umount /dev/pts
umount /mnt/asec
umount /mnt/obb
umount /cache
umount /data

######## Cleanup

rm -f /sbin/lsof

## busybox cleanup..
for cmd in $($BB --list); do
    [ -L "/sbin/$cmd" ] && $BB rm "/sbin/$cmd"
done

$BB rm -f /sbin/busybox

## used for adbd shell (can be bash also)
$BB ln -s $BB /sbin/sh

## reduce lcd backlight to save battery
echo 18 > /sys/class/leds/lcd-backlight/brightness

######## Let's go

# dual cores: disable second core to save power in bootmenu/recovery
echo 0 > /sys/devices/system/cpu/cpu1/online

/system/bootmenu/binary/2nd-init

