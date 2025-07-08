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

#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "rga_im2d_slt"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <memory.h>
#include <pthread.h>

#ifdef __RT_THREAD__
#include <rtdevice.h>
#include <rtthread.h>
#include <rthw.h>
#else
#include "dma_alloc.h"
#endif

#include "utils.h"

#include "RgaUtils.h"
#ifdef __cplusplus
#include "im2d.hpp"
#else
#include "im2d.h"
#endif

#include "slt_config.h"
#include "rga_slt_parser.h"
#include "rga_slt_crc.h"

enum {
    FILL_BUFF  = 0,
    EMPTY_BUFF = 1
};

enum {
    INVALID_CACHE,
    FLUSH_CACHE,
};

enum ERR_NUM {
    slt_error           = -1,
    slt_check_error     = -2,
    slt_rga_error       = -3,
};

struct rga_image_info {
    rga_buffer_t img;
    char *buf;
    int fd;
    int buf_size;
};

typedef struct private_data {
    int id;
    const char *name;
    const char *dma_heap_name;
    int mode;
    unsigned int num;

    int width;
    int height;
    int format;

    int rd_mode;
    int core;
    int priority;

    int result;
#ifdef __RT_THREAD__
    rt_sem_t sem;
#endif
} private_data_t;

typedef int (*rga_slt_case) (private_data_t *, int, struct rga_image_info, struct rga_image_info, struct rga_image_info);
#ifdef __RT_THREAD__
typedef void (thread_func_t)(void *);
#define THREAD_FUNC_RETURN_TYPE void
#else
typedef void *(*thread_func_t)(void *);
#define THREAD_FUNC_RETURN_TYPE void *
#endif

static int file_exists(const char* file_name) {
    FILE* file = fopen(file_name, "r");
    if (file != NULL) {
        fclose(file);
        return true;
    }

    return false;
}

static void rga_sync_cache(struct rga_image_info *img, int ops) {
    switch (ops) {
        case INVALID_CACHE:
#ifdef __RT_THREAD__
            if (img->buf)
                rt_hw_cpu_dcache_ops(RT_HW_CACHE_INVALIDATE, (void *)img->buf, img->buf_size);
#else
            if (img->fd)
                dma_sync_device_to_cpu(img->fd);
#endif
            break;

        case FLUSH_CACHE:
#ifdef __RT_THREAD__
            if (img->buf)
                rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, (void *)img->buf, img->buf_size);
#else
            if (img->fd)
                dma_sync_cpu_to_device(img->fd);
#endif
            break;

        default:
            printf("Invalid cache operation: %d\n", ops);
            return;
    }
}

