#!/system/bootmenu/binary/busybox ash

######## BootMenu Script
######## Execute [ADB Daemon] Menu

source /system/bootmenu/script/_config.sh

######## Main Script

/system/bootmenu/binary/busybox mount -o remount,rw /
[ -L /tmp ] && rm /tmp
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
mkdir -p /cache/bootmenu
chown system.shell /cache/bootmenu
chmod 775 /cache/bootmenu
export HISTFILE=/cache/bootmenu/.ash_history
export HISTFILESIZE=256

# start adbd
export PATH=/sbin:/system/xbin:/system/bin

if [ -e /sbin/adbd.root ]; then
  /sbin/adbd.root recovery &
else
  /sbin/adbd recovery &
fi

# sample log reports
logcat -d > /cache/bootmenu/.adbd.log
dmesg > /cache/bootmenu/.dmesg.log
getprop > /cache/bootmenu/.getprop.log

exit
