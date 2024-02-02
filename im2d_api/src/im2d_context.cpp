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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#ifdef ANDROID
#include <cutils/properties.h>
#endif

#include "im2d_log.h"
#include "im2d_context.h"
#include "im2d_impl.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "im2d_rga_context"
#else
#define LOG_TAG "im2d_rga_context"
#endif

rga_session_t g_rga_session;

static int get_debug_property(void) {
#ifdef ANDROID
    char level[PROP_VALUE_MAX];
    __system_property_get("vendor.rga.log" ,level);
#else
    char *level = getenv("ROCKCHIP_RGA_LOG");
    if (level == NULL)
        level = (char *)"0";
#endif

    return atoi(level);
}

static void set_driver_feature(rga_session_t *session) {
    if (rga_version_compare(session->driver_verison, (struct rga_version_t){ 1, 3, 0, {0} }) >= 0)
        session->driver_feature |= RGA_DRIVER_FEATURE_USER_CLOSE_FENCE;
}

static inline void clear_driver_feature(rga_session_t *session) {
    session->driver_feature = 0;
}

static int rga_device_init() {
    int ret;
    int fd;
    rga_session_t *session =  &g_rga_session;

    pthread_rwlock_wrlock(&session->rwlock);
    if (session->rga_dev_fd != -1 || session->rga_dev_fd >= 0) {
        pthread_rwlock_unlock(&session->rwlock);
        return 0;
    }

    fd = open(RGA_DEVICE_NODE_PATH, O_RDWR, 0);
    if (fd < 0) {
        IM_LOGE("failed to open %s:%s.", RGA_DEVICE_NODE_PATH, strerror(errno));
        ret = -ENODEV;
        goto unlock;
    }

    ret = ioctl(fd, RGA_IOC_GET_DRVIER_VERSION, &session->driver_verison);
    if (ret >= 0) {
        ret = ioctl(fd, RGA_IOC_GET_HW_VERSION, &session->core_version);
        if (ret < 0) {
            IM_LOGE("librga fail to get hardware versions!\n");
            goto unlock;
        }

        session->driver_type = RGA_DRIVER_IOC_MULTI_RGA;
    } else {
        session->core_version.size = 1;
        ret = ioctl(fd, RGA2_GET_VERSION, session->core_version.version[0].str);
        if (ret < 0) {
            /* Try to get the version of RGA1 */
            ret = ioctl(fd, RGA_GET_VERSION, session->core_version.version[0].str);
            if (ret < 0) {
                IM_LOGE("librga fail to get RGA2/RGA1 version! %s\n", strerror(ret));
                goto unlock;
            }
        }

        sscanf((char *)session->core_version.version[0].str, "%x.%x.%x",
            &session->core_version.version[0].major,
            &session->core_version.version[0].minor,
            &session->core_version.version[0].revision);

        if (session->core_version.version[0].major < 2)
            session->driver_type = RGA_DRIVER_IOC_RGA1;
        else
            session->driver_type = RGA_DRIVER_IOC_RGA2;

        IM_LOGI("Enable compatibility mode, currently adapted to RGA1/RGA2 Device Driver!\n");
    }

    ret = rga_check_driver(session->driver_verison);
    if (ret == IM_STATUS_ERROR_VERSION)
        goto unlock;

    set_driver_feature(session);
    session->rga_dev_fd = fd;
    ret = 0;

unlock:
    pthread_rwlock_unlock(&session->rwlock);

    return ret;
}

static void rga_device_exit(void) {
    rga_session_t *session = &g_rga_session;

    pthread_rwlock_wrlock(&session->rwlock);
    if (session->rga_dev_fd < 0) {
        pthread_rwlock_unlock(&session->rwlock);
        return;
    }

    close(session->rga_dev_fd);
    session->rga_dev_fd = -1;
    session->driver_type = RGA_DRIVER_IOC_UNKONW;

    clear_driver_feature(session);
}

rga_session_t *get_rga_session() {
    int ret;

    if (rga_device_init() == 0)
        return &g_rga_session;
    else
        return NULL;
}

int get_debug_state(void) {
    if (rga_device_init() != 0)
        return false;

    g_rga_session.is_debug = get_debug_property();
    return g_rga_session.is_debug;
}

int is_debug_en(void) {
    return g_rga_session.is_debug;
}

/* Pre-processing during librga load/unload */
__attribute__((constructor)) static void librga_init() {
    if (pthread_rwlock_init(&g_rga_session.rwlock, NULL) != 0) {
        IM_LOGE("im2d API context init failed!\n");
        return;
    }

    g_rga_session.rga_dev_fd = -1;
}

__attribute__((destructor)) static void librga_exit() {
    rga_device_exit();
}