int rga_raster_test(private_data_t *data, int time,
                    struct rga_image_info src_img,
                    struct rga_image_info tmp_img,
                    struct rga_image_info dst_img) {
    int ret;
    int case_index;
    int usage = 0;
    int ori_format;
    unsigned int result_crc = 0;
    const rga_slt_crc_table *crc_golden_table = NULL;

    rga_buffer_t src, dst, tmp;
    char *src_buf, *dst_buf, *tmp_buf;
    int src_buf_size, dst_buf_size, tmp_buf_size;
    im_rect src_rect, tmp_rect, dst_rect;

    memset(&src_rect, 0, sizeof(src_rect));
    memset(&tmp_rect, 0, sizeof(tmp_rect));
    memset(&dst_rect, 0, sizeof(dst_rect));

    src = src_img.img;
    src_buf = src_img.buf;
    src_buf_size = src_img.buf_size;

    tmp = tmp_img.img;
    tmp_buf = tmp_img.buf;
    tmp_buf_size = tmp_img.buf_size;

    dst = dst_img.img;
    dst_buf = dst_img.buf;
    dst_buf_size = dst_img.buf_size;

    if (!g_golden_generate_crc) {
        crc_golden_table = get_crcdata_table();
        if (crc_golden_table == NULL) {
            printf("cannot read crc golden table!\n");
            return slt_error;
        }
    }

    {
        /* case: bypass + src-CSC */
        case_index = 0;

        ret = imcvtcolor(src, dst, RK_FORMAT_YCbCr_420_SP, dst.format);
        if (ret != IM_STATUS_SUCCESS) {
            printf("ID[%d]: %s bypass + src-CSC %d time running failed! %s\n", data->id, data->name, time, imStrError(ret));
            return slt_rga_error;
        }

        rga_sync_cache(&dst_img, INVALID_CACHE);

        result_crc = crc32(0xffffffff, (unsigned char *)dst_buf, dst_buf_size);
        if(g_golden_generate_crc) {
            save_crcdata(result_crc, data->id, case_index);
        } else {
            if (!crc_check(data->id, case_index, result_crc, crc_golden_table))
                goto CHECK_ERROR;
        }

        if (!(g_chip_config.func_flags & RGA_SLT_FUNC_DIS_ALPHA)) {
            /* case: 3-channel blend + rotate-180 + H_V mirror + scale-up + dst-CSC */
            case_index++;

            src_rect.x = 100;
            src_rect.y = 100;
            src_rect.width = 480;
            src_rect.height = 320;

            dst_rect.x = 100;
            dst_rect.y = 100;
            dst_rect.width = 720;
            dst_rect.height = 540;

            ori_format = dst.format;
            dst.format = RK_FORMAT_YCbCr_420_SP;

            usage = IM_SYNC | IM_ALPHA_BLEND_SRC_OVER | IM_ALPHA_BLEND_PRE_MUL | IM_HAL_TRANSFORM_ROT_180 | IM_HAL_TRANSFORM_FLIP_H_V;

            ret = improcess(src, dst, tmp, src_rect, dst_rect, dst_rect, usage);
            if (ret != IM_STATUS_SUCCESS) {
                printf("ID[%d]: %s 3-channel blend + rotate-180 + H_V mirror + scale-up + dst-CSC %d time running failed! %s\n", data->id, data->name, time, imStrError(ret));
                return slt_rga_error;
            }

            rga_sync_cache(&dst_img, INVALID_CACHE);

            result_crc = crc32(0xffffffff, (unsigned char *)dst_buf, dst_buf_size);
            if(g_golden_generate_crc) {
                save_crcdata(result_crc, data->id, case_index);
            } else {
                if (!crc_check(data->id, case_index, result_crc, crc_golden_table))
                    goto CHECK_ERROR;
            }

            dst.format = ori_format;
        }

        /* case: rotate-90 + H_V mirror + scale-down */
        case_index++;

        dst_rect.x = 100;
        dst_rect.y = 100;
        dst_rect.width = 480;
        dst_rect.height = 320;

        usage = IM_SYNC | IM_HAL_TRANSFORM_ROT_90 | IM_HAL_TRANSFORM_FLIP_H_V;

        ret = improcess(src, dst, (rga_buffer_t){}, (im_rect){}, dst_rect, (im_rect){}, usage);
        if (ret != IM_STATUS_SUCCESS) {
            printf("ID[%d]: %s rotate-90 + H_V mirror + scale-down %d time running failed! %s\n", data->id, data->name, time, imStrError(ret));
            return slt_rga_error;
        }

        rga_sync_cache(&dst_img, INVALID_CACHE);

        result_crc = crc32(0xffffffff, (unsigned char *)dst_buf, dst_buf_size);
        if(g_golden_generate_crc) {
            save_crcdata(result_crc, data->id, case_index);
        } else {
            if (!crc_check(data->id, case_index, result_crc, crc_golden_table))
                goto CHECK_ERROR;
        }

        if (data->core == IM_SCHEDULER_RGA2_CORE0 ||
            data->core == IM_SCHEDULER_RGA2_CORE1) {
            /* case: color fill */
            case_index++;

            dst_rect.x = 100;
            dst_rect.y = 100;
            dst_rect.width = 720;
            dst_rect.height = 540;

            ret = imfill(dst, dst_rect, 0xffaabbcc);
            if (ret != IM_STATUS_SUCCESS) {
                printf("ID[%d]: %s fill %d time running failed! %s\n", data->id, data->name, time, imStrError(ret));
                return slt_rga_error;
            }

            rga_sync_cache(&dst_img, INVALID_CACHE);

            result_crc = crc32(0xffffffff, (unsigned char *)dst_buf, dst_buf_size);
            if(g_golden_generate_crc) {
                save_crcdata(result_crc, data->id, case_index);
            } else {
                if (!crc_check(data->id, case_index, result_crc, crc_golden_table))
                    goto CHECK_ERROR;
            }
        }
    }

    return 0;

CHECK_ERROR:
    printf("ID[%d] loop[%d]: %s case[%d] check-CRC failed! result = %#x, golden = %#x\n",
           data->id, time, data->name, case_index,
           result_crc, crc_golden_table ? (*crc_golden_table)[data->id][case_index] : 0);

    return slt_check_error;
}

int rga_special_test(private_data_t *data, int time,
                     struct rga_image_info src_img,
                     struct rga_image_info tmp_img,
                     struct rga_image_info dst_img) {
    int ret;
    int case_index;
    unsigned int result_crc = 0;
    const rga_slt_crc_table *crc_golden_table = NULL;

    rga_buffer_t src, dst, tmp;
    char *src_buf, *dst_buf, *tmp_buf;
    int src_buf_size, dst_buf_size, tmp_buf_size;
    im_rect src_rect, tmp_rect, dst_rect;

    memset(&src_rect, 0, sizeof(src_rect));
    memset(&tmp_rect, 0, sizeof(tmp_rect));
    memset(&dst_rect, 0, sizeof(dst_rect));

    src = src_img.img;
    src_buf = src_img.buf;
    src_buf_size = src_img.buf_size;

    tmp = tmp_img.img;
    tmp_buf = tmp_img.buf;
    tmp_buf_size = tmp_img.buf_size;

    dst = dst_img.img;
    dst_buf = dst_img.buf;
    dst_buf_size = dst_img.buf_size;

    if (!g_golden_generate_crc) {
        crc_golden_table = get_crcdata_table();
        if (crc_golden_table == NULL) {
            printf("cannot read crc golden table!\n");
            return slt_error;
        }
    }

    {
        /* case: in */
        case_index = 0;

        ret = imcopy(src, tmp);
        if (ret != IM_STATUS_SUCCESS) {
            printf("ID[%d]: %s input %d time running failed! %s\n", data->id, data->name, time, imStrError(ret));
            return slt_rga_error;
        }

        rga_sync_cache(&dst_img, INVALID_CACHE);

        result_crc = crc32(0xffffffff, (unsigned char *)dst_buf, dst_buf_size);
        if(g_golden_generate_crc) {
            save_crcdata(result_crc, data->id, case_index);
        } else {
            if (!crc_check(data->id, case_index, result_crc, crc_golden_table))
                goto CHECK_ERROR;
        }

        /* case: out */
        if (!(data->rd_mode == IM_AFBC32x8_MODE ||
                data->rd_mode == IM_RKFBC64x4_MODE)) {
            case_index++;

            ret = imcopy(tmp, dst);
            if (ret != IM_STATUS_SUCCESS) {
                printf("ID[%d]: %s output %d time running failed! %s\n", data->id, data->name, time, imStrError(ret));
                return slt_rga_error;
            }

            rga_sync_cache(&dst_img, INVALID_CACHE);

            result_crc = crc32(0xffffffff, (unsigned char *)dst_buf, dst_buf_size);
            if(g_golden_generate_crc) {
                save_crcdata(result_crc, data->id, case_index);
            } else {
                if (!crc_check(data->id, case_index, result_crc, crc_golden_table))
                    goto CHECK_ERROR;
            }
        }
    }

    return 0;

CHECK_ERROR:
    printf("ID[%d] loop[%d]: %s case[%d] check-CRC failed! result = %#x, golden = %#x\n",
           data->id, time, data->name, case_index,
           result_crc, crc_golden_table ? (*crc_golden_table)[data->id][case_index] : 0);

    return slt_check_error;
}

