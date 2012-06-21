/* vim:et:sts=4:sw=4
 *
 * Copyright (C) 2012, Android AOSP
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

#define LOG_TAG "CameraHAL"
#define LOG_NDEBUG 0
#define LOG_FULL_PARAMS
//#define LOG_EACH_FRAME

#include <hardware/camera.h>
#include <ui/Overlay.h>
#include <binder/IMemory.h>
#include <hardware/gralloc.h>
#include <utils/Errors.h>
#include <vector>

using namespace std;

#include "CameraWrapper.h"

namespace android {

struct legacy_camera_device {
    camera_device_t device;
    int id;

    // New world
    camera_notify_callback         notify_callback;
    camera_data_callback           data_callback;
    camera_data_timestamp_callback data_timestamp_callback;
    camera_request_memory          request_memory;
    void                          *user;
    preview_stream_ops            *window;

    // Old world
    sp<CameraWrapper>              hwif;
    gralloc_module_t const        *gralloc;
    camera_memory_t*               clientData;
    vector<camera_memory_t*>       sentFrames;
    sp<Overlay>                    overlay;

    int32_t                        previewWidth;
    int32_t                        previewHeight;
    Overlay::Format                previewFormat;
};

/** camera_hw_device implementation **/
static inline struct legacy_camera_device * to_lcdev(struct camera_device *dev)
{
    return reinterpret_cast<struct legacy_camera_device *>(dev);
}

//
// http://code.google.com/p/android/issues/detail?id=823#c4
//

static void Yuv420spToRgb565(char* rgb, char* yuv420sp, int width, int height, int stride)
{
    int frameSize = width * height;
    int padding = (stride - width) * 2; //two bytes per pixel for rgb565
    int colr = 0;
    for (int j = 0, yp = 0, k = 0; j < height; j++) {
        int uvp = frameSize + (j >> 1) * width, u = 0, v = 0;
        for (int i = 0; i < width; i++, yp++) {
            int y = (0xff & ((int) yuv420sp[yp])) - 16;
            if (y < 0) y = 0;
            if ((i & 1) == 0) {
                v = (0xff & yuv420sp[uvp++]) - 128;
                u = (0xff & yuv420sp[uvp++]) - 128;
            }

            int y1192 = 1192 * y;
            int r = (y1192 + 1634 * v);
            int g = (y1192 - 833 * v - 400 * u);
            int b = (y1192 + 2066 * u);

            r = std::max(0, std::min(r, 262143));
            g = std::max(0, std::min(g, 262143));
            b = std::max(0, std::min(b, 262143));

            rgb[k++] = ((g >> 7) & 0xe0) | ((b >> 13) & 0x1f);
            rgb[k++] = ((r >> 10) & 0xf8) | ((g >> 15) & 0x07);
        }
        k += padding;
    }
}

static void Yuv422iToRgb565(char* rgb, char* yuv422i, int width, int height, int stride)
{
    int yuvIndex = 0;
    int rgbIndex = 0;
    int padding = (stride - width) * 2; //two bytes per pixel for rgb565

    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width / 2; i++) {

            int y1 = (0xff & ((int) yuv422i[yuvIndex++])) - 16;
            if (y1 < 0) y1 = 0;

            int u = (0xff & yuv422i[yuvIndex++]) - 128;

            int y2 = (0xff & ((int) yuv422i[yuvIndex++])) - 16;
            if (y2 < 0) y2 = 0;

            int v = (0xff & yuv422i[yuvIndex++]) - 128;

            int yy1 = 1192 * y1;
            int yy2 = 1192 * y2;
            int uv = 833 * v + 400 * u;
            int uu = 2066 * u;
            int vv = 1634 * v;

            int r = yy1 + vv;
            int g = yy1 - uv;
            int b = yy1 + uu;

            r = std::max(0, std::min(r, 262143));
            g = std::max(0, std::min(g, 262143));
            b = std::max(0, std::min(b, 262143));

            rgb[rgbIndex++] = ((g >> 7) & 0xe0) | ((b >> 13) & 0x1f);
            rgb[rgbIndex++] = ((r >> 10) & 0xf8) | ((g >> 15) & 0x07);

            r = yy2 + vv;
            g = yy2 - uv;
            b = yy2 + uu;

            r = std::max(0, std::min(r, 262143));
            g = std::max(0, std::min(g, 262143));
            b = std::max(0, std::min(b, 262143));

            rgb[rgbIndex++] = ((g >> 7) & 0xe0) | ((b >> 13) & 0x1f);
            rgb[rgbIndex++] = ((r >> 10) & 0xf8) | ((g >> 15) & 0x07);
        }
        rgbIndex += padding;
    }
}

