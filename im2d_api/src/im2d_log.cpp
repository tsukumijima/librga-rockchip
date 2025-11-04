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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>

#ifndef __cplusplus
# include <stdatomic.h>
#else
# include <atomic>
# define _Atomic(X) std::atomic< X >
using namespace std;
#endif

#if (defined(ANDROID) || defined(ANDROID_VNDK))
#include <android/log.h>
#include <sys/system_properties.h>
#endif

#include "im2d_context.h"
#include "im2d_version.h"
#include "im2d_log.h"

static int rga_log_enable_property_get(void);
static int rga_log_level_property_get(void);

RGA_THREAD_LOCAL char g_rga_err_str[IM_ERR_MSG_LEN] = "The current error message is empty!";

#ifdef __cplusplus
static atomic_int g_log_en = ATOMIC_VAR_INIT(rga_log_enable_property_get());
static atomic_int g_log_level = ATOMIC_VAR_INIT(rga_log_level_property_get());
static size_t g_start_time = rga_get_current_time_ms();
#else
static atomic_int g_log_en = 0;
static atomic_int g_log_level = 0;
static size_t g_start_time = 0;

__attribute__((constructor)) static void rga_set_start_time_ms() {
    g_log_en = ATOMIC_VAR_INIT(rga_log_enable_property_get());
    g_log_level = ATOMIC_VAR_INIT(rga_log_level_property_get());
    g_start_time = rga_get_current_time_ms();
}
#endif

const char *rga_get_error_type_str(int type) {
    switch (type & IM_LOG_LEVEL_MASK) {
        case IM_LOG_DEBUG:
            return "D";
        case IM_LOG_INFO:
            return "I";
        case IM_LOG_WARN:
            return "W";
        case IM_LOG_ERROR:
            return "E";
        case IM_LOG_UNKNOWN:
        case IM_LOG_DEFAULT:
        default:
            return "unkonwn";
    }
}

int rga_error_msg_set(const char* format, ...) {
    int ret = 0;
    va_list ap;

    va_start(ap, format);
    ret = vsnprintf(g_rga_err_str, IM_ERR_MSG_LEN, format, ap);
    va_end(ap);

    return ret;
}

static int inline rga_log_enable_property_get(void) {
#if (defined(ANDROID) || defined(ANDROID_VNDK))
    char level[PROP_VALUE_MAX];
    __system_property_get("vendor.rga.log" ,level);
#else
    char *level = getenv("ROCKCHIP_RGA_LOG");
    if (level == NULL)
        level = (char *)"0";
#endif

    return atoi(level);
}

static int inline rga_log_level_property_get(void) {
#if (defined(ANDROID) || defined(ANDROID_VNDK))
    char level[PROP_VALUE_MAX];
    __system_property_get("vendor.rga.log_level" ,level);
#else
    char *level = getenv("ROCKCHIP_RGA_LOG_LEVEL");
    if (level == NULL)
        level = (char *)"0";
#endif

    return atoi(level);
}

void rga_version_update(void) {
#if (defined(ANDROID) || defined(ANDROID_VNDK))
    __system_property_set("vendor.rga_api.version", RGA_API_VERSION);
#else
    setenv("ROCKCHIP_RGA_API_VERSION", RGA_API_VERSION, 1);
#endif
}

int rga_log_enable_update(void) {
    atomic_store(&g_log_en, rga_log_enable_property_get());
    return atomic_load(&g_log_en);
}

int rga_log_level_update(void) {
    atomic_store(&g_log_level, rga_log_level_property_get());
    return atomic_load(&g_log_level);
}

int rga_log_level_get(void) {
    return atomic_load(&g_log_level);
}

int rga_log_enable_get(void) {
    return atomic_load(&g_log_en);
}

size_t rga_get_current_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

size_t rga_get_start_time_ms(void) {
    return g_start_time;
}
