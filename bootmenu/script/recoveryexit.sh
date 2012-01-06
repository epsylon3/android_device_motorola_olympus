#!/sbin/sh

mount /system

if [ -f /system/bin/bootmenu ]; then

    echo bootmenu > /cache/recovery/bootmode.conf
    /system/bin/logwrapper /system/bin/bootmenu

    /system/bin/toolbox stop recovery
    exit
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