static void processPreviewData(char *frame, size_t size, legacy_camera_device *lcdev, Overlay::Format format)
{
#ifdef LOG_EACH_FRAME
    LOGV("%s: frame=%p, size=%d, lcdev=%p", __FUNCTION__, frame, size, lcdev);
#endif
    if (lcdev->window == NULL) {
        LOGV("%s: window is null !", __FUNCTION__);
        return;
    }

    int32_t stride;
    buffer_handle_t *bufHandle = NULL;
    int ret = lcdev->window->dequeue_buffer(lcdev->window, &bufHandle, &stride);
    if (ret != NO_ERROR) {
        LOGW("%s: ERROR dequeueing the buffer", __FUNCTION__);

        //Note Tegra camera libs never use Overlay::enqueueBuffer
        ret = lcdev->window->enqueue_buffer(lcdev->window, bufHandle);
        if (ret != NO_ERROR) {
           LOGE("%s: ERROR enqueueing a buffer", __FUNCTION__);
           return;
        }

        ret = lcdev->window->dequeue_buffer(lcdev->window, &bufHandle, &stride);
        if (ret != NO_ERROR) {
            LOGE("%s: ERROR dequeueing the buffer", __FUNCTION__);
            return;
        }
    }

    if (stride != lcdev->previewWidth) {
        LOGE("%s: stride=%d doesn't equal width=%d", __FUNCTION__, stride, lcdev->previewWidth);
    }

    ret = lcdev->window->lock_buffer(lcdev->window, bufHandle);
    if (ret != NO_ERROR) {
        LOGE("%s: ERROR locking the buffer", __FUNCTION__);
        lcdev->window->cancel_buffer(lcdev->window, bufHandle);
        return;
    }

    int tries = 5;
    void *vaddr;

    do {
        ret = lcdev->gralloc->lock(lcdev->gralloc, *bufHandle, GRALLOC_USAGE_SW_WRITE_MASK,
//                GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER,
                0, 0, lcdev->previewWidth, lcdev->previewHeight, &vaddr);
        tries--;
        if (ret) {
            lcdev->gralloc->unlock(lcdev->gralloc, *bufHandle);
            LOGW("%s: gralloc lock retry", __FUNCTION__);
            usleep(1000);
        }
    } while (ret && tries > 0);

    if (ret) {
        LOGE("%s: could not lock gralloc buffer", __FUNCTION__);
    } else {
        // The data we get is in YUV... but Window is RGB565. It needs to be converted
        switch (format) {
            case Overlay::FORMAT_YUV422I:
                Yuv422iToRgb565((char*)vaddr, frame, lcdev->previewWidth, lcdev->previewHeight, stride);
                break;
            case Overlay::FORMAT_YUV420P:
            case Overlay::FORMAT_YUV420SP:
                Yuv420spToRgb565((char*)vaddr, frame, lcdev->previewWidth, lcdev->previewHeight, stride);
                break;
            case Overlay::FORMAT_RGB565:
                memcpy(vaddr, frame, size);
                break;
            default:
                LOGE("%s: Unknown video format, cannot convert!", __FUNCTION__);
        }
        lcdev->gralloc->unlock(lcdev->gralloc, *bufHandle);
    }

    if (lcdev->window->enqueue_buffer(lcdev->window, bufHandle) != 0) {
        LOGE("%s: could not enqueue gralloc buffer", __FUNCTION__);
    }
}

/*
 * typedef void (*QueueBufferHook)(void *data, void* buffer, size_t size);
 */
static void overlayQueueBuffer(void *data, void *buffer, size_t size)
{
#ifdef LOG_EACH_FRAME
    LOGV("%s: queue buffer hook data=%p, buffer=%p, size=%d", __FUNCTION__, data, buffer, size);
#endif
    if (data != NULL && buffer != NULL) {
        legacy_camera_device *lcdev = (legacy_camera_device *) data;
        Overlay::Format format = (Overlay::Format) lcdev->overlay->getFormat();
        processPreviewData((char*)buffer, size, lcdev, format);
    }
}

