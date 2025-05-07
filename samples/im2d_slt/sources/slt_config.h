/*
 * Copyright (C) 2021 Rockchip Electronics Co., Ltd.
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

#ifndef _IM2D_SLT_CONFIG_H_
#define _IM2D_SLT_CONFIG_H_

#include <stdint.h>

#include "im2d.h"
#include "rga.h"

/* dma-heap path */
#define DEFAULT_DMA_HEAP_PATH              "/dev/dma_heap/system-uncached"
#define DEFAULT_DMA32_HEAP_PATH            "/dev/dma_heap/system-uncached-dma32"
#define DEFAULT_CMA_HEAP_PATH              "/dev/dma_heap/cma-uncached"
#define DEFAULT_RK_DMA_HEAP_PATH           "/dev/rk_dma_heap/rk-dma-heap-cma"

/* image path */
#define IM2D_SLT_DEFAULT_INPUT_PATH         "/data/rga_slt"
#define IM2D_SLT_DEFAULT_OUTPUT_PATH        "/data/rga_slt"

/* crc32 golden config */
#define IM2D_SLT_GENERATE_CRC_GOLDEN_PREFIX "crcdata"
#define IM2D_SLT_DEFAULT_GOLDEN_PATH        "/data/rga_slt/golden"

/* im2d_slt config */
#define IM2D_SLT_THREAD_EN                  true    /* Enable multi-threaded mode. */
#define IM2D_SLT_THREAD_MAX                 10      /* Maximum number of threads. */
#define IM2D_SLT_WHILE_NUM                  500     /* Number of while mode. */

#define IM2D_SLT_TEST_PERF_EN               false   /* Enable perf test. */

enum {
    RGA_SLT_FUNC_DIS_ALPHA = 1 << 0,
} RGA_SLT_FUNC_FLAGS;

struct im2d_slt_config {
    int default_width;
    int default_height;
    int default_format;

    bool perf_case_en;
    int while_num;

    bool special_case_en;

    int core_mask;
    int special_mask;
    int func_flags;

    const char *heap_path;
};

static struct im2d_slt_config rk3588_config = {
    .default_width = 1280,
    .default_height = 720,
    .default_format = RK_FORMAT_RGBA_8888,

    .perf_case_en = IM2D_SLT_TEST_PERF_EN,
    .while_num = IM2D_SLT_WHILE_NUM,

    .special_case_en = true,

    .core_mask = IM_SCHEDULER_RGA2_CORE0 | IM_SCHEDULER_RGA3_CORE0 | IM_SCHEDULER_RGA3_CORE1,
    .special_mask = IM_AFBC16x16_MODE | IM_TILE8x8_MODE,
    .func_flags = 0,

    .heap_path = DEFAULT_DMA32_HEAP_PATH,
};

static struct im2d_slt_config rk3576_config = {
    .default_width = 1280,
    .default_height = 720,
    .default_format = RK_FORMAT_RGBA_8888,

    .perf_case_en = IM2D_SLT_TEST_PERF_EN,
    .while_num = IM2D_SLT_WHILE_NUM,

    .special_case_en = true,

    .core_mask = IM_SCHEDULER_RGA2_CORE0 | IM_SCHEDULER_RGA2_CORE1,
    .special_mask = IM_AFBC32x8_MODE | IM_TILE4x4_MODE,
    .func_flags = 0,

    .heap_path = DEFAULT_DMA32_HEAP_PATH,
};

static struct im2d_slt_config common_rga2_config = {
    .default_width = 1280,
    .default_height = 720,
    .default_format = RK_FORMAT_RGBA_8888,

    .perf_case_en = IM2D_SLT_TEST_PERF_EN,
    .while_num = IM2D_SLT_WHILE_NUM,

    .special_case_en = false,

    .core_mask = IM_SCHEDULER_RGA2_CORE0,
    .special_mask = 0,
    .func_flags = 0,

    .heap_path = DEFAULT_DMA32_HEAP_PATH,
};

static struct im2d_slt_config rv1103b_config = {
    .default_width = 1280,
    .default_height = 720,
    .default_format = RK_FORMAT_RGBA_8888,

    .perf_case_en = IM2D_SLT_TEST_PERF_EN,
    .while_num = IM2D_SLT_WHILE_NUM,

    .special_case_en = false,

    .core_mask = IM_SCHEDULER_RGA2_CORE0,
    .special_mask = 0,
    .func_flags = RGA_SLT_FUNC_DIS_ALPHA,

    .heap_path = DEFAULT_RK_DMA_HEAP_PATH,
};

static struct im2d_slt_config rk3506_config = {
    .default_width = 1280,
    .default_height = 720,
    .default_format = RK_FORMAT_ARGB_4444,

    .perf_case_en = IM2D_SLT_TEST_PERF_EN,
    .while_num = IM2D_SLT_WHILE_NUM,

    .special_case_en = false,

    .core_mask = IM_SCHEDULER_RGA2_CORE0,
    .special_mask = 0,
    .func_flags = RGA_SLT_FUNC_DIS_ALPHA,

    .heap_path = 0,
};

#endif /* #ifndef _IM2D_SLT_CONFIG_H_ */
