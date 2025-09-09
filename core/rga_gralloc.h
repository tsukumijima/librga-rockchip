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

#ifndef __rga_gralloc_h__
#define __rga_gralloc_h__

#ifdef ANDROID

#include <inttypes.h>
#include <cutils/native_handle.h>

int rga_gralloc_get_handle_fd(buffer_handle_t handle);
int rga_gralloc_get_handle_width(buffer_handle_t handle);
int rga_gralloc_get_handle_height(buffer_handle_t handle);
int rga_gralloc_get_handle_stride(buffer_handle_t handle);
int rga_gralloc_get_handle_height_stride(buffer_handle_t handle);
int rga_gralloc_get_handle_format(buffer_handle_t handle);
int rga_gralloc_get_handle_size(buffer_handle_t handle);
uint32_t rga_gralloc_get_handle_drm_fourcc(buffer_handle_t handle);
uint64_t rga_gralloc_get_handle_drm_modifier(buffer_handle_t handle);
void *rga_gralloc_get_handle_virtual_addr(buffer_handle_t handle);

#endif /* #ifdef ANDROID */

#endif /* #ifndef __rga_gralloc_h__ */
