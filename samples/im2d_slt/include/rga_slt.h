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

#ifndef _RGA_SLT_H_
#define _RGA_SLT_H_

#ifdef __RT_THREAD__

int rga_slt(int argc, char *argv[]);

static inline int rga_slt_rk1820()
{
    return rga_slt(3, (char *[]){"rga_slt", "-c", "rk1820"});
}

#endif /* #ifdef __RT_THREAD__ */

#endif /* #ifndef _RGA_SLT_H_ */
