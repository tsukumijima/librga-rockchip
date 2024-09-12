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

#ifndef _RGA_IM2D_DEBUGGER_H_
#define _RGA_IM2D_DEBUGGER_H_

#include "im2d_type.h"

const char *string_rd_mode(uint32_t mode);
const char *string_color_space(uint32_t mode);
const char *string_blend_mode(uint32_t mode);
const char *string_rotate_mode(uint32_t rotate);
const char *string_flip_mode(uint32_t flip);
const char *string_mosaic_mode(uint32_t mode);
const char *string_rop_mode(uint32_t mode);
const char *string_colorkey_mode(uint32_t mode);

void rga_dump_image(int log_level,
                    const rga_buffer_t *src, const rga_buffer_t *dst, const rga_buffer_t *pat,
                    const im_rect *srect, const im_rect *drect, const im_rect *prect);
void rga_dump_opt(int log_level, const im_opt_t *opt, const int usage);
void rga_dump_info(int log_level,
                   const im_job_handle_t job_handle,
                   const rga_buffer_t *src, const rga_buffer_t *dst, const rga_buffer_t *pat,
                   const im_rect *srect, const im_rect *drect, const im_rect *prect,
                   const int acquire_fence_fd, const int *release_fence_fd,
                   const im_opt_t *opt_ptr, const int usage);

#endif /* #ifndef _RGA_IM2D_DEBUGGER_H_ */
