/*
 * Copyright (C) 2024 Rockchip Electronics Co., Ltd.
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

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "librga"
#else
#define LOG_TAG "librga"
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "RockchipRga.h"

#include "im2d_debugger.h"
#include "im2d_impl.h"
#include "im2d_log.h"

const char *string_rd_mode(uint32_t mode) {
    switch (mode) {
        case IM_RASTER_MODE:
            return "raster";
        case IM_FBC_MODE:
            return "afbc16x16";
        case IM_TILE_MODE:
            return "tile8x8";
        default:
            return "unknown";
    }
}

const char *string_color_space(uint32_t mode) {
    switch (mode) {
        case IM_YUV_TO_RGB_BT601_LIMIT:
            return "yuv2rgb-bt.601-limit";
        case IM_YUV_TO_RGB_BT601_FULL:
            return "yuv2rgb-bt.601-full";
        case IM_YUV_TO_RGB_BT709_LIMIT:
            return "yuv2rgb-bt.709-limit";
        case IM_RGB_TO_YUV_BT601_FULL:
            return "rgb2yuv-bt.601-full";
        case IM_RGB_TO_YUV_BT601_LIMIT:
            return "rgb2yuv-bt.601-limit";
        case IM_RGB_TO_YUV_BT709_LIMIT:
            return "rgb2yuv-bt.709-limit";
        case IM_RGB_TO_Y4:
            return "rgb-to-y4";
        case IM_RGB_TO_Y4_DITHER:
            return "rgb-to-y4-dither";
        case IM_RGB_TO_Y1_DITHER:
            return "rgb-to-y1-dither";
        case IM_COLOR_SPACE_DEFAULT:
            return "default";
        case IM_RGB_FULL:
            return "rgb_full";
        case IM_RGB_CLIP:
            return "rga_clip";
        case IM_YUV_BT601_LIMIT_RANGE:
            return "yuv_bt.601-limit";
        case IM_YUV_BT601_FULL_RANGE:
            return "yuv_bt.601-full";
        case IM_YUV_BT709_LIMIT_RANGE:
            return "yuv_bt.709-limit";
        case IM_YUV_BT709_FULL_RANGE:
            return "yuv_bt.709-full";
        default:
            return "unknown";
    }
}

const char *string_blend_mode(uint32_t mode) {
    switch (mode) {
        case IM_ALPHA_BLEND_SRC:
            return "src";
        case IM_ALPHA_BLEND_DST:
            return "dst";
        case IM_ALPHA_BLEND_SRC_OVER:
            return "src-over";
        case IM_ALPHA_BLEND_DST_OVER:
            return "dst-over";
        case IM_ALPHA_BLEND_SRC_IN:
            return "src-in";
        case IM_ALPHA_BLEND_DST_IN:
            return "dst-in";
        case IM_ALPHA_BLEND_SRC_OUT:
            return "src-out";
        case IM_ALPHA_BLEND_DST_OUT:
            return "dst-our";
        case IM_ALPHA_BLEND_SRC_ATOP:
            return "src-atop";
        case IM_ALPHA_BLEND_DST_ATOP:
            return "dst-atop";
        case IM_ALPHA_BLEND_XOR:
            return "xor";
        default:
            return "unknown";
    }
}

const char *string_rotate_mode(uint32_t rotate) {
    switch (rotate) {
        case IM_HAL_TRANSFORM_ROT_90:
            return "90";
        case IM_HAL_TRANSFORM_ROT_180:
            return "180";
        case IM_HAL_TRANSFORM_ROT_270:
            return "270";
        default:
            return "unknown";
    }
}

const char *string_flip_mode(uint32_t flip) {
    switch (flip) {
        case IM_HAL_TRANSFORM_FLIP_H:
            return "horiz";
        case IM_HAL_TRANSFORM_FLIP_V:
            return "verti";
        case IM_HAL_TRANSFORM_FLIP_H_V:
            return "horiz & verti";
        default:
            return "unknown";
    }
}

const char *string_mosaic_mode(uint32_t mode) {
    switch (mode) {
        case IM_MOSAIC_8:
            return "mosaic 8x8";
        case IM_MOSAIC_16:
            return "mosaic 16x16";
        case IM_MOSAIC_32:
            return "mosaic 32x32";
        case IM_MOSAIC_64:
            return "mosaic 64x64";
        case IM_MOSAIC_128:
            return "mosaic 128x128";
        default:
            return "unknown";
    }
}

const char *string_rop_mode(uint32_t mode) {
    switch(mode) {
        case IM_ROP_AND:
            return "and";
        case IM_ROP_OR:
            return "or";
        case IM_ROP_NOT_DST:
            return "not-dst";
        case IM_ROP_NOT_SRC:
            return "not-src";
        case IM_ROP_XOR:
            return "xor";
        case IM_ROP_NOT_XOR:
            return "not-xor";
        default:
            return "unknown";
    }
}

const char *string_colorkey_mode(uint32_t mode) {
    switch (mode) {
        case IM_ALPHA_COLORKEY_NORMAL:
            return "normal";
        case IM_ALPHA_COLORKEY_INVERTED:
            return "inverted";
        default:
            return "unknown";
    }
}

static void rga_dump_channel_info_tabular(int log_level, const char *name,
                                          const im_rect *rect, const rga_buffer_t *image) {
    log_level |= IM_LOG_FORCE;

    IM_LOG(log_level,
           " %8s | %10s(%#4x) | %5d, %5d, %5d, %5d | %5d, %5d, %5d, %5d | %17s(%#4x) | %#10x, %#10x, %#18lx, %#18lx | %20s(%#4x) | %#12x ",
           name, string_rd_mode(image->rd_mode), image->rd_mode,
           rect->x, rect->y, rect->width, rect->height,
           image->width, image->height, image->wstride, image->hstride,
           translate_format_str(image->format), image->format,
           image->handle, image->fd, (unsigned long)image->vir_addr, (unsigned long)image->phy_addr,
           string_color_space(image->color_space_mode), image->color_space_mode,
           image->global_alpha);
}

static void rga_dump_osd_info(int log_level, const im_osd_t *osd_info) {
    IM_LOG(log_level, "\tosd_mode[0x%x]:", osd_info->osd_mode);

    IM_LOG(log_level, "\t\tblock:");
    IM_LOG(log_level, "\t\t\twidth_mode[0x%x], width/witdh_index[0x%x], block_count[%d]\n",
           osd_info->block_parm.width_mode, osd_info->block_parm.width, osd_info->block_parm.block_count);
    IM_LOG(log_level, "\t\t\tbackground_config[0x%x], direction[0x%x], color_mode[0x%x]\n",
           osd_info->block_parm.background_config, osd_info->block_parm.direction, osd_info->block_parm.color_mode);
    IM_LOG(log_level, "\t\t\tnormal_color[0x%x], invert_color[0x%x]\n",
           osd_info->block_parm.normal_color.value, osd_info->block_parm.invert_color.value);

    IM_LOG(log_level, "\t\tinvert_config:");
    IM_LOG(log_level, "\t\t\tchannel[0x%x], flags_mode[0x%x], flages_index[%d] threash[0x%x]",
           osd_info->invert_config.invert_channel, osd_info->invert_config.flags_mode,
           osd_info->invert_config.flags_index, osd_info->invert_config.threash);
    IM_LOG(log_level, "\t\t\tflages: invert[0x%llx], current[0x%llx]",
           (unsigned long long)osd_info->invert_config.invert_flags,
           (unsigned long long)osd_info->invert_config.current_flags);
    IM_LOG(log_level, "\t\t\tinvert_mode[%x]",
           osd_info->invert_config.invert_mode);
    if (osd_info->invert_config.invert_mode == IM_OSD_INVERT_USE_FACTOR)
        IM_LOG(log_level, "\t\t\tfactor[min,max] = alpha[0x%x, 0x%x], yg[0x%x, 0x%x], crb[0x%x, 0x%x]",
               osd_info->invert_config.factor.alpha_min, osd_info->invert_config.factor.alpha_max,
               osd_info->invert_config.factor.yg_min, osd_info->invert_config.factor.yg_max,
               osd_info->invert_config.factor.crb_min, osd_info->invert_config.factor.crb_max);

    IM_LOG(log_level, "\t\tbpp2rgb info:");
    IM_LOG(log_level, "\t\t\tac_swap[0x%x], endian_swap[0x%x], color0[0x%x], color1[0x%x]",
           osd_info->bpp2_info.ac_swap, osd_info->bpp2_info.endian_swap,
           osd_info->bpp2_info.color0.value, osd_info->bpp2_info.color1.value);
}

void rga_dump_image(int log_level,
                    const rga_buffer_t *src, const rga_buffer_t *dst, const rga_buffer_t *pat,
                    const im_rect *srect, const im_rect *drect, const im_rect *prect) {
    IM_LOG(log_level, "----------+------------------+----------------------------+----------------------------+-------------------------+----------------------------------------------------------------+----------------------------+--------------");
    IM_LOG(log_level, " Channel  |    Store Mode    |       Rect[x,y,w,h]        |   Image Info[w,h,ws,hs]    |         Format          |     Handle,         Fd,          Virt Addr,          Phys Addr |        Color Space         | Global Alpha ");
    IM_LOG(log_level, "----------+------------------+----------------------------+----------------------------+-------------------------+----------------------------------------------------------------+----------------------------+--------------");
    //                " src1/pat | afbc32x32(0xff)  | 10000, 10000, 10000, 10000 | 10000, 10000, 10000, 10000 | YCrCb_420SP 10bit(0xff) | 0xffffffff, 0xffffffff, 0xffffffffffffffff, 0xffffffffffffffff | yuv2rgb-bt.601-limit(0xff) | 0xff         "
    rga_dump_channel_info_tabular(log_level, "src", srect, src);
    if (pat != NULL && rga_is_buffer_valid(*pat))
        rga_dump_channel_info_tabular(log_level, "src1/pat", prect, pat);
    rga_dump_channel_info_tabular(log_level, "dst", drect, dst);

    IM_LOG(log_level, "----------+------------------+----------------------------+----------------------------+-------------------------+----------------------------------------------------------------+----------------------------+--------------");
}

void rga_dump_opt(int log_level, const im_opt_t *opt, const int usage) {
    log_level |= IM_LOG_FORCE;

    IM_LOG(log_level, "usage[0x%x]", usage);
    IM_LOG(log_level, "option:");

    IM_LOG(log_level, "\tapi_version[0x%x]", opt->version);
    IM_LOG(log_level, "\tset_core[0x%x], priority[%d]", opt->core, opt->priority);

    if (usage & IM_SYNC)
        IM_LOG(log_level, "\tjob_mode[sync]");
    else if (usage & IM_ASYNC)
        IM_LOG(log_level, "\tjob_mode[aync]");

    if (usage & IM_HAL_TRANSFORM_ROT_MASK)
        IM_LOG(log_level, "\trotate[%s(0x%x)]",
               string_rotate_mode(usage & IM_HAL_TRANSFORM_ROT_MASK),
               usage & IM_HAL_TRANSFORM_ROT_MASK);

    if (usage & IM_HAL_TRANSFORM_FLIP_MASK)
        IM_LOG(log_level, "\tmirror[%s(0x%x)]",
               string_flip_mode(usage & IM_HAL_TRANSFORM_FLIP_MASK),
               usage & IM_HAL_TRANSFORM_FLIP_MASK);

    if (usage & IM_ALPHA_BLEND_MASK)
        IM_LOG(log_level, "\tblend_mode[%s(0x%x)], pre-mul[%s]",
               string_blend_mode(usage & IM_ALPHA_BLEND_MASK), usage & IM_ALPHA_BLEND_MASK,
               (usage & IM_ALPHA_BLEND_PRE_MUL) ? "true" : "false");

    if (usage & IM_COLOR_FILL)
        IM_LOG(log_level, "\tfill_color[0x%x] ", opt->color);

    if (usage & IM_MOSAIC)
        IM_LOG(log_level, "\tmosaic[%s(0x%x)] ", string_mosaic_mode(opt->mosaic_mode), opt->mosaic_mode);

    if (usage & IM_ROP)
        IM_LOG(log_level, "\trop[%s(0x%x)] ", string_rop_mode(opt->rop_code), opt->rop_code);

    if (usage & IM_ALPHA_COLORKEY_MASK) {
        IM_LOG(log_level, "\tcolor_key:");
        IM_LOG(log_level, "\t\tmode[%s(0x%x)], color_range[min,max] = [0x%x, 0x%x] ",
               string_colorkey_mode(usage & IM_ALPHA_COLORKEY_MASK),
               usage & IM_ALPHA_COLORKEY_MASK,
               opt->colorkey_range.min, opt->colorkey_range.max);
    }

    if (usage & IM_NN_QUANTIZE) {
        IM_LOG(log_level, "\tnn:");
        IM_LOG(log_level, "\t\tscale[r,g,b] = [%d, %d, %d], offset[r,g,b] = [0x%x, 0x%x, 0x%x]",
               opt->nn.scale_r, opt->nn.scale_g, opt->nn.scale_b,
               opt->nn.offset_r, opt->nn.offset_g, opt->nn.offset_b);
    }

    if (usage & IM_OSD)
        rga_dump_osd_info(log_level, &opt->osd_config);

    if (usage & IM_PRE_INTR) {
        IM_LOG(log_level, "\tpre_intr:");
        IM_LOG(log_level, "\t\tflags[0x%x], read_threshold[0x%x], write_start[0x%x], write_step[0x%x]",
               opt->intr_config.flags, opt->intr_config.read_threshold,
               opt->intr_config.write_start, opt->intr_config.write_step);
    }
}

void rga_dump_info(int log_level,
                   const im_job_handle_t job_handle,
                   const rga_buffer_t *src, const rga_buffer_t *dst, const rga_buffer_t *pat,
                   const im_rect *srect, const im_rect *drect, const im_rect *prect,
                   const int acquire_fence_fd, const int *release_fence_fd,
                   const im_opt_t *opt_ptr, const int usage) {
    IM_LOG(log_level, "job_handle[%#x], aquire_fence[%d(%#x)], release_fence_ptr[%p]",
           job_handle, acquire_fence_fd, acquire_fence_fd, release_fence_fd);

    rga_dump_image(log_level, src, dst, pat, srect, drect, prect);

    if (opt_ptr != NULL)
        rga_dump_opt(log_level, opt_ptr, usage);
}