int rga_perf_test(private_data_t *data, int time,
                  struct rga_image_info src_img,
                  struct rga_image_info tmp_img,
                  struct rga_image_info dst_img) {
    int ret;

    {
        ret = imcopy(src_img.img, dst_img.img);
        if (ret != IM_STATUS_SUCCESS) {
            printf("ID[%d]: %s input %d time running failed! %s\n", data->id, data->name, time, imStrError(ret));
            return slt_rga_error;
        }
    }

    return 0;
}

/******************************************************************************/
static int rga_run(void *args, rga_slt_case running_case) {
    int ret = 0, time = 0;
    int use_dma_heap;
    int fbc_en = false;
    bool tile4x4_en = false;
    unsigned int num;
    int src_width, src_height, src_format;
    int dst_width, dst_height, dst_format;

    int src_buf_size, dst_buf_size, tmp_buf_size;
    char *src_buf = NULL, *dst_buf = NULL, *tmp_buf = NULL;
    int src_dma_fd = -1, dst_dma_fd = -1, tmp_dma_fd = -1;
    rga_buffer_handle_t src_handle = 0, dst_handle = 0, tmp_handle = 0;
    im_handle_param_t src_param, dst_param;

    rga_buffer_t src;
    rga_buffer_t tmp;
    rga_buffer_t dst;
    im_rect src_rect;
    im_rect dst_rect;

    struct rga_image_info src_img;
    struct rga_image_info tmp_img;
    struct rga_image_info dst_img;

    private_data_t *data = (private_data_t *)args;

    num = data->num;

    src_width = data->width;
    src_height = data->height;
    src_format = data->format;

    dst_width = data->width;
    dst_height = data->height;
    dst_format = data->format;

    memset(&src, 0x0, sizeof(src));
    memset(&dst, 0x0, sizeof(dst));
    memset(&tmp, 0x0, sizeof(tmp));
    memset(&src_rect, 0x0, sizeof(src_rect));
    memset(&dst_rect, 0x0, sizeof(dst_rect));

    if (data->rd_mode == IM_AFBC16x16_MODE ||
        data->rd_mode == IM_AFBC32x8_MODE ||
        data->rd_mode == IM_RKFBC64x4_MODE)
        fbc_en = true;

    if (data->rd_mode == IM_TILE4x4_MODE)
        tile4x4_en = true;

    src_buf_size = src_width * src_height * get_bpp_from_format(src_format);
    dst_buf_size = dst_width * dst_height * get_bpp_from_format(dst_format);

    src_param = (im_handle_param_t){(uint32_t)src_width, (uint32_t)src_height, (uint32_t)src_format};
    dst_param = (im_handle_param_t){(uint32_t)dst_width, (uint32_t)dst_height, (uint32_t)dst_format};
    if (fbc_en) {
        src_buf_size = src_buf_size * 1.5;
        dst_buf_size = dst_buf_size * 1.5;

        src_param = (im_handle_param_t){(uint32_t)src_width, (uint32_t)(int)(src_height * 1.5), (uint32_t)src_format};
        dst_param = (im_handle_param_t){(uint32_t)dst_width, (uint32_t)(int)(dst_height * 1.5), (uint32_t)dst_format};
    }
    tmp_buf_size = src_buf_size;

#ifdef __RT_THREAD__
    src_buf = (char *)rt_malloc(src_buf_size);
    tmp_buf = (char *)rt_malloc(tmp_buf_size);
    dst_buf = (char *)rt_malloc(dst_buf_size);
    if (src_buf == NULL || tmp_buf == NULL || dst_buf == NULL) {
        printf("malloc fault!\n");
        ret = slt_error;
        goto RELEASE_BUFFER;
    }
    src = wrapbuffer_physicaladdr(src_buf, src_width, src_height, src_format);
    tmp = wrapbuffer_physicaladdr(tmp_buf, src_width, src_height, src_format);
    dst = wrapbuffer_physicaladdr(dst_buf, dst_width, dst_height, dst_format);
    if (src.width == 0 || tmp.width == 0 || dst.width == 0) {
        printf("warpbuffer failed, %s\n", imStrError());
        ret = slt_error;
        goto RELEASE_BUFFER;
    }
#else
    use_dma_heap = (data->dma_heap_name != NULL) && file_exists(data->dma_heap_name);
    if (use_dma_heap) {
        ret = dma_buf_alloc(data->dma_heap_name, src_buf_size, &src_dma_fd, (void **)&src_buf);
        if (ret < 0) {
            printf("alloc src dma_heap buffer failed!\n");
            goto RELEASE_BUFFER;
        }

        ret = dma_buf_alloc(data->dma_heap_name, tmp_buf_size, &tmp_dma_fd, (void **)&tmp_buf);
        if (ret < 0) {
            printf("alloc tmp dma_heap buffer failed!\n");
            goto RELEASE_BUFFER;
        }

        ret = dma_buf_alloc(data->dma_heap_name, dst_buf_size, &dst_dma_fd, (void **)&dst_buf);
        if (ret < 0) {
            printf("alloc dst dma_heap buffer failed!\n");
            goto RELEASE_BUFFER;
        }

        src_handle = importbuffer_fd(src_dma_fd, &src_param);
        if (src_handle <= 0) {
            printf("ID[%d] %s import src dma_buf failed!\n", data->id, data->name);
            ret = slt_error;
            goto RELEASE_BUFFER;
        }

        tmp_handle = importbuffer_fd(tmp_dma_fd, &src_param);
        if (tmp_handle <= 0) {
            printf("ID[%d] %s import tmp dma_buf failed!\n", data->id, data->name);
            ret = slt_error;
            goto RELEASE_BUFFER;
        }

        dst_handle = importbuffer_fd(dst_dma_fd, &dst_param);
        if (dst_handle <= 0) {
            printf("ID[%d] %s import dst dma_buf failed!\n", data->id, data->name);
            ret = slt_error;
            goto RELEASE_BUFFER;
        }
    } else {
        src_buf = (char *)malloc(src_buf_size);
        tmp_buf = (char *)malloc(tmp_buf_size);
        dst_buf = (char *)malloc(dst_buf_size);
        if (src_buf == NULL || tmp_buf == NULL || dst_buf == NULL) {
            printf("malloc fault!\n");
            ret = slt_error;
            goto RELEASE_BUFFER;
        }

        src_handle = importbuffer_virtualaddr(src_buf, &src_param);
        if (src_handle <= 0) {
            printf("ID[%d] %s import src virt_addr failed!\n", data->id, data->name);
            ret = slt_error;
            goto RELEASE_BUFFER;
        }

        tmp_handle = importbuffer_virtualaddr(tmp_buf, &src_param);
        if (tmp_handle <= 0) {
            printf("ID[%d] %s import tmp virt_addr failed!\n", data->id, data->name);
            ret = slt_error;
            goto RELEASE_BUFFER;
        }

        dst_handle = importbuffer_virtualaddr(dst_buf, &dst_param);
        if (dst_handle <= 0) {
            printf("ID[%d] %s import dst virt_addr failed!\n", data->id, data->name);
            ret = slt_error;
            goto RELEASE_BUFFER;
        }
    }

    src = wrapbuffer_handle(src_handle, src_width, src_height, src_format);
    tmp = wrapbuffer_handle(tmp_handle, src_width, src_height, src_format);
    dst = wrapbuffer_handle(dst_handle, dst_width, dst_height, dst_format);
    if (src.width == 0 || tmp.width == 0 || dst.width == 0) {
        printf("warpbuffer failed, %s\n", imStrError());
        ret = slt_error;
        goto RELEASE_BUFFER;
    }
#endif /* ifdef __RT_THREAD__ */

    src_img.img = src;
    src_img.buf = src_buf;
    src_img.fd = src_dma_fd;
    src_img.buf_size = src_buf_size;

    tmp_img.img = tmp;
    tmp_img.buf = tmp_buf;
    tmp_img.fd = tmp_dma_fd;
    tmp_img.buf_size = tmp_buf_size;

    dst_img.img = dst;
    dst_img.buf = dst_buf;
    dst_img.fd = dst_dma_fd;
    dst_img.buf_size = dst_buf_size;

#ifdef __RT_THREAD__
    draw_image(src_buf, src_width, src_height, src_format);
#else
    if (fbc_en) {
        ret = read_image_from_fbc_file(src_buf, g_input_path,
                                       src_width, src_height, src_format, 0);
        if (ret < 0)
            goto RELEASE_BUFFER;
    } else {
        ret = read_image_from_file(src_buf, g_input_path,
                                   src_width, src_height, src_format, 0);
        if (ret < 0)
            goto RELEASE_BUFFER;
    }
#endif /* ifdef __RT_THREAD__ */
    memset(tmp_buf, 0x22, tmp_buf_size);
    memset(dst_buf, 0x33, dst_buf_size);

    rga_sync_cache(&src_img, FLUSH_CACHE);
    rga_sync_cache(&tmp_img, FLUSH_CACHE);
    rga_sync_cache(&dst_img, FLUSH_CACHE);

    src.rd_mode = data->rd_mode;
    dst.rd_mode = data->rd_mode;

    if (data->core != IM_SCHEDULER_DEFAULT)
        imconfig(IM_CONFIG_SCHEDULER_CORE, data->core);
    imconfig(IM_CONFIG_PRIORITY, data->priority);

    do {
        time++;

        ret = running_case(data, time,
                           src_img, tmp_img, dst_img);
        switch (ret) {
            case slt_error:
                goto RELEASE_BUFFER;
            case slt_check_error:
                goto CHECK_FAILED;
            case slt_rga_error:
                goto RUNNING_FAILED;
        }
    } while (data->mode && --num);

    printf("ID[%d]: %s running success!\n", data->id, data->name);

    ret = 0;
    goto RELEASE_BUFFER;

CHECK_FAILED:
RUNNING_FAILED:
    printf("src: %#x %#x %#x %#x\n", (int)src_buf[0], (int)src_buf[1], (int)src_buf[2], (int)src_buf[3]);
    printf("tmp: %#x %#x %#x %#x\n", (int)tmp_buf[0], (int)tmp_buf[1], (int)tmp_buf[2], (int)tmp_buf[3]);
    printf("dst: %#x %#x %#x %#x\n", (int)dst_buf[0], (int)dst_buf[1], (int)dst_buf[2], (int)dst_buf[3]);

#ifdef __RT_THREAD__
RELEASE_BUFFER:
    if (src_buf != NULL)
        rt_free(src_buf);
    if (tmp_buf != NULL)
        rt_free(tmp_buf);
    if (dst_buf != NULL)
        rt_free(dst_buf);
#else
    rga_sync_cache(&src_img, INVALID_CACHE);
    rga_sync_cache(&tmp_img, INVALID_CACHE);
    rga_sync_cache(&dst_img, INVALID_CACHE);

    if (fbc_en) {
        write_image_to_fbc_file(src_buf, g_output_path,
                                src.wstride, src.hstride, src.format, data->id * 10 + 1);
        write_image_to_file(tmp_buf, g_output_path,
                            tmp.wstride, tmp.hstride, tmp.format, data->id * 10 + 2);
        write_image_to_fbc_file(dst_buf, g_output_path,
                                dst.wstride, dst.hstride, dst.format, data->id * 10 + 3);
    } else {
        write_image_to_file(src_buf, g_output_path,
                            src.wstride, src.hstride, src.format, data->id * 10 + 1);
        write_image_to_file(tmp_buf, g_output_path,
                            tmp.wstride, tmp.hstride, tmp.format, data->id * 10 + 2);
        write_image_to_file(dst_buf, g_output_path,
                            dst.wstride, dst.hstride, dst.format, data->id * 10 + 3);
    }

RELEASE_BUFFER:
    if (src_handle)
        releasebuffer_handle(src_handle);
    if (tmp_handle)
        releasebuffer_handle(tmp_handle);
    if (dst_handle)
        releasebuffer_handle(dst_handle);

    if (use_dma_heap) {
        if (src_dma_fd > 0 && src_buf != NULL)
            dma_buf_free(src_buf_size, &src_dma_fd, (void *)src_buf);
        if (tmp_dma_fd > 0 && tmp_buf != NULL)
            dma_buf_free(tmp_buf_size, &tmp_dma_fd, (void *)tmp_buf);
        if (dst_dma_fd > 0 && dst_buf != NULL)
            dma_buf_free(dst_buf_size, &dst_dma_fd, (void *)dst_buf);
    } else {
        if (src_buf != NULL)
            free(src_buf);
        if (tmp_buf != NULL)
            free(tmp_buf);
        if (dst_buf != NULL)
            free(dst_buf);
    }
#endif

    return ret;
}

