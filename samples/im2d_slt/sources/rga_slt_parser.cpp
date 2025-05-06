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
#include <string.h>
#include <getopt.h>

#include "rga_slt_parser.h"
#include "slt_config.h"

/* slt parser */
#define MODE_HELP_CHAR          'h'
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

static void help_function(bool all) {
    printf("\n====================================================================================================\n");
    printf( "   usage: im2d_slt  [--help/-h] [--input/-i] [--output/-o] [--golden/-g] [--prefix/-p] [--crc/r]\n\n");

    if (all) {
        printf(
            "---------------------------------------- Config ----------------------------------------------------\n"
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

    char strings[] = "h::i:o:g:p:r";
    static struct option mode_options[] = {
        {   "help", optional_argument, NULL, MODE_HELP_CHAR     },
        {  "input", required_argument, NULL, MODE_INPUT_CHAR    },
        { "output", required_argument, NULL, MODE_OUTPUT_CHAR   },
        { "golden", required_argument, NULL, MODE_GOLDEN_CHAR   },
        { "prefix", required_argument, NULL, MODE_GOLDEN_PREFIX_CHAR        },
        {    "crc",       no_argument, NULL, MODE_GOLDEN_GENERATE_CRC_CHAR  },
    };

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
