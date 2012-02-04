#!/sbin/sh

source /system/bootmenu/script/_config.sh

if [ ! -d /cache/bootmenu ]; then

    # stock mount, with fsck
    if [ -x /system/bin/mount_ext3.sh ]; then
       /system/bin/mount_ext3.sh cache /cache
    fi

    mount -t $FS_CACHE -o nosuid,nodev,noatime,nodiratime,barrier=1 $PART_CACHE /cache

    mkdir -p /cache/bootmenu
fi

mv /cache/recovery/bootmode.conf /cache/recovery/last_bootmode