THREAD_FUNC_RETURN_TYPE pthread_rga_raster_func(void *args)
{
    private_data_t *data = (private_data_t *)args;

    data->result = rga_run(args, rga_raster_test);

#if IM2D_SLT_THREAD_EN
#ifdef __RT_THREAD__
    rt_sem_release(data->sem);
#else
    pthread_exit(NULL);
#endif /* #ifdef __RT_THREAD__ */
#else
    return NULL;
#endif /* #if IM2D_SLT_THREAD_EN */
}

THREAD_FUNC_RETURN_TYPE pthread_rga_special_func(void *args) {
    private_data_t *data = (private_data_t *)args;

    data->result = rga_run(args, rga_special_test);

#if IM2D_SLT_THREAD_EN
#ifdef __RT_THREAD__
    rt_sem_release(data->sem);
#else
    pthread_exit(NULL);
#endif /* #ifdef __RT_THREAD__ */
#else
    return NULL;
#endif /* #if IM2D_SLT_THREAD_EN */
}

THREAD_FUNC_RETURN_TYPE pthread_rga_perf_func(void *args) {
    private_data_t *data = (private_data_t *)args;

    data->result = rga_run(args, rga_perf_test);

#if IM2D_SLT_THREAD_EN
#ifdef __RT_THREAD__
    rt_sem_release(data->sem);
#else
    pthread_exit(NULL);
#endif /* #ifdef __RT_THREAD__ */
#else
    return NULL;
#endif /* #if IM2D_SLT_THREAD_EN */
}