static camera_memory_t* genClientData(legacy_camera_device *lcdev,
                                      const sp<IMemory> &dataPtr)
{
    ssize_t          offset;
    size_t           size;
    camera_memory_t *clientData = NULL;
    sp<IMemoryHeap> mHeap = dataPtr->getMemory(&offset, &size);

    LOGV("%s: offset:%#x size:%#x base:%p", __FUNCTION__,
          (unsigned)offset, size, mHeap != NULL ? mHeap->base() : 0);

    clientData = lcdev->request_memory(-1, size, 1, lcdev->user);
    if (clientData != NULL) {
        LOGV("%s: clientData=%p -> clientData->data=%p off %ld size %Zd", __FUNCTION__,
            clientData, clientData->data, offset, size);
        memcpy(clientData->data, (char *)(mHeap->base()) + offset, size);
    } else {
        LOGV("%s: ERROR allocating memory from client", __FUNCTION__);
    }
    return clientData;
}

static void dataCallback(int32_t msgType, const sp<IMemory>& dataPtr, void* user)
{
    struct legacy_camera_device *lcdev = (struct legacy_camera_device *) user;

    LOGV("%s: msgType:0x%x user:%p", __FUNCTION__, msgType, user);

    if (lcdev->data_callback != NULL && lcdev->request_memory != NULL) {
        if (lcdev->clientData != NULL) {
            lcdev->clientData->release(lcdev->clientData);
        }
        lcdev->clientData = genClientData(lcdev, dataPtr);
        if (lcdev->clientData != NULL) {
            LOGV("%s: Posting data to client", __FUNCTION__);
            lcdev->data_callback(msgType, lcdev->clientData, 0, NULL, lcdev->user);
        }
    }

    if (msgType == CAMERA_MSG_PREVIEW_FRAME) {
        if (lcdev->overlay != NULL) {
            LOGW("%s: overlay is not null, skipping...", __FUNCTION__);
            return;
        }
        ssize_t offset;
        size_t  size;
        sp<IMemoryHeap> mHeap = dataPtr->getMemory(&offset, &size);
        char* buffer = (char*) mHeap->getBase() + offset;

        LOGV("%s: preview size = %dx%d", __FUNCTION__, lcdev->previewWidth, lcdev->previewHeight);
        processPreviewData(buffer, size, lcdev, lcdev->previewFormat);
    }
}

static void dataTimestampCallback(nsecs_t timestamp, int32_t msgType,
                                  const sp<IMemory>& dataPtr, void *user)
{
    struct legacy_camera_device *lcdev = (struct legacy_camera_device *) user;

    LOGV("%s: timestamp:%lld msgType:%d user:%p", __FUNCTION__,
            timestamp /1000, msgType, user);

    if (lcdev->data_timestamp_callback != NULL && lcdev->request_memory != NULL) {
        camera_memory_t *mem = genClientData(lcdev, dataPtr);
        if (mem != NULL) {
            LOGV("%s: Posting data to client timestamp:%lld", __FUNCTION__,
                  systemTime());
            lcdev->sentFrames.push_back(mem);
            lcdev->data_timestamp_callback(timestamp, msgType, mem, /*index*/0, lcdev->user);
            lcdev->hwif->releaseRecordingFrame(dataPtr);
        } else {
            LOGD("%s: ERROR allocating memory from client", __FUNCTION__);
        }
    }
}

static void notifyCallback(int32_t msgType, int32_t ext1, int32_t ext2, void *user)
{
    struct legacy_camera_device *lcdev = (struct legacy_camera_device *) user;

    LOGV("%s: msgType:0x%x ext1:%d ext2:%d user:%p", __FUNCTION__, msgType, ext1, ext2, user);
    if (lcdev->notify_callback != NULL) {
        lcdev->notify_callback(msgType, ext1, ext2, lcdev->user);
    }
}

inline void destroyOverlay(legacy_camera_device *lcdev)
{
    LOGV("%s\n", __FUNCTION__);
    if (lcdev->overlay != NULL) {
        lcdev->overlay.clear();
        if (lcdev->hwif != NULL) {
            lcdev->hwif->setOverlay(lcdev->overlay);
        }
    }
}

static void releaseCameraFrames(legacy_camera_device *lcdev)
{
    vector<camera_memory_t*>::iterator it;
    for (it = lcdev->sentFrames.begin(); it != lcdev->sentFrames.end(); ++it) {
        (*it)->release(*it);
    }
    lcdev->sentFrames.clear();
}

