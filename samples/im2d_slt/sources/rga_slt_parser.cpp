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

#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "rga_im2d_slt"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "rga_slt_parser.h"
#include "slt_config.h"

/* slt parser */
#define MODE_HELP_CHAR          'h'
#define MODE_CHIP_CHAR          'c'
#define MODE_PERF_CHAR          'f'
#define MODE_INPUT_CHAR         'i'
#define MODE_OUTPUT_CHAR        'o'
#define MODE_GOLDEN_CHAR        'g'
#define MODE_GOLDEN_PREFIX_CHAR 'p'
#define MODE_GOLDEN_GENERATE_CRC_CHAR   'r'

char g_input_path[RGA_SLT_STRING_MAX] = IM2D_SLT_DEFAULT_INPUT_PATH;
char g_output_path[RGA_SLT_STRING_MAX] = IM2D_SLT_DEFAULT_OUTPUT_PATH;
char g_golden_path[RGA_SLT_STRING_MAX] = IM2D_SLT_DEFAULT_GOLDEN_PATH;
char g_golden_prefix[RGA_SLT_STRING_MAX] = IM2D_SLT_GENERATE_CRC_GOLDEN_PREFIX;
bool g_golden_generate_crc = false;
struct im2d_slt_config g_chip_config = common_rga2_config;

static void help_function(bool all) {
    printf("\n====================================================================================================\n");
    printf( "   usage: im2d_slt  [--help/-h] [--chip/-c] [--perf/-f] [--input/-i] [--output/-o] [--golden/-g] \n"
            "                    [--prefix/-p] [--crc/r]\n\n");

    if (all) {
        printf(
            "---------------------------------------- Config ----------------------------------------------------\n"
            "\t --chip/-c     Set chip\n"
            "\t                 <options>: \n"
            "\t                   <chip>        chip ready for testing, e.g. \"--chip=rk3588\".\n"
            "\t --perf/-f     Set perf mode\n"
            "\t                 <options>: \n"
            "\t                   <num>         set loop num, e.g. \"--perf=50\".\n"
            "\t --input/-i    Set input image file path.\n"
            "\t                 <options>: \n"
            "\t                   <path>        input image file path, e.g. \"--input=/data\".\n"
            "\t --output/-o   Set output image file path.\n"
            "\t                 <options>: \n"
            "\t                   <path>        output image file path, e.g. \"--output=/data\".\n"
            "\t --golden/-g   Set golden file path.\n"
            "\t                 <options>: \n"
            "\t                   <path>        golden image file path, e.g. \"--golden=/data\".\n"
            "\t --prefix/-p   Set golden prefix.\n"
            "\t                 <options>: \n"
            "\t                   <string>      golden image file prefix, e.g. \"--prefix=crcdata\", so that the file name is \"crcdata_xx.bin\".\n"
            "\t --crc/-r      Generate golden by CRC. The target file will be generated according to --golden and --prefix\n"
            "---------------------------------------- Other -----------------------------------------------------\n"
            "\t --help/-h     Call help\n"
            "\t                 <options>:\n"
            "\t                   all           Show full help.\n"
        );
    } else {
        printf( "   If you need to see more detailed instructions, please use the command '--help=all'\n");
    }
    printf("====================================================================================================\n\n");
}