static int run_test(int start, int end, private_data_t *data, thread_func_t test_func) {
#if IM2D_SLT_THREAD_EN
#ifdef __RT_THREAD__
    rt_thread_t tid[IM2D_SLT_THREAD_MAX];
    rt_sem_t all_done_sem;

    all_done_sem = rt_sem_create("done_sem", 0, RT_IPC_FLAG_FIFO);
    if (all_done_sem == RT_NULL) {
        rt_kprintf("Failed to create semaphore.\n");
        return -1;
    }

    for (int i = start; i < end; i++) {
        data[i].sem = all_done_sem;
        tid[i] = rt_thread_create(data[i].name, test_func, (void *)(&data[i]), 163840, 16, 10);
        if (tid[i]) {
            rt_thread_startup(tid[i]);
        } else {
            rt_kprintf("Failed to create thread %s\n", data[i].name);
            return -1;
        }
    }

    for (int i = start; i < end; i++) {
        rt_err_t result = rt_sem_take(all_done_sem, RT_WAITING_FOREVER);
        if (result != RT_EOK) {
            rt_kprintf("rt_sem_take failed (%d/%d).\n", i, end);
            return -1;
        }

        if (data[i].result < 0) {
            rt_kprintf("ID[%d] case '%s' is faile!\n", data[i].id, data[i].name);
            return -1;
        }
    }

    rt_sem_delete(all_done_sem);
#else
    pthread_t tdSyncID[IM2D_SLT_THREAD_MAX];

    for (int i = start; i < end; i++) {
        pthread_create(&tdSyncID[i], NULL, test_func, (void *)(&data[i]));
        printf("creat Sync pthread[0x%lx] = %d, id = %d\n", tdSyncID[i], i, data[i].id);
    }

    for (int i = start; i < end; i++) {
        pthread_join(tdSyncID[i], NULL);
        if (data[i].result < 0) {
            printf("ID[%d] case '%s' is faile!\n", data[i].id, data[i].name);
            return -1;
        }
    }
#endif /* #ifdef __RT_THREAD__ */
#else
    for (int i = start; i < end; i++) {
        test_func((void *)(&data[i]));
        printf("ID[%d] %s run end!\n", data[i].id, data[i].name);
        if (data[i].result < 0) {
            printf("ID[%d] case '%s' is faile!\n", data[i].id, data[i].name);
            return -1;
        }
    }
#endif /* #if IM2D_SLT_THREAD_EN */

    return 0;
}

