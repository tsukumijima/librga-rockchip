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

#include "drm_utils/drm_utils.h"
#include "rga.h"
#include "im2d_type.h"

#ifndef RGA_UTILS_DRM_DISABLE

#include "drm_fourcc.h"

#ifdef __cplusplus
#include <unordered_map>

typedef std::unordered_map<uint32_t, uint32_t> rga_drm_fourcc_map_t;
#else
struct drm_fourcc_format {
    uint32_t drm_format;
    uint32_t rga_format;
};

typedef struct drm_fourcc_format rga_drm_fourcc_map_t[];
#endif /* #ifdef __cplusplus */

const static rga_drm_fourcc_map_t drm_fourcc_table = {
    { DRM_FORMAT_RGBA8888, RK_FORMAT_ABGR_8888 },
    { DRM_FORMAT_BGRA8888, RK_FORMAT_ARGB_8888 },
    { DRM_FORMAT_ARGB8888, RK_FORMAT_BGRA_8888 },
    { DRM_FORMAT_ABGR8888, RK_FORMAT_RGBA_8888 },
    { DRM_FORMAT_RGBX8888, RK_FORMAT_XBGR_8888 },
    { DRM_FORMAT_BGRX8888, RK_FORMAT_XRGB_8888 },
    { DRM_FORMAT_XRGB8888, RK_FORMAT_BGRX_8888 },
    { DRM_FORMAT_XBGR8888, RK_FORMAT_RGBX_8888 },

    { DRM_FORMAT_RGBA5551, RK_FORMAT_ABGR_5551 },
    { DRM_FORMAT_BGRA5551, RK_FORMAT_ARGB_5551 },
    { DRM_FORMAT_ARGB1555, RK_FORMAT_BGRA_5551 },
    { DRM_FORMAT_ABGR1555, RK_FORMAT_RGBA_5551 },
    { DRM_FORMAT_RGBA4444, RK_FORMAT_ABGR_4444 },
    { DRM_FORMAT_BGRA4444, RK_FORMAT_ARGB_4444 },
    { DRM_FORMAT_ARGB4444, RK_FORMAT_BGRA_4444 },
    { DRM_FORMAT_ABGR4444, RK_FORMAT_RGBA_4444 },

    { DRM_FORMAT_RGB888, RK_FORMAT_BGR_888 },
    { DRM_FORMAT_BGR888, RK_FORMAT_RGB_888 },
    { DRM_FORMAT_RGB565, RK_FORMAT_BGR_565 },
    { DRM_FORMAT_BGR565, RK_FORMAT_RGB_565 },

    { DRM_FORMAT_NV16, RK_FORMAT_YCbCr_422_SP },
    { DRM_FORMAT_NV61, RK_FORMAT_YCrCb_422_SP },
    { DRM_FORMAT_YUV422, RK_FORMAT_YCbCr_422_P },
    { DRM_FORMAT_YVU422, RK_FORMAT_YCrCb_422_P },
    // { , RK_FORMAT_YCbCr_422_SP_10B },
    // { , RK_FORMAT_YCrCb_422_SP_10B },
    { DRM_FORMAT_NV12, RK_FORMAT_YCbCr_420_SP },
    { DRM_FORMAT_NV21, RK_FORMAT_YCrCb_420_SP },
    { DRM_FORMAT_YUV420, RK_FORMAT_YCbCr_420_P },
    { DRM_FORMAT_YVU420, RK_FORMAT_YCrCb_420_P },
    { DRM_FORMAT_NV15, RK_FORMAT_YCbCr_420_SP_10B },
    // { , RK_FORMAT_YCrCb_420_SP_10B },

    { DRM_FORMAT_YUYV, RK_FORMAT_YUYV_422 },
    { DRM_FORMAT_YVYU, RK_FORMAT_YVYU_422 },
    { DRM_FORMAT_UYVY, RK_FORMAT_UYVY_422 },
    { DRM_FORMAT_VYUY, RK_FORMAT_VYUY_422 },
    // { , RK_FORMAT_YUYV_420 },
    // { , RK_FORMAT_YVYU_420 },
    // { , RK_FORMAT_UYVY_420 },
    // { , RK_FORMAT_VYUY_420 },

    // { , RK_FORMAT_Y4 },
    // { , RK_FORMAT_YCbCr_400 },

    // { , RK_FORMAT_BPP1 },
    // { , RK_FORMAT_BPP2 },
    // { , RK_FORMAT_BPP4 },
    // { , RK_FORMAT_BPP8 },
    // { , RK_FORMAT_RGBA2BPP },

    { DRM_FORMAT_ABGR2101010, RK_FORMAT_RGBA_1010102 },
    { DRM_FORMAT_ARGB2101010, RK_FORMAT_BGRA_1010102 },
    { DRM_FORMAT_XBGR2101010, RK_FORMAT_RGBX_1010102 },
    { DRM_FORMAT_XRGB2101010, RK_FORMAT_BGRX_1010102 },
    { DRM_FORMAT_RGBA1010102, RK_FORMAT_ABGR_2101010 },
    { DRM_FORMAT_BGRA1010102, RK_FORMAT_ARGB_2101010 },
    { DRM_FORMAT_RGBX1010102, RK_FORMAT_XBGR_2101010 },
    { DRM_FORMAT_BGRX1010102, RK_FORMAT_XRGB_2101010 },

    { DRM_FORMAT_VUY101010, RK_FORMAT_YUV_101010 },

    { DRM_FORMAT_INVALID, RK_FORMAT_UNKNOWN },
};

