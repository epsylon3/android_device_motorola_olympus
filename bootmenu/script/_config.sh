######## BootMenu Script Env
######## common variables for scripts

export PATH=/sbin:/system/xbin:/system/bin

PART_PREINSTALL=/dev/block/mmcblk0p17
PART_SYSTEM=/dev/block/mmcblk0p12
PART_CACHE=/dev/block/mmcblk0p15
PART_DATA=/dev/block/mmcblk0p16
PART_OSH=/dev/block/mmcblk0p13
PART_PDS=/dev/block/mmcblk0p3

FS_PREINSTALL=ext3
FS_SYSTEM=ext3
FS_CACHE=ext3
FS_DATA=ext3
FS_OSH=ext3
FS_PDS=ext3

BM_ROOTDIR=/system/bootmenu

BOARD_UMS_LUNFILE=/sys/devices/platform/usb_mass_storage/lun0/file