#ifdef __RT_THREAD__
int rga_slt_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    int start_id = 0;
    int pthread_num = 0;
    private_data_t data[IM2D_SLT_THREAD_MAX];

    init_crc_table();

    if (rga_slt_parse_argv(argc, argv) < 0) {
        return 0;
    }

    memset(&data, 0x0, sizeof(private_data_t) * IM2D_SLT_THREAD_MAX);
    printf("-------------------------------------------------\n");

    start_id = pthread_num;

    if (g_chip_config.core_mask & IM_SCHEDULER_RGA3_CORE0) {
        data[pthread_num].id = pthread_num;
        data[pthread_num].name = "RGA3_core0";
        data[pthread_num].dma_heap_name = g_chip_config.heap_path;
        data[pthread_num].mode = false;
        data[pthread_num].num = 0;
        data[pthread_num].width = g_chip_config.default_width;
        data[pthread_num].height = g_chip_config.default_height;
        data[pthread_num].format = g_chip_config.default_format;
        data[pthread_num].rd_mode = IM_RASTER_MODE;
        data[pthread_num].core = IM_SCHEDULER_RGA3_CORE0;
        data[pthread_num].priority = 1;
        pthread_num++;
    }

    if (g_chip_config.core_mask & IM_SCHEDULER_RGA3_CORE0) {
        data[pthread_num].id = pthread_num;
        data[pthread_num].name = "RGA3_core1";
        data[pthread_num].dma_heap_name = g_chip_config.heap_path;
        data[pthread_num].mode = false;
        data[pthread_num].num = 0;
        data[pthread_num].width = g_chip_config.default_width;
        data[pthread_num].height = g_chip_config.default_height;
        data[pthread_num].format = g_chip_config.default_format;
        data[pthread_num].rd_mode = IM_RASTER_MODE;
        data[pthread_num].core = IM_SCHEDULER_RGA3_CORE1;
        data[pthread_num].priority = 1;
        pthread_num++;
    }

    if (g_chip_config.core_mask & IM_SCHEDULER_RGA2_CORE0) {
        data[pthread_num].id = pthread_num;
        data[pthread_num].name = "RGA2_core0";
        data[pthread_num].dma_heap_name = g_chip_config.heap_path;
        data[pthread_num].mode = false;
        data[pthread_num].num = 0;
        data[pthread_num].width = g_chip_config.default_width;
        data[pthread_num].height = g_chip_config.default_height;
        data[pthread_num].format = g_chip_config.default_format;
        data[pthread_num].rd_mode = IM_RASTER_MODE;
        data[pthread_num].core = IM_SCHEDULER_RGA2_CORE0;
        data[pthread_num].priority = 1;
        pthread_num++;
    }

    if (g_chip_config.core_mask & IM_SCHEDULER_RGA2_CORE1) {
        data[pthread_num].id = pthread_num;
        data[pthread_num].name = "RGA2_core1";
        data[pthread_num].dma_heap_name = g_chip_config.heap_path;
        data[pthread_num].mode = false;
        data[pthread_num].num = 0;
        data[pthread_num].width = g_chip_config.default_width;
        data[pthread_num].height = g_chip_config.default_height;
        data[pthread_num].format = g_chip_config.default_format;
        data[pthread_num].rd_mode = IM_RASTER_MODE;
        data[pthread_num].core = IM_SCHEDULER_RGA2_CORE1;
        data[pthread_num].priority = 1;
        pthread_num++;
    }

    if (run_test(start_id, pthread_num, data, pthread_rga_raster_func) < 0) {
        printf("-------------------------------------------------\n");
        printf("RGA raster-test fail!\n");
        return -1;
    }

    printf("-------------------------------------------------\n");
    printf("RGA raster-test success!\n");

    if (g_chip_config.special_case_en) {
        memset(&data, 0x0, sizeof(private_data_t) * IM2D_SLT_THREAD_MAX);
        printf("-------------------------------------------------\n");

        start_id = pthread_num;

        if (g_chip_config.special_mask & IM_AFBC16x16_MODE) {
            if (g_chip_config.core_mask & IM_SCHEDULER_RGA3_CORE0) {
                data[pthread_num].id = pthread_num;
                data[pthread_num].name = "RGA3_core0_fbc";
                data[pthread_num].dma_heap_name = g_chip_config.heap_path;
                data[pthread_num].mode = false;
                data[pthread_num].num = 0;
                data[pthread_num].width = g_chip_config.default_width;
                data[pthread_num].height = g_chip_config.default_height;
                data[pthread_num].format = g_chip_config.default_format;
                data[pthread_num].rd_mode = IM_AFBC16x16_MODE;
                data[pthread_num].core = IM_SCHEDULER_RGA3_CORE0;
                data[pthread_num].priority = 1;
                pthread_num++;
            }

            if (g_chip_config.core_mask & IM_SCHEDULER_RGA3_CORE1) {
                data[pthread_num].id = pthread_num;
                data[pthread_num].name = "RGA3_core1_fbc";
                data[pthread_num].dma_heap_name = g_chip_config.heap_path;
                data[pthread_num].mode = false;
                data[pthread_num].num = 0;
                data[pthread_num].width = g_chip_config.default_width;
                data[pthread_num].height = g_chip_config.default_height;
                data[pthread_num].format = g_chip_config.default_format;
                data[pthread_num].rd_mode = IM_AFBC16x16_MODE;
                data[pthread_num].core = IM_SCHEDULER_RGA3_CORE1;
                data[pthread_num].priority = 1;
                pthread_num++;
            }
        }

        if (g_chip_config.special_mask & IM_AFBC32x8_MODE) {
            if (g_chip_config.core_mask & IM_SCHEDULER_RGA2_CORE0) {
                data[pthread_num].id = pthread_num;
                data[pthread_num].name = "RGA2_core0_afbc32x8";
                data[pthread_num].dma_heap_name = g_chip_config.heap_path;
                data[pthread_num].mode = false;
                data[pthread_num].num = 0;
                data[pthread_num].width = 320;
                data[pthread_num].height = 240;
                data[pthread_num].format = RK_FORMAT_RGBA_8888;
                data[pthread_num].rd_mode = IM_AFBC32x8_MODE;
                data[pthread_num].core = IM_SCHEDULER_RGA2_CORE0;
                data[pthread_num].priority = 1;
                pthread_num++;
            }

            if (g_chip_config.core_mask & IM_SCHEDULER_RGA2_CORE1) {
                data[pthread_num].id = pthread_num;
                data[pthread_num].name = "RGA2_core1_afbc32x8";
                data[pthread_num].dma_heap_name = g_chip_config.heap_path;
                data[pthread_num].mode = false;
                data[pthread_num].num = 0;
                data[pthread_num].width = 320;
                data[pthread_num].height = 240;
                data[pthread_num].format = RK_FORMAT_RGBA_8888;
                data[pthread_num].rd_mode = IM_AFBC32x8_MODE;
                data[pthread_num].core = IM_SCHEDULER_RGA2_CORE1;
                data[pthread_num].priority = 1;
                pthread_num++;
            }
        }

        if (g_chip_config.special_mask & IM_RKFBC64x4_MODE) {
            if (g_chip_config.core_mask & IM_SCHEDULER_RGA2_CORE0) {
                data[pthread_num].id = pthread_num;
                data[pthread_num].name = "RGA2_core0_rkfbc64x4";
                data[pthread_num].dma_heap_name = g_chip_config.heap_path;
                data[pthread_num].mode = false;
                data[pthread_num].num = 0;
                data[pthread_num].width = 320;
                data[pthread_num].height = 240;
                data[pthread_num].format = RK_FORMAT_YCbCr_420_SP;
                data[pthread_num].rd_mode = IM_RKFBC64x4_MODE;
                data[pthread_num].core = IM_SCHEDULER_RGA2_CORE0;
                data[pthread_num].priority = 1;
                pthread_num++;
            }

            if (g_chip_config.core_mask & IM_SCHEDULER_RGA2_CORE1) {
                data[pthread_num].id = pthread_num;
                data[pthread_num].name = "RGA2_core1_rkfbc64x4";
                data[pthread_num].dma_heap_name = g_chip_config.heap_path;
                data[pthread_num].mode = false;
                data[pthread_num].num = 0;
                data[pthread_num].width = 320;
                data[pthread_num].height = 240;
                data[pthread_num].format = RK_FORMAT_YCbCr_420_SP;
                data[pthread_num].rd_mode = IM_RKFBC64x4_MODE;
                data[pthread_num].core = IM_SCHEDULER_RGA2_CORE1;
                data[pthread_num].priority = 1;
                pthread_num++;
            }
        }

        if (g_chip_config.special_mask & IM_TILE4x4_MODE) {
            if (g_chip_config.core_mask & IM_SCHEDULER_RGA2_CORE0) {
                data[pthread_num].id = pthread_num;
                data[pthread_num].name = "RGA2_core0_tile4x4";
                data[pthread_num].dma_heap_name = g_chip_config.heap_path;
                data[pthread_num].mode = false;
                data[pthread_num].num = 0;
                data[pthread_num].width = g_chip_config.default_width;
                data[pthread_num].height = g_chip_config.default_height;
                data[pthread_num].format = g_chip_config.default_format;
                data[pthread_num].rd_mode = IM_TILE4x4_MODE;
                data[pthread_num].core = IM_SCHEDULER_RGA2_CORE0;
                data[pthread_num].priority = 1;
                pthread_num++;
            }

            if (g_chip_config.core_mask & IM_SCHEDULER_RGA2_CORE1) {
                data[pthread_num].id = pthread_num;
                data[pthread_num].name = "RGA2_core1_tile4x4";
                data[pthread_num].dma_heap_name = g_chip_config.heap_path;
                data[pthread_num].mode = false;
                data[pthread_num].num = 0;
                data[pthread_num].width = g_chip_config.default_width;
                data[pthread_num].height = g_chip_config.default_height;
                data[pthread_num].format = g_chip_config.default_format;
                data[pthread_num].rd_mode = IM_TILE4x4_MODE;
                data[pthread_num].core = IM_SCHEDULER_RGA2_CORE1;
                data[pthread_num].priority = 1;
                pthread_num++;
            }
        }

        if (run_test(start_id, pthread_num, data, pthread_rga_special_func) < 0) {
            printf("-------------------------------------------------\n");
            printf("RGA special-test fail!\n");
            return -1;
        }

        printf("-------------------------------------------------\n");
        printf("RGA special-test success!\n");
    }

    if (g_chip_config.perf_case_en) {
        memset(&data, 0x0, sizeof(private_data_t) * IM2D_SLT_THREAD_MAX);
        printf("-------------------------------------------------\n");

        start_id = pthread_num;

        for (pthread_num = start_id; pthread_num < start_id + IM2D_SLT_THREAD_MAX; pthread_num++) {
            data[pthread_num].id = pthread_num;
            data[pthread_num].name = "perf_test";
            data[pthread_num].dma_heap_name = g_chip_config.heap_path;
            data[pthread_num].mode = true;
            data[pthread_num].num = g_chip_config.while_num;
            data[pthread_num].width = g_chip_config.default_width;
            data[pthread_num].height = g_chip_config.default_height;
            data[pthread_num].format = g_chip_config.default_format;
            data[pthread_num].rd_mode = IM_RASTER_MODE;
            data[pthread_num].core = IM_SCHEDULER_DEFAULT;
            data[pthread_num].priority = 1;
        }

        if (run_test(start_id, pthread_num, data, pthread_rga_perf_func) < 0) {
            printf("-------------------------------------------------\n");
            printf("RGA perf-test fail!\n");
            return -1;
        }

        printf("-------------------------------------------------\n");
        printf("RGA perf-test success!\n");
    }

    printf("-------------------------------------------------\n");

    if (g_golden_generate_crc) {
        printf("RGA slt generate CRC golden data success!\n");
        rga_slt_dump_generate_crc();
        save_crc_table_to_file(g_golden_suffix);
    }

    return 0;
}

