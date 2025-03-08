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

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "im2d_job"
#else
#define LOG_TAG "im2d_job"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "im2d_type.h"
#include "im2d_job.h"
#include "im2d_log.h"
#include "im2d_context.h"

struct im2d_job_manager g_im2d_job_manager;

#if !IM2D_JOB_USE_MAP
static void rga_map_insert_head(rga_job_map_t *job_map, rga_map_node_data_t data) {
    rga_map_node_t *node;

    node = (rga_map_node_t *)malloc(sizeof(rga_map_node_t));

    node->data = data;

    node->prev = NULL;
    node->next = job_map->head;

    /* if job_map is empty */
    if (job_map->head == NULL)
        job_map->tail = node;
    else
        job_map->head->prev = node;

    job_map->head = node;
}

static void rga_map_insert_tail(rga_job_map_t *job_map, rga_map_node_data_t data) {
    rga_map_node_t *node;

    node = (rga_map_node_t *)malloc(sizeof(rga_map_node_t));

    node->data = data;

    node->prev = job_map->tail;
    node->next = NULL;

    /* if job_map is empty */
    if (job_map->tail == NULL)
        job_map->head = node;
    else
        job_map->tail->next = node;

    job_map->tail = node;
}

static void rga_map_delete(rga_job_map_t *job_map, rga_map_node_t *node) {
    if (node == NULL)
        return;

    if (node->prev != NULL)
        node->prev->next = node->next;
    else
        job_map->head = node->next;

    if (node->next != NULL)
        node->next->prev = node->prev;
    else
        job_map->tail = node->prev;

    free(node);
}

static rga_map_node_t *rga_map_find(rga_job_map_t *job_map, uint64_t key) {
    rga_map_node_t *curr = job_map->head;
    while (curr != NULL) {
        if (curr->data.key == key)
            return curr;

        curr = curr->next;
    }

    return NULL;
}

static void rga_map_list_deatroy(rga_job_map_t *job_map) {
    while (job_map->head != NULL)
        rga_map_delete(job_map, job_map->head);
}

static void rga_map_list_init(rga_job_map_t *job_map) {
    job_map->head = NULL;
    job_map->tail = NULL;
}
#endif

void rga_map_insert_job(rga_job_map_t *job_map, im_job_handle_t handle, im_rga_job_t *job) {
#if IM2D_JOB_USE_MAP
    if (job_map->count(handle) != 0) {
        IM_LOGE("insert job failed, handle[%d] is exist.", handle);
        return;
    } else {
        job_map->emplace(handle, job);
    }

#else
    rga_map_node_data_t data;
    rga_map_node_t *node;

    data.key = (uint64_t)handle;
    data.value = (void *)job;

    node = rga_map_find(job_map, data.key);
    if (node != NULL) {
        IM_LOGE("insert job failed, handle[%d] is exist.", handle);
        return;
    } else {
        rga_map_insert_tail(job_map, data);
    }
#endif
}

void rga_map_delete_job(rga_job_map_t *job_map, im_job_handle_t handle) {
#if IM2D_JOB_USE_MAP
    job_map->erase(handle);
#else
    rga_map_node_t *node;

    node = rga_map_find(job_map, (uint64_t)handle);
    if (node != NULL)
        rga_map_delete(job_map, node);
#endif
}

im_rga_job_t *rga_map_find_job(rga_job_map_t *job_map, im_job_handle_t handle) {
#if IM2D_JOB_USE_MAP
    if (job_map->count(handle) != 0) {
        auto iterator = job_map->find(handle);
        if (iterator != job_map->end())
            return iterator->second;
    }
#else
    rga_map_node_t *node;

    node = rga_map_find(job_map, (uint64_t)handle);
    if (node != NULL)
        return (im_rga_job_t *)node->data.value;
#endif

    return NULL;
}

__attribute__((constructor)) static void rga_job_manager_init() {
    if (pthread_mutex_init(&g_im2d_job_manager.mutex, NULL) != 0) {
        IM_LOGE("im2d job manager init mutex_lock failed!\n");
        return;
    }

#if !IM2D_JOB_USE_MAP
    rga_map_list_init(&g_im2d_job_manager.job_map);
#endif

    g_im2d_job_manager.job_count = 0;
}

__attribute__((destructor)) static void rga_job_manager_destroy() {
    pthread_mutex_destroy(&g_im2d_job_manager.mutex);
#if !IM2D_JOB_USE_MAP
    rga_map_list_deatroy(&g_im2d_job_manager.job_map);
#endif
}
