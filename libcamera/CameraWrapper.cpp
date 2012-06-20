/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "CameraWrapper"
#define LOG_NDEBUG 0

#include <cmath>
#include <dlfcn.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <camera/Camera.h>
#include "CameraWrapper.h"

namespace android {

wp<CameraWrapper> CameraWrapper::singleton;

static bool
deviceCardMatches(const char *device, const char *matchCard)
{
    struct v4l2_capability caps;
    int fd = ::open(device, O_RDWR);
    bool ret;

    if (fd < 0) {
        return false;
    }

    if (::ioctl(fd, VIDIOC_QUERYCAP, &caps) < 0) {
        ret = false;
    } else {
        const char *card = (const char *) caps.card;

        LOGD("device %s card is %s\n", device, card);
        ret = strstr(card, matchCard) != NULL;
    }

    ::close(fd);

    return ret;
}

static sp<CameraHardwareInterface>
openMotoInterface(const char *libName, const char *funcName, int id)
{
    sp<CameraHardwareInterface> interface;
    void *libHandle = ::dlopen(libName, RTLD_NOW);

    if (libHandle != NULL) {
        typedef sp<CameraHardwareInterface> (*OpenCamFunc)(int);
        OpenCamFunc func = (OpenCamFunc) ::dlsym(libHandle, funcName);
        if (func != NULL) {
            interface = func(id);
        } else {
            LOGE("Could not find library entry point!");
        }
    } else {
        LOGE("dlopen() error: %s\n", dlerror());
    }

    return interface;
}

static void
setTorchMode(bool enable)
{
/* no led interface for flash on the atrix (useCameraInterface)

    int fd = ::open("/sys/class/leds/torch-flash/flash_light", O_WRONLY);
    if (fd >= 0) {
        const char *value = enable ? "100" : "0";
        write(fd, value, strlen(value));
        close(fd);
    }
*/
}

sp<CameraWrapper> CameraWrapper::createInstance(int cameraId)
{
    struct stat st;

    LOGV("%s :", __func__);
    if (singleton != NULL) {
        sp<CameraWrapper> hardware = singleton.promote();
        if (hardware != NULL) {
            return hardware;
        }
    }

    CameraType type = CAM_NONE;
    sp<CameraHardwareInterface> motoInterface;
    sp<CameraWrapper> hardware;

    if (stat("/system/lib/libnvmm_camera.so", &st) == 0) {
        LOGI("Using NvOmx bayer device");
        motoInterface = openMotoInterface("libcamera.so", "_ZN7android11NvOmxCamera14createInstanceEm", cameraId);
        type = CAM_NVIDIA_BAYER;
    }

    if (motoInterface != NULL) {
        hardware = new CameraWrapper(motoInterface, type, cameraId);
        singleton = hardware;
    } else {
        LOGE("Could not open hardware interface");
    }

    return hardware;
}

CameraWrapper::CameraWrapper(sp<CameraHardwareInterface>& motoInterface, CameraType type, int id) :
    mMotoInterface(motoInterface),
    mCameraType(type),
    mCameraId(id),
    mVideoMode(false),
    mNotifyCb(NULL),
    mDataCb(NULL),
    mDataCbTimestamp(NULL),
    mCbUserData(NULL)
{
    if (id == 0) {
        mTorchThread = new TorchEnableThread(this);
    }
}

CameraWrapper::~CameraWrapper()
{
    if (mCameraId == 0) {
        setTorchMode(false);
        mTorchThread->cancelAndWait();
        mTorchThread.clear();
    }
}

void
CameraWrapper::toggleTorchIfNeeded()
{
    if (mCameraId == 0) {
        setTorchMode(mFlashMode == CameraParameters::FLASH_MODE_TORCH);
    }
}

sp<IMemoryHeap>
CameraWrapper::getPreviewHeap() const
{
    return mMotoInterface->getPreviewHeap();
}

sp<IMemoryHeap>
CameraWrapper::getRawHeap() const
{
    return mMotoInterface->getRawHeap();
}

void
CameraWrapper::setCallbacks(notify_callback notify_cb,
                                  data_callback data_cb,
                                  data_callback_timestamp data_cb_timestamp,
                                  void* user)
{
    mNotifyCb = notify_cb;
    mDataCb = data_cb;
    mDataCbTimestamp = data_cb_timestamp;
    mCbUserData = user;

    if (mNotifyCb != NULL) {
        notify_cb = &CameraWrapper::notifyCb;
    }
    if (mDataCb != NULL) {
        data_cb = &CameraWrapper::dataCb;
    }
    if (mDataCbTimestamp != NULL) {
        data_cb_timestamp = &CameraWrapper::dataCbTimestamp;
    }

    mMotoInterface->setCallbacks(notify_cb, data_cb, data_cb_timestamp, this);
}

void
CameraWrapper::notifyCb(int32_t msgType, int32_t ext1, int32_t ext2, void* user)
{
    CameraWrapper *_this = (CameraWrapper *) user;
    user = _this->mCbUserData;

    if (msgType == CAMERA_MSG_FOCUS) {
        _this->toggleTorchIfNeeded();
    }
    _this->mNotifyCb(msgType, ext1, ext2, user);
}

void
CameraWrapper::dataCb(int32_t msgType, const sp<IMemory>& dataPtr, void* user)
{
    CameraWrapper *_this = (CameraWrapper *) user;
    user = _this->mCbUserData;

    if (msgType == CAMERA_MSG_COMPRESSED_IMAGE) {
        _this->fixUpBrokenGpsLatitudeRef(dataPtr);
    }

    _this->mDataCb(msgType, dataPtr, user);

    if (msgType == CAMERA_MSG_RAW_IMAGE || msgType == CAMERA_MSG_COMPRESSED_IMAGE) {
        if (_this->mTorchThread != NULL) {
            _this->mTorchThread->scheduleTorch();
        }
    }
 }

void
CameraWrapper::dataCbTimestamp(nsecs_t timestamp, int32_t msgType,
                                     const sp<IMemory>& dataPtr, void* user)
{
    CameraWrapper *_this = (CameraWrapper *) user;
    user = _this->mCbUserData;

    _this->mDataCbTimestamp(timestamp, msgType, dataPtr, user);
}

/*
 * Motorola's libcamera fails in writing the GPS latitude reference
 * tag properly. Instead of writing 'N' or 'S', it writes 'W' or 'E'.
 * Below is a very hackish workaround for that: We search for the GPS
 * latitude reference tag by pattern matching into the first couple of
 * data bytes. As the output format of Motorola's libcamera is static,
 * this should be fine until Motorola fixes their lib.
 */
void
CameraWrapper::fixUpBrokenGpsLatitudeRef(const sp<IMemory>& dataPtr)
{
    ssize_t offset;
    size_t size;
    sp<IMemoryHeap> heap = dataPtr->getMemory(&offset, &size);
    uint8_t *data = (uint8_t*)heap->base();

    if (data != NULL) {
        data += offset;

        /* scan first 512 bytes for GPS latitude ref marker */
        static const unsigned char sLatitudeRefMarker[] = {
            0x01, 0x00, /* GPS Latitude ref tag */
            0x02, 0x00, /* format: string */
            0x02, 0x00, 0x00, 0x00 /* 2 bytes long */
        };

        for (size_t i = 0; i < 512 && i < (size - 10); i++) {
            if (memcmp(data + i, sLatitudeRefMarker, sizeof(sLatitudeRefMarker)) == 0) {
                char *ref = (char *) (data + i + sizeof(sLatitudeRefMarker));
                if ((*ref == 'W' || *ref == 'E') && *(ref + 1) == '\0') {
                    LOGI("Found broken GPS latitude ref marker, offset %d, item %c",
                         i + sizeof(sLatitudeRefMarker), *ref);
                    *ref = (*ref == 'W') ? 'N' : 'S';
                }
                break;
            }
        }
    }
}

void
CameraWrapper::enableMsgType(int32_t msgType)
{
    mMotoInterface->enableMsgType(msgType);
}

void
CameraWrapper::disableMsgType(int32_t msgType)
{
    mMotoInterface->disableMsgType(msgType);
}

bool
CameraWrapper::msgTypeEnabled(int32_t msgType)
{
    return mMotoInterface->msgTypeEnabled(msgType);
}

status_t
CameraWrapper::startPreview()
{
    return mMotoInterface->startPreview();
}

bool
CameraWrapper::useOverlay()
{
    return mMotoInterface->useOverlay();
}

status_t
CameraWrapper::setOverlay(const sp<Overlay> &overlay)
{
    return mMotoInterface->setOverlay(overlay);
}

void
CameraWrapper::stopPreview()
{
    mMotoInterface->stopPreview();
}

bool
CameraWrapper::previewEnabled()

{
    return mMotoInterface->previewEnabled();
}

status_t
CameraWrapper::startRecording()
{
    toggleTorchIfNeeded();
    return mMotoInterface->startRecording();
}

void
CameraWrapper::stopRecording()
{
    toggleTorchIfNeeded();
    mMotoInterface->stopRecording();
}

bool
CameraWrapper::recordingEnabled()
{
    return mMotoInterface->recordingEnabled();
}

void
CameraWrapper::releaseRecordingFrame(const sp<IMemory>& mem)
{
    return mMotoInterface->releaseRecordingFrame(mem);
}

status_t
CameraWrapper::autoFocus()
{
    return mMotoInterface->autoFocus();
}

status_t
CameraWrapper::cancelAutoFocus()
{
    return mMotoInterface->cancelAutoFocus();
}

status_t
CameraWrapper::takePicture()
{
    return mMotoInterface->takePicture();
}

status_t
CameraWrapper::cancelPicture()
{
    return mMotoInterface->cancelPicture();
}

status_t
CameraWrapper::setParameters(const CameraParameters& params)
{
    CameraParameters pars(params.flatten());
    String8 oldFlashMode = mFlashMode;
    String8 sceneMode;
    status_t retval;
    int width, height;
    char buf[10];
    bool is1080p;

    /*
     * getInt returns -1 if the value isn't present and 0 on parse failure,
     * so if it's larger than 0, we can be sure the value was parsed properly
     */
    mVideoMode = pars.getInt("cam-mode") > 0;
    pars.remove("cam-mode");

    pars.getPreviewSize(&width, &height);
    is1080p = (width == 1920 && height == 1080);

    if (is1080p && !mVideoMode) {
        pars.setPreviewFrameRate(14);
    }
    if (mCameraId == 1 && mVideoMode) {
        // sample condition
        pars.setPreviewFrameRate(14);
    }

/*
    sceneMode = pars.get(CameraParameters::KEY_SCENE_MODE);
    if (sceneMode != CameraParameters::SCENE_MODE_AUTO) {
        // if the lib doesn't update the flash mode correctly when a scene mode is set, we need to do it here.
        pars.set(CameraParameters::KEY_FOCUS_MODE, CameraParameters::FOCUS_MODE_AUTO);

        if (sceneMode == CameraParameters::SCENE_MODE_PORTRAIT ||
            sceneMode == CameraParameters::SCENE_MODE_NIGHT_PORTRAIT)
        {
            pars.set(CameraParameters::KEY_FLASH_MODE, CameraParameters::FLASH_MODE_AUTO);
        } else {
            pars.set(CameraParameters::KEY_FLASH_MODE, CameraParameters::FLASH_MODE_OFF);
        }
    }
*/

    mFlashMode = pars.get(CameraParameters::KEY_FLASH_MODE);
    float exposure = pars.getFloat(CameraParameters::KEY_EXPOSURE_COMPENSATION);
    /* exposure-compensation comes multiplied in the -9...9 range, while
       we need it in the -3...3 range -> adjust for that */
    exposure /= 3;

    /* format the setting in a way the lib understands */
    bool even = (exposure - round(exposure)) < 0.05;
    snprintf(buf, sizeof(buf), even ? "%.0f" : "%.2f", exposure);
    pars.set("mot-exposure-offset", buf);

    /* kill off the original setting */
    pars.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, "0");

