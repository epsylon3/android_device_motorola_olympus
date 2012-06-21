ifeq ($(TARGET_BOOTLOADER_BOARD_NAME),olympus)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_PATH    := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE         := camera.olympus
LOCAL_SRC_FILES      := cameraHal.cpp CameraWrapper.cpp
LOCAL_PRELINK_MODULE := false

LOCAL_C_INCLUDES += $(ANDROID_BUILD_TOP)/frameworks/base/include

LOCAL_SHARED_LIBRARIES += \
    liblog \
    libutils \
    libbinder \
    libcutils \
    libmedia \
    libhardware \
    libcamera_client \
    libdl \
    libui \
    libstlport \

include external/stlport/libstlport.mk

ifeq ($(BOARD_CAMERA_CUSTOM_PARAMETERS),true)
    LOCAL_CFLAGS += -DUSE_CUSTOM_PARAMETERS
endif

include $(BUILD_SHARED_LIBRARY)

endif
