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

#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "rga_im2d_slt"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "slt_config.h"
#include "rga_slt_parser.h"
#include "rga_slt_crc.h"

unsigned int crc_table[256];
rga_slt_crc_table g_generated_golden_data = {0};
const rga_slt_crc_table *g_read_golden_data = NULL;

void init_crc_table(void)
{
	unsigned int c;
	unsigned int i, j;

	for (i = 0; i < 256; i++) {
		c = (unsigned int)i;
		for (j = 0; j < 8; j++) {
			if (c & 1)
				c = 0xedb88320L ^ (c >> 1);
			else
			    c = c >> 1;
		}
		crc_table[i] = c;
	}
}

unsigned int crc32(unsigned int crc,unsigned char *buffer, unsigned int size)
{
	unsigned int i;
	for (i = 0; i < size; i++) {
		crc = crc_table[(crc ^ buffer[i]) & 0xff] ^ (crc >> 8);
	}
	return crc;
}

void rga_slt_dump_generate_crc(void)
{
    printf("====================================================================================================\n");
    printf("RGA SLT CRC Golden Data 2D Array:\n");
    printf("{\n");
    for (int thread_id = 0; thread_id < RGA_SLT_THREAD_MAX; thread_id++) {
        printf("    {\n        ");

        for (int i = 0; i < RGA_SLT_CASE_MAX; i++) {
            printf("0x%08x", g_generated_golden_data[thread_id][i]);

            if (i < RGA_SLT_CASE_MAX - 1) {
                printf(", ");

                if (i % 8 == 7)
                    printf("\n        ");
            }
        }

        if (thread_id < RGA_SLT_THREAD_MAX - 1)
            printf("\n    },\n");
        else
            printf("\n    }\n");
    }

    printf("};\n");
    printf("====================================================================================================\n");
}

int save_crc_table_to_file(const char *prefix_name) {
    int len;
    FILE* crc_file = NULL;
    char file_name[RGA_SLT_STRING_MAX];

    len = snprintf(file_name, sizeof(file_name), "%s/%s_%s.txt",
            g_golden_path,
            g_golden_prefix,
            prefix_name);
    if (len >= RGA_SLT_STRING_MAX) {
        printf("%s,%d:File name too long: %s\n", __FUNCTION__, __LINE__, file_name);
        return -1;
    }

    crc_file = fopen(file_name, "wb+");
    if(crc_file == NULL){
        printf("%s,%d:openFile %s fail\n",__FUNCTION__,__LINE__,file_name);
        return -1;
    }

    for (int thread_id = 0; thread_id < RGA_SLT_THREAD_MAX; thread_id++) {
        for (int i = 0; i < RGA_SLT_CASE_MAX; i++) {
            fprintf(crc_file, "0x%X,", g_generated_golden_data[thread_id][i]);
        }
    }

    fclose(crc_file);

    printf("Save CRC golden data to file: %s\n", file_name);

    /* golden.bin */
    len = snprintf(file_name, sizeof(file_name), "%s/%s_%s.bin",
            g_golden_path,
            g_golden_prefix,
            prefix_name);
    if (len >= RGA_SLT_STRING_MAX) {
        printf("%s,%d:File name too long: %s\n", __FUNCTION__, __LINE__, file_name);
        return -1;
    }

    crc_file = fopen(file_name, "wb+");
    if(crc_file == NULL){
        printf("%s,%d:openFile %s fail\n", __FUNCTION__, __LINE__, file_name);
        return -1;
    }

    for (int thread_id = 0; thread_id < RGA_SLT_THREAD_MAX; thread_id++) {
        for (int i = 0; i < RGA_SLT_CASE_MAX; i++) {
            fwrite(&g_generated_golden_data[thread_id][i], sizeof(unsigned int), 1, crc_file);
        }
    }

    fclose(crc_file);

    printf("Save CRC golden data to file: %s\n", file_name);

    return 0;
}

const rga_slt_crc_table *read_crc_table_from_file(const char *prefix_name) {
    int len;
    int size;
    FILE *crc_file = NULL;
    char file_name[RGA_SLT_STRING_MAX];

    len = snprintf(file_name, sizeof(file_name), "%s/%s_%s.bin",
            g_golden_path,
            g_golden_prefix,
            prefix_name);
    if (len >= RGA_SLT_STRING_MAX) {
        printf("%s,%d:File name too long: %s\n", __FUNCTION__, __LINE__, file_name);
        return NULL;
    }

    crc_file = fopen(file_name, "rb");
    if (crc_file) {
        fseek(crc_file, 0, SEEK_END);
        size = ftell(crc_file);

        if (size != sizeof(g_generated_golden_data)) {
            printf("File size mismatch: expected %zu, got %d\n",
                  sizeof(g_generated_golden_data), size);
            fclose(crc_file);
            return NULL;
        }

        fseek(crc_file, 0, SEEK_SET);
        fread((void*)&g_generated_golden_data, size, 1, crc_file);
        fclose(crc_file);

        printf("Read CRC golden data from file: %s\n", file_name);
        return &g_generated_golden_data;
    } else {
        printf("Could not open file: %s\n", file_name);
    }

    return NULL;
}

void save_crcdata(unsigned int crc_data, int thread_id, int case_index)
{
    g_generated_golden_data[thread_id][case_index] = crc_data;
}

const rga_slt_crc_table *get_crcdata_table(void) {
    if (g_read_golden_data != NULL)
        return g_read_golden_data;
    else if (g_chip_config.crc_data != NULL)
        return g_chip_config.crc_data;
    else
        return NULL;
}

rga_slt_crc_table common_golden_data = {
    {
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    },
    {
        0xc995faf0, 0xcb38771a, 0xd99833f0, 0xbe8e2acf, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    },
    {
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    },
    {
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    },
    {
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    },
    {
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    },
    {
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    },
    {
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    },
    {
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    },
    {
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    },
    {
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    },
    {
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    },
    {
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    },
    {
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    },
    {
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    },
    {
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    }
};
