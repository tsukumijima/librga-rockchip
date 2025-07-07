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

#ifndef _IM2D_SLT_CRC_H_
#define _IM2D_SLT_CRC_H_

// #define RGA_SLT_CASE_MAX 64
#define RGA_SLT_THREAD_MAX 16
#define RGA_SLT_CASE_MAX 16

typedef unsigned int rga_slt_crc_table[RGA_SLT_THREAD_MAX][RGA_SLT_CASE_MAX];

extern rga_slt_crc_table common_golden_data;
extern const rga_slt_crc_table *g_read_golden_data;

void init_crc_table(void);
unsigned int crc32(unsigned int crc,unsigned char *buffer, unsigned int size);

void save_crcdata(unsigned int crc_data, int thread_id, int case_index);
const rga_slt_crc_table *get_crcdata_table(void);

void rga_slt_dump_generate_crc(void);
int save_crc_table_to_file(const char *prefix_name);
const rga_slt_crc_table *read_crc_table_from_file(const char *prefix_name);

inline static int crc_check(int id, int index, unsigned int crc_data, const rga_slt_crc_table *golden_table) {
    if (golden_table == NULL)
        return false;
    return crc_data != (*golden_table)[id][index] ? false : true;
}

#endif /* #ifndef _IM2D_SLT_CRC_H_ */