/* Hardware Camera interface handlers. */
static int camera_set_preview_window(struct camera_device * device, struct preview_stream_ops *window)
{
    int rv = -EINVAL;
    const int kBufferCount = 6;
    struct legacy_camera_device *lcdev = to_lcdev(device);

    LOGV("%s: Window %p\n", __FUNCTION__, window);
    if (device == NULL) {
        LOGE("%s: Invalid device.\n", __FUNCTION__);
        return -EINVAL;
    }

    if (lcdev->window == window && window) {
        LOGV("%s: reconfiguring window %p", __FUNCTION__, window);
        destroyOverlay(lcdev);
    }

    lcdev->window = window;

    if (!window) {
        // It means we need to release old window
        LOGV("%s: releasing previous window", __FUNCTION__);
        destroyOverlay(lcdev);
        return NO_ERROR;
    }

    LOGD("%s: OK window is %p", __FUNCTION__, window);

    if (!lcdev->gralloc) {
        hw_module_t const* module;
        int err = 0;
        if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module) == 0) {
            lcdev->gralloc = (const gralloc_module_t *)module;
            LOGD("%s: loaded gralloc, module name=%s; author=%s", __FUNCTION__, module->name, module->author);
        } else {
            LOGE("%s: Fail on loading gralloc HAL", __FUNCTION__);
        }
    }

    LOGD("%s: OK on loading gralloc HAL", __FUNCTION__);
    int min_bufs = -1;
    if (window->get_min_undequeued_buffer_count(window, &min_bufs)) {
        LOGE("%s: could not retrieve min undequeued buffer count", __FUNCTION__);
        return -1;
    }
    LOGD("%s: OK get_min_undequeued_buffer_count", __FUNCTION__);

    LOGD("%s: minimum buffer count is %i", __FUNCTION__, min_bufs);
    if (min_bufs >= kBufferCount) {
        LOGE("%s: min undequeued buffer count %i is too high (expecting at most %i)", __FUNCTION__, min_bufs, kBufferCount - 1);
    }

    LOGD("%s: setting buffer count to %i", __FUNCTION__, kBufferCount);
    if (window->set_buffer_count(window, kBufferCount)) {
        LOGE("%s: could not set buffer count", __FUNCTION__);
        return -1;
    }

    CameraParameters params(lcdev->hwif->getParameters());
    params.getPreviewSize(&lcdev->previewWidth, &lcdev->previewHeight);

    const char *previewFormat = params.getPreviewFormat();
    LOGD("%s: preview format %s", __FUNCTION__, previewFormat);
    lcdev->previewFormat = Overlay::getFormatFromString(previewFormat);

    if (window->set_usage(window, GRALLOC_USAGE_SW_WRITE_MASK)) { //GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_SW_READ_OFTEN)) {
        LOGE("%s: could not set usage on gralloc buffer", __FUNCTION__);
        return -1;
    }

    if (window->set_buffers_geometry(window, lcdev->previewWidth, lcdev->previewHeight, HAL_PIXEL_FORMAT_RGB_565)) {
        LOGE("%s: could not set buffers geometry (%dx%d)", __FUNCTION__, lcdev->previewWidth, lcdev->previewHeight);
        return -1;
    }

    if (lcdev->hwif->useOverlay()) {
        LOGI("%s: Using overlay for device %p (%dx%d)", __FUNCTION__, lcdev, lcdev->previewWidth, lcdev->previewHeight);
        lcdev->overlay = new Overlay(lcdev->previewWidth, lcdev->previewHeight,
                Overlay::FORMAT_YUV420SP, overlayQueueBuffer, (void*) lcdev);

    //Note: Tegra camera libs never use Overlay::enqueueBuffer
    if (lcdev->hwif->useOverlay()) {
        int i, ret;
        //LOGV("%s, kBufferCount=%d", __FUNCTION__, kBufferCount);
        for (i=0; i < kBufferCount - 1; i++) {
            ret = lcdev->overlay->queueBuffer((overlay_buffer_t) i);
            if (ret != NO_ERROR) {
                LOGE("%s: ERROR enqueueing a buffer, ret=%d", __FUNCTION__, ret);
                break;
            }
        }
    }


        lcdev->hwif->setOverlay(lcdev->overlay);
    } else {
        LOGW("%s: Not using overlay !", __FUNCTION__);
    }

    return NO_ERROR;
}

