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

#ifndef _RGA_IM2D_JOB_H_
#define _RGA_IM2D_JOB_H_

#include <pthread.h>

#include "rga_ioctl.h"
#include "im2d_type.h"
#include "im2d_context.h"

#ifdef __cplusplus
#define IM2D_JOB_USE_MAP true
#else
#define IM2D_JOB_USE_MAP false
#endif

typedef struct im_rga_job {
    struct rga_req req[RGA_TASK_NUM_MAX];
    int task_count;

    int id;
} im_rga_job_t;

#if IM2D_JOB_USE_MAP
#include <map>

typedef std::map<im_job_handle_t, im_rga_job_t *> rga_job_map_t;
#else
typedef struct rga_map_node_data {
    uint64_t key;
    void *value;
} rga_map_node_data_t;

typedef struct rga_map_node rga_map_node_t;

typedef struct rga_map_node {
    rga_map_node_t* prev;
    rga_map_node_t* next;

    rga_map_node_data_t data;
} rga_map_node_t;

typedef struct rga_map_list {
    rga_map_node_t* head;
    rga_map_node_t* tail;
} rga_map_list_t;

typedef rga_map_list_t rga_job_map_t;
#endif

struct im2d_job_manager {
    rga_job_map_t job_map;
    int job_count;

    pthread_mutex_t mutex;
};

void rga_map_insert_job(rga_job_map_t *job_map, im_job_handle_t handle, im_rga_job_t *job);
void rga_map_delete_job(rga_job_map_t *job_map, im_job_handle_t handle);
im_rga_job_t *rga_map_find_job(rga_job_map_t *job_map, im_job_handle_t handle);

extern struct im2d_job_manager g_im2d_job_manager;

#endif /* #ifndef _RGA_IM2D_JOB_H_ */
