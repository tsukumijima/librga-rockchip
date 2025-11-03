/*
 * Copyright (C) 2025 Rockchip Electronics Co., Ltd.
 * Authors:
 *  Cerf Yu <cerf.yu@rock-chips.com>
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

#ifdef ANDROID

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <utils/Log.h>

#include "rga_gralloc.h"

#if USE_GRALLOC_4
#include <hardware/gralloc.h>

#include "platform_gralloc4.h"
#elif USE_GRALLOC_5
#include <hardware/gralloc.h>

#include "platform_gralloc5.h"
#else /* Gralloc 0.3 */

#if (defined(ANDROID_7_DRM) && !defined(RK3368))
#include <hardware/gralloc.h>
#else
#ifdef RK3368
#include <hardware/gralloc.h>
#include <hardware/img_gralloc_public.h>

#define private_handle_t IMG_native_handle_t
#else
#include <gralloc_priv.h>
#endif /* #ifdef RK3368 */
#endif /* #if (defined(ANDROID_7_DRM) && !defined(RK3368)) */

/*
 *   Only these macros in rockchip's private gralloc-0.3 header are
 * needed in librga, and these are defined in different paths for
 * different Android versions.
 *   So when librga refers to gralloc0.3, it will judge whether
 * there is a reference to the corresponding path, and if not,
 * define these macros by itself.
 */
#ifndef GRALLOC_MODULE_PERFORM_GET_HADNLE_PRIME_FD
#define GRALLOC_MODULE_PERFORM_GET_HADNLE_PRIME_FD 0x08100002
#endif

#ifndef GRALLOC_MODULE_PERFORM_GET_HADNLE_ATTRIBUTES
#define GRALLOC_MODULE_PERFORM_GET_HADNLE_ATTRIBUTES 0x08100004
#endif

#ifndef GRALLOC_MODULE_PERFORM_GET_INTERNAL_FORMAT
#define GRALLOC_MODULE_PERFORM_GET_INTERNAL_FORMAT 0x08100006
#endif

#ifndef GRALLOC_MODULE_PERFORM_GET_HADNLE_WIDTH
#define GRALLOC_MODULE_PERFORM_GET_HADNLE_WIDTH 0x08100008
#endif

#ifndef GRALLOC_MODULE_PERFORM_GET_HADNLE_HEIGHT
#define GRALLOC_MODULE_PERFORM_GET_HADNLE_HEIGHT 0x0810000A
#endif

#ifndef GRALLOC_MODULE_PERFORM_GET_HADNLE_STRIDE
#define GRALLOC_MODULE_PERFORM_GET_HADNLE_STRIDE 0x0810000C
#endif

#ifndef GRALLOC_MODULE_PERFORM_GET_HADNLE_FORMAT
#define GRALLOC_MODULE_PERFORM_GET_HADNLE_FORMAT 0x08100010
#endif

#ifndef GRALLOC_MODULE_PERFORM_GET_HADNLE_SIZE
#define GRALLOC_MODULE_PERFORM_GET_HADNLE_SIZE 0x08100012
#endif

#ifndef GRALLOC_MODULE_PERFORM_GET_USAGE
#define GRALLOC_MODULE_PERFORM_GET_USAGE 0x0feeff03
#endif

gralloc_module_t const *g_gralloc = NULL;

static int rga_get_gralloc_module() {
    const hw_module_t *module = NULL;
    int ret = 0;

    if (g_gralloc)
        return 0;

    ret= hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
    ALOGE_IF(ret, "FATAL:can't find the %s module",GRALLOC_HARDWARE_MODULE_ID);
    if (ret == 0)
        g_gralloc = reinterpret_cast<gralloc_module_t const *>(module);

    return ret;
}

static int rga_gralloc_perform_backend(buffer_handle_t handle, int op, int *value) {
#if (defined(ANDROID_7_DRM) && !defined(RK3368))
    int ret = 0;

    if (!g_gralloc) {
        ret = rga_get_gralloc_module();
        if (ret < 0)
            return ret;
    }

    if (g_gralloc->perform) {
        ret = g_gralloc->perform(g_gralloc, op, handle, value);
    } else {
        return -EINVAL;
    }

    return ret;
#else
    private_handle_t *priv_handle = (private_handle_t *)handle;

    switch (op) {
        case GRALLOC_MODULE_PERFORM_GET_HADNLE_PRIME_FD:
#ifdef RK3368
            *value = priv_handle->fd[0];
#else
            *value = priv_handle->share_fd;
#endif
            return 0;
        case GRALLOC_MODULE_PERFORM_GET_HADNLE_WIDTH:
            *value = priv_handle->width;
            return 0;
        case GRALLOC_MODULE_PERFORM_GET_HADNLE_HEIGHT:
            *value = priv_handle->height;
            return 0;
        case GRALLOC_MODULE_PERFORM_GET_HADNLE_STRIDE:
            *value = priv_handle->stride;
            return 0;
        case GRALLOC_MODULE_PERFORM_GET_HADNLE_FORMAT:
            *value = priv_handle->format;
            return 0;
        case GRALLOC_MODULE_PERFORM_GET_HADNLE_SIZE:
            *value = priv_handle->size;
            return 0;
    }
#endif
}

