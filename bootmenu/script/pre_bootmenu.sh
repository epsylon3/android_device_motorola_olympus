#!/system/bootmenu/binary/busybox ash

######## BootMenu Script
######## Execute Pre BootMenu

source /system/bootmenu/script/_config.sh

######## Main Script

BB_STATIC="/system/bootmenu/binary/busybox"

BB="/sbin/busybox"

## reduce lcd backlight to save battery
echo 64 > /sys/class/leds/lcd-backlight/brightness

$BB_STATIC mount -o remount,rw /
$BB_STATIC mount -o remount,rw /system

# copy static busybox in ramfs
$BB_STATIC cp -f $BB_STATIC $BB

chmod 775 /sbin
chmod 775 $BB
$BB chown 0.0 $BB
$BB chmod 4755 $BB

if [ -L /sbin/bbconfig ]; then
    # job already done...
    exit 0
fi

$BB cp -f /system/bootmenu/binary/lsof /sbin/lsof
$BB cp -f /system/bootmenu/binary/reboot /sbin/reboot

# busybox symlink..
for cmd in $($BB_STATIC --list); do
    $BB ln -s busybox /sbin/$cmd
done

$BB_STATIC chmod +rx /sbin/*

# custom adbd (allow always root)
$BB cp -f /system/bootmenu/binary/adbd /sbin/adbd.root
$BB chown 0.0 /sbin/adbd.root
$BB chmod 4755 /sbin/adbd.root

$BB chmod 666 /dev/graphics/fb0

## missing system files
[ ! -c /dev/tty0 ]  && ln -s /dev/tty /dev/tty0

## /default.prop replace.. (TODO: check if that works)
$BB cp -f /system/bootmenu/config/default.prop /default.prop

## fstab for busybox mount
if [ ! -e /system/etc/fstab ]; then
    $BB cp -f /system/bootmenu/config/busybox.fstab /system/etc/fstab
fi

## mount cache for boot mode and recovery logs
$BB mkdir -p /cache

if [ ! -d /cache/bootmenu ]; then

    # stock mount, with fsck
    if [ -x /system/bin/mount_ext3.sh ]; then
       /system/bin/mount_ext3.sh cache /cache
    fi

    $BB mount -t $FS_CACHE -o nosuid,nodev,noatime,nodiratime,barrier=1 $PART_CACHE /cache

    $BB mkdir -p /cache/bootmenu
fi

# Atrix need that to enable the keypad
/system/bin/touchpad -a

# tegra : disable second core to save power in bootmenu/recovery
echo 0 > /sys/devices/system/cpu/cpu1/online

# load ondemand safe settings to reduce heat and battery use
#if [ -x /system/bootmenu/script/overclock.sh ]; then
#    /system/bootmenu/script/overclock.sh safe
#fi

exit 0