    retval = mMotoInterface->setParameters(pars);

    if (oldFlashMode != mFlashMode) {
        toggleTorchIfNeeded();
    }

    return retval;
}

CameraParameters
CameraWrapper::getParameters() const
{
    CameraParameters ret = mMotoInterface->getParameters();

    if (mCameraId == 1) {
        // to check...
        ret.set(CameraParameters::KEY_MAX_ZOOM, "3");
        ret.set(CameraParameters::KEY_ZOOM_RATIOS, "100,200,300,400");
    }

    /* cut down supported effects to values supported by framework */
    //ret.set(CameraParameters::KEY_SUPPORTED_EFFECTS, "none,mono,sepia,negative,solarize,red-tint,green-tint,blue-tint");
    ret.set(CameraParameters::KEY_SUPPORTED_EFFECTS, "mono,negative,none,posterize,sepia,solarize");

    /* Motorola uses mot-exposure-offset instead of exposure-compensation
       for whatever reason -> adapt the values.
       The limits used here are taken from the lib, we surely also
       could parse it, but it's likely not worth the hassle */
/*
    float exposure = ret.getFloat("mot-exposure-offset");
    int exposureParam = (int) round(exposure * 3);

    ret.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, exposureParam);
    ret.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, "9");
    ret.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, "-9");
    ret.set(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, "0.3333333333333");
*/

    ret.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT, CameraParameters::PIXEL_FORMAT_YUV420P);
//    ret.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE,
//            "(14000,30000),(14000,30000),(14000,30000),(14000,30000),(14000,30000),(14000,30000),(14000,30000),(14000,30000),(14000,30000)");
//    ret.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, "10000, 30000");

    ret.set("cam-mode", mVideoMode ? "1" : "0");

    return ret;
}

status_t
CameraWrapper::sendCommand(int32_t cmd, int32_t arg1, int32_t arg2)
{
    return mMotoInterface->sendCommand(cmd, arg1, arg2);
}

void
CameraWrapper::release()
{
    mMotoInterface->release();
}

status_t
CameraWrapper::dump(int fd, const Vector<String16>& args) const
{
    return mMotoInterface->dump(fd, args);
}

}; //namespace android