int rga_slt_parse_argv(int argc, char *argv[]) {
    int ret;
    int opt = 0, option_index = 0;

    char strings[] = "h::c:f::i:o:g:p:r";
    static struct option mode_options[] = {
        {   "help", optional_argument, NULL, MODE_HELP_CHAR     },
        {   "chip", required_argument, NULL, MODE_CHIP_CHAR     },
        {   "perf", optional_argument, NULL, MODE_PERF_CHAR     },
        {  "input", required_argument, NULL, MODE_INPUT_CHAR    },
        { "output", required_argument, NULL, MODE_OUTPUT_CHAR   },
        { "golden", required_argument, NULL, MODE_GOLDEN_CHAR   },
        { "prefix", required_argument, NULL, MODE_GOLDEN_PREFIX_CHAR        },
        {    "crc",       no_argument, NULL, MODE_GOLDEN_GENERATE_CRC_CHAR  },
    };

    /* init config */
    strcpy(g_input_path, IM2D_SLT_DEFAULT_INPUT_PATH);
    strcpy(g_output_path, IM2D_SLT_DEFAULT_OUTPUT_PATH);
    strcpy(g_golden_path, IM2D_SLT_DEFAULT_GOLDEN_PATH);
    strcpy(g_golden_prefix, IM2D_SLT_GENERATE_CRC_GOLDEN_PREFIX);
    g_golden_generate_crc = false;

    while ((opt = getopt_long(argc, argv, strings, mode_options, &option_index))!= -1) {
        switch (opt) {
            /* optional_argument */
            case MODE_HELP_CHAR:
                if (optarg != NULL) {
                    if (strcmp(optarg,"all") == 0) {
                        help_function(true);
                    } else {
                        help_function(false);
                    }
                } else {
                    help_function(false);
                }

                return -1;

            /* required_argument */
            case MODE_CHIP_CHAR:
                if (optarg == NULL) {
                    printf("[%s, %d], Invalid parameter: chip = %s\n", __FUNCTION__, __LINE__, optarg);
                    return -1;
                }

                if (strcmp(optarg, "rk3588") == 0) {
                    g_chip_config = rk3588_config;
                } else if (strcmp(optarg, "rk3576") == 0) {
                    g_chip_config = rk3576_config;
                } else if ((strcmp(optarg, "rk3528") == 0) ||
                           (strcmp(optarg, "rk3562") == 0) ||
                           (strcmp(optarg, "rv1126b") == 0) ||
                           (strcmp(optarg, "rv1106") == 0)) {
                    g_chip_config = common_rga2_config;
                } else if (strcmp(optarg, "rv1103b") == 0) {
                    g_chip_config = rv1103b_config;
                } else if (strcmp(optarg, "rk3506") == 0) {
                    g_chip_config = rk3506_config;
                } else {
                    g_chip_config = common_rga2_config;
                    printf("set chip [common_RGA2]\n");

                    break;
                }

                printf("set chip[%s]\n", optarg);

                break;

            default:
                break;
        }
    }

    /* reset optind, re-entrant for getopt_long(). */
    optind = 0;
    while ((opt = getopt_long(argc, argv, strings, mode_options, &option_index))!= -1) {
        if (mode_options[option_index].has_arg == required_argument &&
            optarg == NULL)
            continue;

        switch (opt) {
            /* optional_argument */
            case MODE_PERF_CHAR:
                g_chip_config.perf_case_en = true;

                if (optarg != NULL) {
                    g_chip_config.while_num = atoi(optarg);
                    if (g_chip_config.while_num <= 0) {
                        printf("[%s, %d], Invalid parameter: perf = %s\n", __FUNCTION__, __LINE__, optarg);
                        return -1;
                    }

                } else {
                    g_chip_config.while_num = IM2D_SLT_WHILE_NUM;
                }

                printf("set perf[%d]\n", g_chip_config.while_num);

                break;

            /* required_argument */
            case MODE_INPUT_CHAR:
                memset(g_input_path, 0x0, sizeof(g_input_path));
                ret = sscanf(optarg, "%s", g_input_path );
                if (ret != 1) {
                    printf("[%s, %d], Invalid parameter: level = %s\n", __FUNCTION__, __LINE__, optarg);
                    return -1;
                }

                printf("set input_path[%s]\n", g_input_path);

                break;

            /* required_argument */
            case MODE_OUTPUT_CHAR:
                memset(g_output_path, 0x0, sizeof(g_output_path));
                ret = sscanf(optarg, "%s", g_output_path );
                if (ret != 1) {
                    printf("[%s, %d], Invalid parameter: level = %s\n", __FUNCTION__, __LINE__, optarg);
                    return -1;
                }

                printf("set output_path[%s]\n", g_output_path);

                break;

            /* required_argument */
            case MODE_GOLDEN_CHAR:
                memset(g_golden_path, 0x0, sizeof(g_golden_path));
                ret = sscanf(optarg, "%s", g_golden_path );
                if (ret != 1) {
                    printf("[%s, %d], Invalid parameter: level = %s\n", __FUNCTION__, __LINE__, optarg);
                    return -1;
                }

                printf("set golden_path[%s]\n", g_golden_path);

                break;

            /* required_argument */
            case MODE_GOLDEN_PREFIX_CHAR:
                memset(g_golden_prefix, 0x0, sizeof(g_golden_prefix));
                ret = sscanf(optarg, "%s", g_golden_prefix);
                if (ret != 1) {
                    printf("[%s, %d], Invalid parameter: level = %s\n", __FUNCTION__, __LINE__, optarg);
                    return -1;
                }

                printf("set golden_prefix[%s]\n", g_golden_prefix);

                break;

            /* no_argument */
            case MODE_GOLDEN_GENERATE_CRC_CHAR:
                g_golden_generate_crc = 1;

                printf("enable generate golden by CRC\n");
                break;

            default:
                break;
        }
    }

    return 0;
}
