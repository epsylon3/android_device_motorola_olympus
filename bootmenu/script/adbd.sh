#!/sbin/sh

######## BootMenu Script
######## Execute [ADB Daemon] Menu

source /system/bootmenu/script/_config.sh

######## Main Script

mkdir -p /tmp
chown system.shell /tmp
chmod 0777 /tmp

# eth to disable MSC
sync
echo eth > /dev/usb_device_mode

stop adbd
sleep 1
busybox ifconfig lo up

echo charge_adb > /dev/usb_device_mode
echo usb_mode_charge_adb > /tmp/usbd_current_state

# busybox ash history
export HISTFILE=/cache/bootmenu/.ash_history
export HISTFILESIZE=1000

# start adbd
export PATH=/sbin:/system/xbin:/system/bin
/sbin/adbd.root recovery &

# sample log reports
# logcat -d > /cache/bootmenu/.adbd.log
# dmesg > /cache/bootmenu/.dmesg.log
# getprop > /cache/bootmenu/.getprop.log

exit