uint32_t get_format_from_drm_fourcc(uint32_t drm_fourcc) {
#ifdef __cplusplus
    auto entry = drm_fourcc_table.find(drm_fourcc);
    if (entry == drm_fourcc_table.end())
        return RK_FORMAT_UNKNOWN;

    return entry->second;
#else
    int i;

    for (i = 0; i < sizeof(drm_fourcc_table) / sizeof(drm_fourcc_table[0]); i++) {
        if (drm_fourcc_table[i].drm_format == drm_fourcc)
            return drm_fourcc_table[i].rga_format;
    }

    return RK_FORMAT_UNKNOWN;
#endif /* #ifdef __cplusplus */
}

int get_mode_from_drm_modifier(uint64_t modifier) {
    if ((fourcc_mod_is_vendor(modifier, ARM)) &&
        (((modifier >> 52) & 0xf) == DRM_FORMAT_MOD_ARM_TYPE_AFBC)) {
        if ((modifier & AFBC_FORMAT_MOD_BLOCK_SIZE_MASK) == AFBC_FORMAT_MOD_BLOCK_SIZE_16x16)
            return IM_AFBC16x16_MODE;
        if ((modifier & AFBC_FORMAT_MOD_BLOCK_SIZE_MASK) == AFBC_FORMAT_MOD_BLOCK_SIZE_32x8 &&
            modifier & AFBC_FORMAT_MOD_SPLIT)
            return IM_AFBC32x8_MODE;
    } else if (fourcc_mod_is_vendor(modifier, ROCKCHIP)) {
        if (IS_ROCKCHIP_RFBC_MOD(modifier)) {
            if ((modifier & ROCKCHIP_RFBC_BLOCK_SIZE_64x4) == ROCKCHIP_RFBC_BLOCK_SIZE_64x4)
                return IM_RKFBC64x4_MODE;
        } else if (IS_ROCKCHIP_TILED_MOD(modifier)) {
            switch (modifier & ROCKCHIP_TILED_BLOCK_SIZE_MASK) {
                case ROCKCHIP_TILED_BLOCK_SIZE_4x4_MODE0:
                    return IM_TILE4x4_MODE;
                case ROCKCHIP_TILED_BLOCK_SIZE_8x8:
                    return IM_TILE8x8_MODE;
            }
        }
    }

    return IM_RASTER_MODE;
}
#else
uint32_t get_format_from_drm_fourcc(uint32_t drm_fourcc) {
    return RK_FORMAT_UNKNOWN;
}

int get_mode_from_drm_modifier(uint64_t modifier) {
    return IM_RASTER_MODE;
}


#endif /* #ifndef RGA_UTILS_DRM_DISABLE */