static void camera_set_callbacks(struct camera_device * device,
                                 camera_notify_callback notify_cb,
                                 camera_data_callback data_cb,
                                 camera_data_timestamp_callback data_cb_timestamp,
                                 camera_request_memory get_memory,
                                 void *user)
{
    struct legacy_camera_device *lcdev = to_lcdev(device);

    LOGV("%s: notify_cb: %p, data_cb: %p data_cb_timestamp: %p,"
            " get_memory: %p, user :%p", __FUNCTION__,
            notify_cb, data_cb, data_cb_timestamp, get_memory, user);

    lcdev->notify_callback = notify_cb;
    lcdev->data_callback = data_cb;
    lcdev->data_timestamp_callback = data_cb_timestamp;
    lcdev->request_memory = get_memory;
    lcdev->user = user;

    lcdev->hwif->setCallbacks(notifyCallback, dataCallback, dataTimestampCallback, lcdev);
}

static void camera_enable_msg_type(struct camera_device * device, int32_t msg_type)
{
    struct legacy_camera_device *lcdev = to_lcdev(device);
    LOGV("%s: msg_type=0x%x", __FUNCTION__, msg_type);
    lcdev->hwif->enableMsgType(msg_type);
}

static void camera_disable_msg_type(struct camera_device * device, int32_t msg_type)
{
    struct legacy_camera_device *lcdev = to_lcdev(device);
    LOGV("%s: msg_type=0x%x", __FUNCTION__, msg_type);
    if (msg_type == CAMERA_MSG_VIDEO_FRAME) {
        LOGW("%s: releasing stale video frames", __FUNCTION__);
        releaseCameraFrames(lcdev);
    }
    lcdev->hwif->disableMsgType(msg_type);
}

static int camera_msg_type_enabled(struct camera_device * device, int32_t msg_type)
{
    struct legacy_camera_device *lcdev = to_lcdev(device);
    LOGV("%s: msg_type=0x%x", __FUNCTION__, msg_type);
    return lcdev->hwif->msgTypeEnabled(msg_type);
}

static int camera_start_preview(struct camera_device * device)
{
    struct legacy_camera_device *lcdev = to_lcdev(device);
    LOGV("%s", __FUNCTION__);
    lcdev->hwif->enableMsgType(CAMERA_MSG_PREVIEW_FRAME);
    return lcdev->hwif->startPreview();
}

static void camera_stop_preview(struct camera_device * device)
{
    struct legacy_camera_device *lcdev = to_lcdev(device);
    LOGV("%s", __FUNCTION__);
    lcdev->hwif->stopPreview();
    return;
}

static int camera_preview_enabled(struct camera_device * device)
{
    struct legacy_camera_device *lcdev = to_lcdev(device);
    int ret = lcdev->hwif->previewEnabled();
    LOGV("%s: %d", __FUNCTION__, ret);
    return ret;
}

static int camera_store_meta_data_in_buffers(struct camera_device * device, int enable)
{
    LOGW("%s", __FUNCTION__);
    return INVALID_OPERATION;
}

static int camera_start_recording(struct camera_device * device)
{
    struct legacy_camera_device *lcdev = to_lcdev(device);
    LOGV("%s", __FUNCTION__);
    lcdev->hwif->enableMsgType(CAMERA_MSG_VIDEO_FRAME);
    lcdev->hwif->startRecording();
    return NO_ERROR;
}

static void camera_stop_recording(struct camera_device * device)
{
    struct legacy_camera_device *lcdev = to_lcdev(device);
    LOGV("%s", __FUNCTION__);
    lcdev->hwif->disableMsgType(CAMERA_MSG_VIDEO_FRAME);
    lcdev->hwif->stopRecording();
}

static int camera_recording_enabled(struct camera_device * device)
{
    struct legacy_camera_device *lcdev = to_lcdev(device);
    LOGV("%s", __FUNCTION__);
    return lcdev->hwif->recordingEnabled() ? 1 : 0;
}

static void camera_release_recording_frame(struct camera_device * device, const void *opaque)
{
    LOGV("%s: opaque=%p\n", __FUNCTION__, opaque);
    struct legacy_camera_device *lcdev = to_lcdev(device);
    if (opaque != NULL) {
        vector<camera_memory_t*>::iterator it;
        for (it = lcdev->sentFrames.begin(); it != lcdev->sentFrames.end(); ++it) {
            camera_memory_t *mem = *it;
            if (mem->data == opaque) {
                LOGV("found, removing");
                mem->release(mem);
                lcdev->sentFrames.erase(it);
                break;
            }
        }
    }
}

