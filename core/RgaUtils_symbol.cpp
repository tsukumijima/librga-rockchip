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

/* For C-style symbols */
#ifdef __cplusplus

extern float get_bpp_from_format_impl(int format);
extern int get_perPixel_stride_from_format_impl(int format);
extern int get_buf_from_file_impl(void *buf, int f, int sw, int sh, int index);
extern int output_buf_data_to_file_impl(void *buf, int f, int sw, int sh, int index);
extern const char *translate_format_str_impl(int format);
extern int get_buf_from_file_FBC_impl(void *buf, int f, int sw, int sh, int index);
extern int output_buf_data_to_file_FBC_impl(void *buf, int f, int sw, int sh, int index);

extern "C" {
float get_bpp_from_format(int format) {
    return get_bpp_from_format_impl(format);
}

int get_perPixel_stride_from_format(int format) {
    return get_perPixel_stride_from_format_impl(format);
}

int get_buf_from_file(void *buf, int f, int sw, int sh, int index) {
    return get_buf_from_file_impl(buf, f, sw, sh, index);
}

int output_buf_data_to_file(void *buf, int f, int sw, int sh, int index) {
    return output_buf_data_to_file_impl(buf, f, sw, sh, index);
}

const char *translate_format_str(int format) {
    return translate_format_str_impl(format);
}

int get_buf_from_file_FBC(void *buf, int f, int sw, int sh, int index) {
    return get_buf_from_file_FBC_impl(buf, f, sw, sh, index);
}

int output_buf_data_to_file_FBC(void *buf, int f, int sw, int sh, int index) {
    return output_buf_data_to_file_FBC_impl(buf, f, sw, sh, index);
}

} /* extern "C" */
#endif

