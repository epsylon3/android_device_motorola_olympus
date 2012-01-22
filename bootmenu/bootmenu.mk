# Required tools and blobs for bootmenu
bm_device = device/motorola/olympus

PRODUCT_PACKAGES += \
    bootmenu \
    utility_lsof \
    static_busybox \
    static_logwrapper \
    hijack_boot_2nd-init \

# images
PRODUCT_COPY_FILES += \
    external/bootmenu/images/indeterminate1.png:system/bootmenu/images/indeterminate1.png \
    external/bootmenu/images/indeterminate2.png:system/bootmenu/images/indeterminate2.png \
    external/bootmenu/images/indeterminate3.png:system/bootmenu/images/indeterminate3.png \
    external/bootmenu/images/indeterminate4.png:system/bootmenu/images/indeterminate4.png \
    external/bootmenu/images/indeterminate5.png:system/bootmenu/images/indeterminate5.png \
    external/bootmenu/images/indeterminate6.png:system/bootmenu/images/indeterminate6.png \
    external/bootmenu/images/progress_empty.png:system/bootmenu/images/progress_empty.png \
    external/bootmenu/images/progress_fill.png:system/bootmenu/images/progress_fill.png \
    ${bm_device}/bootmenu/images/background.png:system/bootmenu/images/background.png \
    ${bm_device}/bootmenu/images/background.png:recovery/res/images/icon_clockwork.png \
    ${bm_device}/bootmenu/images/background.png:system/bootmenu/recovery/res/images/icon_clockwork.png

# bootmenu config
PRODUCT_COPY_FILES += \
    ${bm_device}/bootmenu/config/bootmenu_bypass:system/bootmenu/config/bootmenu_bypass \
    ${bm_device}/bootmenu/config/default.prop:system/bootmenu/config/default.prop \
    ${bm_device}/bootmenu/config/default_bootmode.conf:system/bootmenu/config/default_bootmode.conf \
    ${bm_device}/bootmenu/config/overclock.conf:system/bootmenu/config/overclock.conf \
    ${bm_device}/bootmenu/script/_config.sh:system/bootmenu/script/_config.sh \

# static tools
PRODUCT_COPY_FILES += \
    out/target/product/olympus/root/sbin/adbd:system/bootmenu/binary/adbd \
    out/target/product/olympus/utilities/busybox:system/bootmenu/binary/busybox \
    out/target/product/olympus/utilities/lsof:system/bootmenu/binary/lsof \

# recovery tools
PRODUCT_COPY_FILES += \
    ${bm_device}/bootmenu/script/recoveryexit.sh:recovery/root/sbin/recoveryexit.sh \
    ${bm_device}/config/busybox.fstab:recovery/root/etc/fstab \

# general config file to move elsewhere
PRODUCT_COPY_FILES += \
    ${bm_device}/config/profile:system/etc/profile \
    ${bm_device}/config/busybox.fstab:system/etc/fstab

# recovery
PRODUCT_COPY_FILES += \
    ${bm_device}/bootmenu/recovery/res/keys:system/bootmenu/recovery/res/keys \
    ${bm_device}/bootmenu/recovery/res/images/icon_error.png:system/bootmenu/recovery/res/images/icon_error.png \
    ${bm_device}/bootmenu/recovery/res/images/icon_done.png:system/bootmenu/recovery/res/images/icon_done.png \
    ${bm_device}/bootmenu/recovery/res/images/icon_installing.png:system/bootmenu/recovery/res/images/icon_installing.png \
    ${bm_device}/bootmenu/recovery/res/images/icon_firmware_error.png:system/bootmenu/recovery/res/images/icon_firmware_error.png \
    ${bm_device}/bootmenu/recovery/res/images/icon_firmware_install.png:system/bootmenu/recovery/res/images/icon_firmware_install.png \
    ${bm_device}/bootmenu/recovery/res/images/indeterminate1.png:system/bootmenu/recovery/res/images/indeterminate1.png \
    ${bm_device}/bootmenu/recovery/res/images/indeterminate2.png:system/bootmenu/recovery/res/images/indeterminate2.png \
    ${bm_device}/bootmenu/recovery/res/images/indeterminate3.png:system/bootmenu/recovery/res/images/indeterminate3.png \
    ${bm_device}/bootmenu/recovery/res/images/indeterminate4.png:system/bootmenu/recovery/res/images/indeterminate4.png \
    ${bm_device}/bootmenu/recovery/res/images/indeterminate5.png:system/bootmenu/recovery/res/images/indeterminate5.png \
    ${bm_device}/bootmenu/recovery/res/images/indeterminate6.png:system/bootmenu/recovery/res/images/indeterminate6.png \
    ${bm_device}/bootmenu/recovery/res/images/progress_empty.png:system/bootmenu/recovery/res/images/progress_empty.png \
    ${bm_device}/bootmenu/recovery/res/images/progress_fill.png:system/bootmenu/recovery/res/images/progress_fill.png \
    ${bm_device}/bootmenu/recovery/res/images/icon_clockwork.png:system/bootmenu/recovery/res/images/icon_clockwork.png \
    ${bm_device}/bootmenu/recovery/sbin/e2fsck:system/bootmenu/recovery/sbin/e2fsck \
    ${bm_device}/bootmenu/recovery/sbin/fix_permissions:system/bootmenu/recovery/sbin/fix_permissions \
    ${bm_device}/bootmenu/recovery/sbin/killrecovery.sh:system/bootmenu/recovery/sbin/killrecovery.sh \
    ${bm_device}/bootmenu/recovery/sbin/nandroid-md5.sh:system/bootmenu/recovery/sbin/nandroid-md5.sh \
    ${bm_device}/bootmenu/recovery/sbin/parted:system/bootmenu/recovery/sbin/parted \
    ${bm_device}/bootmenu/recovery/sbin/postrecoveryboot.sh:system/bootmenu/recovery/sbin/postrecoveryboot.sh \
    ${bm_device}/bootmenu/recovery/sbin/recovery:system/bootmenu/recovery/sbin/recovery_stable \
    ${bm_device}/bootmenu/recovery/sbin/resize2fs:system/bootmenu/recovery/sbin/resize2fs \
    ${bm_device}/bootmenu/recovery/sbin/sdparted:system/bootmenu/recovery/sbin/sdparted \
    ${bm_device}/bootmenu/recovery/sbin/mke2fs:system/bootmenu/recovery/sbin/mke2fs \
    ${bm_device}/bootmenu/recovery/sbin/mke2fs.bin:system/bootmenu/recovery/sbin/mke2fs.bin \
    ${bm_device}/bootmenu/recovery/recovery.fstab:system/bootmenu/recovery/recovery.fstab \
    ${bm_device}/bootmenu/images/background.png:system/bootmenu/recovery/res/images/icon_bootmenu.png \
