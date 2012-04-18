#!/system/bootmenu/binary/busybox ash

######## BootMenu Script
######## Execute [Normal] Boot


export PATH=/sbin:/system/xbin:/system/bin

######## Main Script

BB=/system/bootmenu/binary/busybox

$BB mount -o remount,rw /
$BB cp -f /system/bootmenu/binary/adbd /sbin/adbd
$BB chmod 4755 /sbin/adbd
$BB chown root.system /sbin/adbd

######## Cleanup

## busybox applets cleanup, keep missing ones in system
for cmd in $($BB --list); do
    [ -L "/sbin/$cmd" ] && $BB rm "/sbin/$cmd"

    # optional, install all missing applets in xbin
    DELETE=0
    [ -e "/system/xbin/$cmd" ] && DELETE=1
    [ -e "/system/bin/$cmd" ] && DELETE=1
    [ "$DELETE" = "0" ] && $BB ln -s busybox "/system/xbin/$cmd"
done

## copy in xbin to have a busybox in $PATH
if [ ! -e /system/xbin/busybox ]; then
    $BB cp $BB /system/xbin/
fi

#$BB rm /sbin/busybox
$BB ln -s $BB /sbin/sh
$BB mv /sbin /.sbin

sync

$BB mount /dev/block/osh /osh

if [ -d /osh/sbin ]; then
    $BB ln -s $BB /osh/sbin/sh
    $BB chmod 4755 /system/bootmenu/binary/adbd
    $BB chown root.system /system/bootmenu/binary/adbd
    $BB ln -s /system/bootmenu/binary/adbd /osh/sbin/adbd
    $BB ln -s /init /osh/sbin/ueventd
    sync
    # Let moto system remount it after
    $BB umount /osh
fi

sync

## reduce lcd backlight to save battery
echo 18 > /sys/class/leds/lcd-backlight/brightness

## restore second core
echo 1 > /sys/devices/system/cpu/cpu1/online

