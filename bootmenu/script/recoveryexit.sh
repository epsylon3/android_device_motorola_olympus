#!/sbin/sh

mount /system

if [ -f /system/bin/bootmenu ]; then

    echo bootmenu > /cache/recovery/bootmode.conf
    /system/bin/logwrapper /system/bin/bootmenu

    /system/bin/toolbox stop recovery

    if [ -L /sbin/busybox ]; then
        rm /sbin/busybox
        # replace recovery symlink by full static busybox bootmenu version
        /system/bootmenu/binary/busybox cp /system/bootmenu/binary/busybox /sbin/busybox

        # keep recovery binary, for extra tools
        # rm /sbin/recovery
    fi
    exit 0
fi

# fast button warning
echo 1 > /sys/class/leds/red/brightness
usleep 50000
echo 0 > /sys/class/leds/red/brightness
usleep 50000
echo 1 > /sys/class/leds/red/brightness
usleep 50000
echo 0 > /sys/class/leds/red/brightness

exit 1