static int camera_auto_focus(struct camera_device * device)
{
    struct legacy_camera_device *lcdev = to_lcdev(device);
    LOGV("%s", __FUNCTION__);
    lcdev->hwif->autoFocus();
    return NO_ERROR;
}

static int camera_cancel_auto_focus(struct camera_device * device)
{
    struct legacy_camera_device *lcdev = to_lcdev(device);
    LOGV("%s", __FUNCTION__);
    lcdev->hwif->cancelAutoFocus();
    return NO_ERROR;
}

static int camera_take_picture(struct camera_device * device)
{
    struct legacy_camera_device *lcdev = to_lcdev(device);
    LOGV("%s", __FUNCTION__);
    lcdev->hwif->enableMsgType(CAMERA_MSG_SHUTTER | CAMERA_MSG_POSTVIEW_FRAME |
                               CAMERA_MSG_RAW_IMAGE | CAMERA_MSG_COMPRESSED_IMAGE);
    lcdev->hwif->takePicture();
    return NO_ERROR;
}

static int camera_cancel_picture(struct camera_device * device)
{
    struct legacy_camera_device *lcdev = to_lcdev(device);
    LOGV("%s", __FUNCTION__);
    lcdev->hwif->cancelPicture();
    return NO_ERROR;
}

static int camera_set_parameters(struct camera_device * device, const char *params)
{
    struct legacy_camera_device *lcdev = to_lcdev(device);
    String8 s(params);
    CameraParameters p(s);
#ifdef LOG_FULL_PARAMS
    LOGV("%s: Parameters", __FUNCTION__);
    p.dump();
#endif
    lcdev->hwif->setParameters(p);
    return NO_ERROR;
}

static char* camera_get_parameters(struct camera_device * device)
{
    struct legacy_camera_device *lcdev = to_lcdev(device);
    CameraParameters params(lcdev->hwif->getParameters());

    int width = 0, height = 0;

    params.getPictureSize(&width, &height);
    if (width > 0 && height > 0) {
        float ratio = (width * 1.0) / (height * 1.0);

        if (ratio > 1.55 && width >= 2592) {
//            params.setPreviewSize(960, 540);
//            params.set(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO, "1280x720");
        } else if (width == 2592) {
//            params.setPreviewSize(720, 540);
//            params.set(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO, "640x480");
        }

        LOGW("%s: target size %dx%d, ratio %f", __FUNCTION__, width, height, ratio);
    }

/*
    params.getPreviewSize(&width, &height);
    if (width != lcdev->previewWidth || height != lcdev->previewHeight) {
        LOGW("%s: change to preview size %dx%d => %dx%d", __FUNCTION__,
            lcdev->previewWidth, lcdev->previewHeight, width, height);
        camera_set_preview_window(device, lcdev->window);
    }
*/

#ifdef LOG_FULL_PARAMS
    LOGV("%s: Parameters", __FUNCTION__);
    params.dump();
#endif

    return strdup(params.flatten().string());
}

static void camera_put_parameters(struct camera_device *device, char *params)
{
    if (params != NULL) {
        free(params);
    }
}

static int camera_send_command(struct camera_device * device, int32_t cmd, int32_t arg0, int32_t arg1)
{
    struct legacy_camera_device *lcdev = to_lcdev(device);
    LOGV("%s: cmd:%d arg0:%d arg1:%d\n", __FUNCTION__, cmd, arg0, arg1);
    return lcdev->hwif->sendCommand(cmd, arg0, arg1);
}

static void camera_release(struct camera_device * device)
{
    struct legacy_camera_device *lcdev = to_lcdev(device);
    LOGD("%s()\n", __FUNCTION__);
    destroyOverlay(lcdev);
    releaseCameraFrames(lcdev);
    if (lcdev->clientData != NULL) {
        lcdev->clientData->release(lcdev->clientData);
        lcdev->clientData = NULL;
    }
    lcdev->hwif->release();
    lcdev->hwif.clear();
}

static int camera_dump(struct camera_device * device, int fd)
{
    struct legacy_camera_device *lcdev = to_lcdev(device);
    LOGV("camera_dump:\n");
    Vector<String16> args;
    return lcdev->hwif->dump(fd, args);
}

