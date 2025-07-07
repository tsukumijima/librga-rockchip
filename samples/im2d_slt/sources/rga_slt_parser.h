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

#ifndef _RGA_SLT_PARSER_H_
#define _RGA_SLT_PARSER_H_

#include <stdbool.h>

#define RGA_SLT_STRING_MAX  256

 extern char g_input_path[RGA_SLT_STRING_MAX];
 extern char g_output_path[RGA_SLT_STRING_MAX];
 extern char g_golden_path[RGA_SLT_STRING_MAX];
 extern char g_golden_prefix[RGA_SLT_STRING_MAX];
 extern bool g_golden_input;
 extern bool g_golden_generate_crc;
 extern struct im2d_slt_config g_chip_config;

 int rga_slt_parse_argv(int argc, char *argv[]);

 #endif /* #ifndef _RGA_SLT_PARSER_H_ */
