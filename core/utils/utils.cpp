/*
 * Copyright (C) 2022 Rockchip Electronics Co., Ltd.
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

#include "rga.h"
#include "utils.h"

bool is_bpp_format(int format) {
    bool ret = false;

    switch (format) {
        case RK_FORMAT_BPP1:
        case RK_FORMAT_BPP2:
        case RK_FORMAT_BPP4:
        case RK_FORMAT_BPP8:
            ret = true;
            break;
        default:
            break;
    }

    return ret;
}

bool is_yuv_format(int format) {
    bool ret = false;

    switch (format) {
        case RK_FORMAT_YCbCr_422_SP:
        case RK_FORMAT_YCbCr_422_P:
        case RK_FORMAT_YCbCr_420_SP:
        case RK_FORMAT_YCbCr_420_P:
        case RK_FORMAT_YCrCb_422_SP:
        case RK_FORMAT_YCrCb_422_P:
        case RK_FORMAT_YCrCb_420_SP:
        case RK_FORMAT_YCrCb_420_P:
        case RK_FORMAT_YVYU_422:
        case RK_FORMAT_YVYU_420:
        case RK_FORMAT_VYUY_422:
        case RK_FORMAT_VYUY_420:
        case RK_FORMAT_YUYV_422:
        case RK_FORMAT_YUYV_420:
        case RK_FORMAT_UYVY_422:
        case RK_FORMAT_UYVY_420:
        case RK_FORMAT_Y4:
        case RK_FORMAT_YCbCr_400:
        case RK_FORMAT_YCbCr_420_SP_10B:
        case RK_FORMAT_YCrCb_420_SP_10B:
        case RK_FORMAT_YCrCb_422_10b_SP:
        case RK_FORMAT_YCbCr_422_10b_SP:
            ret = true;
            break;
    }

    return ret;
}

bool is_rgb_format(int format) {
    bool ret = false;

    switch (format) {
        case RK_FORMAT_RGBA_8888:
        case RK_FORMAT_RGBX_8888:
        case RK_FORMAT_RGBA_5551:
        case RK_FORMAT_RGBA_4444:
        case RK_FORMAT_RGB_888:
        case RK_FORMAT_RGB_565:
        case RK_FORMAT_BGRA_8888:
        case RK_FORMAT_BGRX_8888:
        case RK_FORMAT_BGRA_5551:
        case RK_FORMAT_BGRA_4444:
        case RK_FORMAT_BGR_888:
        case RK_FORMAT_BGR_565:
        /*ARGB*/
        case RK_FORMAT_ARGB_8888:
        case RK_FORMAT_XRGB_8888:
        case RK_FORMAT_ARGB_5551:
        case RK_FORMAT_ARGB_4444:
        case RK_FORMAT_ABGR_8888:
        case RK_FORMAT_XBGR_8888:
        case RK_FORMAT_ABGR_5551:
        case RK_FORMAT_ABGR_4444:
            ret = true;
            break;
        default:
            break;
    }

    return ret;
}

bool is_alpha_format(int format) {
    bool ret = false;

    switch (format) {
        case RK_FORMAT_RGBA_8888:
        case RK_FORMAT_RGBA_5551:
        case RK_FORMAT_RGBA_4444:
        case RK_FORMAT_BGRA_8888:
        case RK_FORMAT_BGRA_5551:
        case RK_FORMAT_BGRA_4444:
        case RK_FORMAT_ARGB_8888:
        case RK_FORMAT_ARGB_5551:
        case RK_FORMAT_ARGB_4444:
        case RK_FORMAT_ABGR_8888:
        case RK_FORMAT_ABGR_5551:
        case RK_FORMAT_ABGR_4444:
        case RK_FORMAT_RGBA2BPP:
            ret = true;
            break;
        default:
            break;
    }

    return ret;
}

static int get_compatible_format(int format) {
#if LINUX
    if (format == 0)
        return format;

    if ((format >> 8) != 0) {
        return format;
    } else {
        return format << 8;
    }
#endif
    return format;
}

int convert_to_rga_format(int ex_format) {
    if (is_drm_fourcc(ex_format))
        return get_format_from_drm_fourcc(ex_format);

    ex_format = get_compatible_format(ex_format);
    if (is_android_hal_format(ex_format))
        return get_format_from_android_hal(ex_format);
    else if (is_rga_format(ex_format))
        return ex_format;

    return RK_FORMAT_UNKNOWN;
}
