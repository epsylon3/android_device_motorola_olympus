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
    ${bm_device}/bootmenu/recovery/res/keys:system/bootmenu/recovery/res/keys \
    ${bm_device}/bootmenu/recovery/recovery.fstab:system/bootmenu/recovery/recovery.fstab \

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