static int camera_device_close(hw_device_t* device)
{
    struct camera_device * hwdev = reinterpret_cast<struct camera_device *>(device);
    struct legacy_camera_device *lcdev = to_lcdev(hwdev);
    int rc = -EINVAL;

    LOGD("%s()\n", __FUNCTION__);
    if (lcdev != NULL) {
        camera_device_ops_t *camera_ops = lcdev->device.ops;
        if (camera_ops) {
            free(camera_ops);
            camera_ops = NULL;
        }
        destroyOverlay(lcdev);
        free(lcdev);
        rc = NO_ERROR;
    }
    return rc;
}

static int camera_device_open(const hw_module_t* module, const char* name, hw_device_t** device)
{
    struct legacy_camera_device *lcdev;
    camera_device_t* camera_device;
    camera_device_ops_t* camera_ops;

    if (name == NULL) {
        return NO_ERROR;
    }

    int cameraId = atoi(name);

    LOGD("%s: name:%s device:%p cameraId:%d\n", __FUNCTION__, name, device, cameraId);

    lcdev = (struct legacy_camera_device *) calloc(1, sizeof(*lcdev));
    if (lcdev == NULL) {
        return -ENOMEM;
    }

    camera_ops = (camera_device_ops_t *) calloc(1, sizeof(*camera_ops));
    if (camera_ops == NULL) {
        free(lcdev);
        return -ENOMEM;
    }

    lcdev->device.common.tag               = HARDWARE_DEVICE_TAG;
    lcdev->device.common.version           = 0;
    lcdev->device.common.module            = (hw_module_t *) module;
    lcdev->device.common.close             = camera_device_close;
    lcdev->device.ops                      = camera_ops;

    camera_ops->set_preview_window         = camera_set_preview_window;
    camera_ops->set_callbacks              = camera_set_callbacks;
    camera_ops->enable_msg_type            = camera_enable_msg_type;
    camera_ops->disable_msg_type           = camera_disable_msg_type;
    camera_ops->msg_type_enabled           = camera_msg_type_enabled;
    camera_ops->start_preview              = camera_start_preview;
    camera_ops->stop_preview               = camera_stop_preview;
    camera_ops->preview_enabled            = camera_preview_enabled;
    camera_ops->store_meta_data_in_buffers = camera_store_meta_data_in_buffers;
    camera_ops->start_recording            = camera_start_recording;
    camera_ops->stop_recording             = camera_stop_recording;
    camera_ops->recording_enabled          = camera_recording_enabled;
    camera_ops->release_recording_frame    = camera_release_recording_frame;
    camera_ops->auto_focus                 = camera_auto_focus;
    camera_ops->cancel_auto_focus          = camera_cancel_auto_focus;
    camera_ops->take_picture               = camera_take_picture;
    camera_ops->cancel_picture             = camera_cancel_picture;

    camera_ops->set_parameters             = camera_set_parameters;
    camera_ops->get_parameters             = camera_get_parameters;
    camera_ops->put_parameters             = camera_put_parameters;
    camera_ops->send_command               = camera_send_command;
    camera_ops->release                    = camera_release;
    camera_ops->dump                       = camera_dump;

    lcdev->id = cameraId;
    lcdev->hwif = CameraWrapper::createInstance(cameraId);
    if (lcdev->hwif == NULL) {
        free(camera_ops);
        free(lcdev);
        return -EIO;
    }

    *device = &lcdev->device.common;
    return NO_ERROR;
}

static int get_number_of_cameras(void)
{
    // or HAL_getNumberOfCameras();

    return 2;
}

static int get_camera_info(int camera_id, struct camera_info *info)
{
    if (camera_id == 0) {
       info->facing = CAMERA_FACING_BACK;
       info->orientation = 90;
    } else {
       info->facing = CAMERA_FACING_FRONT;
       info->orientation = 0;
    }

    // or HAL_getCameraInfo(camera_id, &info);

    LOGD("%s: id:%i facing:%i orientation:%i", __FUNCTION__,
          camera_id, info->facing, info->orientation);

    return 0;
}

} /* namespace android */

static hw_module_methods_t camera_module_methods = {
    open: android::camera_device_open
};

camera_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 1,
        id: CAMERA_HARDWARE_MODULE_ID,
        name: "Camera HAL for ICS/CM9",
        author: "Atrix Dev Team, Epsylon3",
        methods: &camera_module_methods,
        dso: NULL,
        reserved: {0},
    },
    get_number_of_cameras: android::get_number_of_cameras,
    get_camera_info: android::get_camera_info,
};

