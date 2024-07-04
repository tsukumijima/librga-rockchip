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

#include <stdbool.h>

#include "android_utils/android_utils.h"
#include "drm_utils/drm_utils.h"

/*
 * When a pointer is converted to uint64_t, it must first be assigned to
 * an integer of the same size, and then converted to uint64_t. The opposite
 * is true.
 */
#define ptr_to_u64(ptr) ((uint64_t)(uintptr_t)(ptr))
#define u64_to_ptr(var) ((void *)(uintptr_t)(var))
#define PTR_ERR(ptr) ((intptr_t)(ptr))
#define ERR_PTR(err) ((void *)((intptr_t)(err)))
#define IS_ERR(ptr) ((intptr_t)(ptr) < (intptr_t)ERR_PTR(-IM_ERROR_FAILED) && (intptr_t)(ptr) > (intptr_t)ERR_PTR(-IM_ERROR_MAX))
#define IS_ERR_OR_NULL(ptr) ((ptr) == NULL || IS_ERR(ptr))

#define is_rga_format(format) ((format) & 0xff00 || (format) == 0)

bool is_bpp_format(int format);
bool is_yuv_format(int format);
bool is_rgb_format(int format);
bool is_alpha_format(int format);

int convert_to_rga_format(int ex_format);
