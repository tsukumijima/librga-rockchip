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

#ifndef _im2d_context_h_
#define _im2d_context_h_

#include <stdbool.h>
#include <pthread.h>

#include "rga_ioctl.h"
#include "im2d_hardware.h"

#ifdef RT_THREAD
#include "rt-thread/rtt_adapter.h"
#endif

#define RGA_DEVICE_NODE_PATH "/dev/rga"

typedef enum {
    RGA_DRIVER_IOC_UNKONW = 0,
    RGA_DRIVER_IOC_RGA1,
    RGA_DRIVER_IOC_RGA2,
    RGA_DRIVER_IOC_MULTI_RGA,

    RGA_DRIVER_IOC_DEFAULT = RGA_DRIVER_IOC_MULTI_RGA,
} RGA_DRIVER_IOC_TYPE;

typedef enum {
    RGA_DRIVER_FEATURE_USER_CLOSE_FENCE = 1,
} RGA_DRIVER_FEATURE;

typedef struct rga_session {
#ifdef RT_THREAD
    rt_device_t rga_dev_fd;
#else
    int rga_dev_fd;
#endif

    pthread_rwlock_t rwlock;

    bool is_debug;

    struct rga_hw_versions_t core_version;
    struct rga_version_t driver_verison;
    RGA_DRIVER_IOC_TYPE driver_type;
    uint32_t driver_feature;

    rga_info_table_entry hardware_info;
} rga_session_t;

int get_debug_state();
int is_debug_en();

rga_session_t *get_rga_session();

#endif /* #ifndef _im2d_context_h_ */
