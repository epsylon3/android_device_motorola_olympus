#!/system/bootmenu/binary/busybox ash

######## BootMenu Script
######## Execute [Direct] Boot

source /system/bootmenu/script/_config.sh

export PATH=/system/xbin:/system/bin:$PATH

######## Main Script

BB=/system/bootmenu/binary/busybox

$BB mount -o remount,rw /

######## Cleanup

## busybox applets cleanup, keep missing ones in system
for cmd in $($BB --list); do
    [ -L "/sbin/$cmd" ] && $BB rm "/sbin/$cmd"
done

## let /sbin for osh mount
$BB mv /sbin /.sbin

$BB mount -o remount,ro /

## reduce lcd backlight to save battery
echo 18 > /sys/class/leds/lcd-backlight/brightness