#ifdef __RT_THREAD__
struct thread_arg {
    rt_sem_t sem;

    int argc;
    char **argv;

    int ret;
};

void rga_slt_main_thread(void *arg)
{
    struct thread_arg *thread_arg = (struct thread_arg *)arg;
    int ret;

    ret = rga_slt_main(thread_arg->argc, thread_arg->argv);
    if (ret < 0) {
        printf("RGA SLT failed with error code: %d\n", ret);
    }

    thread_arg->ret = ret;
    rt_sem_release(thread_arg->sem);
}

int rga_slt(int argc, char *argv[])
{
    rt_thread_t thread = RT_NULL;
    struct thread_arg arg;

    arg.sem = rt_sem_create("wait_sem", 0, RT_IPC_FLAG_FIFO);
    if (!arg.sem) {
        printf("Failed to create semaphore\n");
        return -1;
    }

    arg.argc = argc;
    arg.argv = argv;

    thread = rt_thread_create("rga_slt", rga_slt_main_thread, &arg, 8192, 16, 10);
    if (thread != RT_NULL) {
        rt_thread_startup(thread);

        rt_sem_take(arg.sem, RT_WAITING_FOREVER);
    } else {
        printf("Failed to create thread\n");

        return -1;
    }

    rt_sem_delete(arg.sem);

    return arg.ret;
}
MSH_CMD_EXPORT(rga_slt, rga_slt)
#endif /* #ifdef __RT_THREAD__ */
