#!/system/bootmenu/binary/busybox ash

######## BootMenu Script
######## Execute [Normal] Boot

source /system/bootmenu/script/_config.sh

export PATH=/sbin:/system/xbin:/system/bin

######## Main Script

BB=/system/bootmenu/binary/busybox

$BB mount -o remount,rw /

######## Cleanup

## busybox applets cleanup, keep missing ones in system
for cmd in $($BB --list); do
    [ -L "/sbin/$cmd" ] && $BB rm "/sbin/$cmd"
done

$BB ln -s $BB /sbin/sh
$BB mv /sbin /.sbin

$BB mount -o rw -t $FS_OSH $PART_OSH /osh

if [ -d /osh/sbin ]; then
    $BB ln -s busybox /osh/sbin/sh
    $BB cp /system/bootmenu/binary/adbd /osh/sbin/adbd
    sync
    # Let moto system remount it after
    $BB umount /osh
fi

## reduce lcd backlight to save battery
echo 18 > /sys/class/leds/lcd-backlight/brightness

## restore second core
# echo 1 > /sys/devices/system/cpu/cpu1/online