#endif /* #if USE_GRALLOC_4 */

int rga_gralloc_get_handle_fd(buffer_handle_t handle) {
    int ret = 0;
    int share_fd;

#if USE_GRALLOC_4
    int err = gralloc4::get_share_fd(handle, &share_fd);
    ret = (err != android::OK) ? -EINVAL : 0;
#elif USE_GRALLOC_5
    int err = gralloc5::get_share_fd(handle, &share_fd);
    ret = (err != android::OK) ? -EINVAL : 0;
#else
    ret = rga_gralloc_perform_backend(handle, GRALLOC_MODULE_PERFORM_GET_HADNLE_PRIME_FD, &share_fd);
#endif /* #if USE_GRALLOC_4 */

    if (ret != 0) {
        ALOGE("cannot get fd from gralloc");
        return ret;
    }

    return share_fd;
}

int rga_gralloc_get_handle_width(buffer_handle_t handle) {
    int ret = 0;

#if USE_GRALLOC_4
    uint64_t width;

    int err = gralloc4::get_width(handle, &width);
    ret = (err != android::OK) ? -EINVAL : 0;
#elif USE_GRALLOC_5
    uint64_t width;

    int err = gralloc5::get_width(handle, &width);
    ret = (err != android::OK) ? -EINVAL : 0;
#else
    int width;

    ret = rga_gralloc_perform_backend(handle, GRALLOC_MODULE_PERFORM_GET_HADNLE_WIDTH, &width);
#endif /* #if USE_GRALLOC_4 */

    if (ret != 0) {
        ALOGE("cannot get width from gralloc");
        return ret;
    }

    return (int)width;
}

int rga_gralloc_get_handle_height(buffer_handle_t handle) {
    int ret = 0;

#if USE_GRALLOC_4
    uint64_t height;

    int err = gralloc4::get_height(handle, &height);
    ret = (err != android::OK) ? -EINVAL : 0;
#elif USE_GRALLOC_5
    uint64_t height;

    int err = gralloc5::get_height(handle, &height);
    ret = (err != android::OK) ? -EINVAL : 0;
#else
    int height;

    ret = rga_gralloc_perform_backend(handle, GRALLOC_MODULE_PERFORM_GET_HADNLE_HEIGHT, &height);
#endif /* #if USE_GRALLOC_4 */

    if (ret != 0) {
        ALOGE("cannot get height from gralloc");
        return ret;
    }

    return (int)height;
}

int rga_gralloc_get_handle_stride(buffer_handle_t handle) {
    int ret = 0;
    int pixel_stride;

#if USE_GRALLOC_4
    int err = gralloc4::get_pixel_stride(handle, &pixel_stride);
    ret = (err != android::OK) ? -EINVAL : 0;
#elif USE_GRALLOC_5
    int err = gralloc5::get_pixel_stride(handle, &pixel_stride);
    ret = (err != android::OK) ? -EINVAL : 0;
#else
    ret = rga_gralloc_perform_backend(handle, GRALLOC_MODULE_PERFORM_GET_HADNLE_STRIDE, &pixel_stride);
#endif /* #if USE_GRALLOC_4 */

    if (ret != 0) {
        ALOGE("cannot get stride from gralloc");
        return ret;
    }

    return (int)pixel_stride;
}

int rga_gralloc_get_handle_height_stride(buffer_handle_t handle) {
    int ret = 0;

#if USE_GRALLOC_4
    int pixel_height_stride;

    int err = gralloc4::get_height_stride(handle, &pixel_height_stride);
    ret = (err != android::OK) ? -EINVAL : 0;
#elif USE_GRALLOC_5
    uint64_t pixel_height_stride;

    int err = gralloc5::get_height_stride(handle, &pixel_height_stride);
    ret = (err != android::OK) ? -EINVAL : 0;
#else
    int pixel_height_stride;

    ret = rga_gralloc_perform_backend(handle, GRALLOC_MODULE_PERFORM_GET_HADNLE_HEIGHT, &pixel_height_stride);
#endif /* #if USE_GRALLOC_4 */

    if (ret != 0) {
        ALOGE("cannot get stride from gralloc");
        return ret;
    }

    return (int)pixel_height_stride;
}

