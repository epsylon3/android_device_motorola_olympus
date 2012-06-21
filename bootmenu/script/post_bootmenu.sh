#!/system/bin/sh

######## BootMenu Script
######## Execute Post BootMenu

source /system/bootmenu/script/_config.sh

export PATH=/system/xbin:/system/bin:/sbin

######## Main Script

# there is a problem, this script is executed if we 
# exit from recovery...

echo 0 > /sys/class/leds/blue/brightness

BB=/system/bootmenu/binary/busybox

######## Don't Delete.... ########################
$BB mount -o remount,rw /
$BB mount -o remount,rw $PART_SYSTEM /system
##################################################

## Double check /tmp exists
if [ ! -e /tmp ]; then
    mkdir /tmp
    $BB mount -t ramfs ramfs /tmp
    $BB chown system.shell /tmp
    $BB chmod 0777 /tmp
fi

## Run Init Script
if [ -d $BM_ROOTDIR/init.d ]; then
    chmod 755 $BM_ROOTDIR/init.d/*
    run-parts $BM_ROOTDIR/init.d/
fi

# normal cleanup here (need fix in recovery first)
# ...


# fast button warning (to check when script is really used)
if [ -f /sbin/busybox ]; then

echo 1 > /sys/class/leds/button-backlight/brightness
usleep 50000
echo 0 > /sys/class/leds/button-backlight/brightness
usleep 50000
echo 1 > /sys/class/leds/button-backlight/brightness
usleep 50000
echo 0 > /sys/class/leds/button-backlight/brightness
usleep 50000
echo 1 > /sys/class/leds/button-backlight/brightness
usleep 50000
echo 0 > /sys/class/leds/button-backlight/brightness

#source $BM_ROOTDIR/script/adbd.sh
setprop service.adb.root 1
setprop persist.service.adb.enable 1

exit 0

fi

######## Don't Delete.... ########################
mount -o remount,ro rootfs /
mount -o remount,ro $PART_SYSTEM /system
##################################################

exit 0