int rga_gralloc_get_handle_format(buffer_handle_t handle) {
    int ret = 0;
    int format;

#if USE_GRALLOC_4
    int err = gralloc4::get_format_requested(handle, &format);
    ret = (err != android::OK) ? -EINVAL : 0;
#elif USE_GRALLOC_5
    int err = gralloc5::get_format_requested(handle, &format);
    ret = (err != android::OK) ? -EINVAL : 0;
#else
    ret = rga_gralloc_perform_backend(handle, GRALLOC_MODULE_PERFORM_GET_HADNLE_FORMAT, &format);
#endif /* #if USE_GRALLOC_4 */

    if (ret != 0) {
        ALOGE("cannot get format from gralloc");
        return ret;
    }

    return (int)format;
}

int rga_gralloc_get_handle_size(buffer_handle_t handle) {
    int ret = 0;

#if USE_GRALLOC_4
    uint64_t size;

    int err = gralloc4::get_allocation_size(handle, &size);
    ret = (err != android::OK) ? -EINVAL : 0;
#elif USE_GRALLOC_5
    uint64_t size;

    int err = gralloc5::get_allocation_size(handle, &size);
    ret = (err != android::OK) ? -EINVAL : 0;
#else
    int size;

    ret = rga_gralloc_perform_backend(handle, GRALLOC_MODULE_PERFORM_GET_HADNLE_SIZE, &size);
#endif /* #if USE_GRALLOC_4 */

    if (ret != 0) {
        ALOGE("cannot get size from gralloc");
        return ret;
    }

    return (int)size;
}

uint32_t rga_gralloc_get_handle_drm_fourcc(buffer_handle_t handle) {
    int ret = 0;
    uint32_t drm_fourcc;

#if USE_GRALLOC_4
    drm_fourcc = gralloc4::get_fourcc_format(handle);
#elif USE_GRALLOC_5
    drm_fourcc = gralloc5::get_fourcc_format(handle);
#else
    drm_fourcc = 0;
#endif /* #if USE_GRALLOC_4 */

    return drm_fourcc;
}

uint64_t rga_gralloc_get_handle_drm_modifier(buffer_handle_t handle) {
    int ret = 0;
    uint64_t drm_modifier;

#if USE_GRALLOC_4
    drm_modifier = gralloc4::get_format_modifier(handle);
#elif USE_GRALLOC_5
    drm_modifier = gralloc5::get_format_modifier(handle);
#else
    drm_modifier = 0;
#endif /* #if USE_GRALLOC_4 */

    return drm_modifier;
}

void *rga_gralloc_get_handle_virtual_addr(buffer_handle_t handle) {
    int ret = 0;
    int usage = GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK;
    void *buf = NULL;

#if USE_GRALLOC_4
    int wstride, hstride;

    ret = gralloc4::get_pixel_stride(handle, &wstride);
    if (ret != android::OK)
    {
        ALOGE("Failed to get buffer width, ret : %d", ret);
        return NULL;
    }

    ret = gralloc4::get_height_stride(handle, &hstride);
    if (ret != android::OK)
    {
        ALOGE("Failed to get buffer height, ret : %d", ret);
        return NULL;
    }

    ret = gralloc4::lock(handle, usage, 0, 0, wstride, hstride, &buf);
    if (ret != android::OK)
    {
        ALOGE("Failed to lock buffer, ret : %d", ret);
        return NULL;
    }

    gralloc4::unlock(handle);
#elif USE_GRALLOC_5
    int wstride;
    uint64_t hstride;

    ret = gralloc5::get_pixel_stride(handle, &wstride);
    if (ret != android::OK)
    {
        ALOGE("Failed to get buffer width, ret : %d", ret);
        return NULL;
    }

    ret = gralloc5::get_height_stride(handle, &hstride);
    if (ret != android::OK)
    {
        ALOGE("Failed to get buffer height, ret : %d", ret);
        return NULL;
    }

    ret = gralloc5::lock(handle, usage, 0, 0, wstride, hstride, &buf);
    if (ret != android::OK)
    {
        ALOGE("Failed to lock buffer, ret : %d", ret);
        return NULL;
    }

    gralloc5::unlock(handle);
#else
#ifdef ANDROID_7_DRM
    usage |= GRALLOC_USAGE_HW_FB;
#endif

    if (!g_gralloc) {
        ret = rga_get_gralloc_module();
        if (ret < 0)
            return NULL;
    }

    if (g_gralloc->lock)
        ret = g_gralloc->lock(g_gralloc, handle, usage, 0, 0, 0, 0, &buf);
    else
        return NULL;

    if (ret != 0)
        ALOGE("cannot get virtual address from gralloc, ret = %d, %s", ret, strerror(ret));
#endif /* #if USE_GRALLOC_4 */

    return buf;
}

#endif /* #ifdef ANDROID */
