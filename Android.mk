#
# Copyright (C) 2018  Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
# Authors:
#     lihuang <putin.li@rock-chips.com>
#     libin <bin.li@rock-chips.com>
#
# This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# BY DOWNLOADING, INSTALLING, COPYING, SAVING OR OTHERWISE USING THIS SOFTWARE,
# YOU ACKNOWLEDGE THAT YOU AGREE THE SOFTWARE RECEIVED FROM ROCKCHIP IS PROVIDED
# TO YOU ON AN "AS IS" BASIS and ROCKCHIP DISCLAIMS ANY AND ALL WARRANTIES AND
# REPRESENTATIONS WITH RESPECT TO SUCH FILE, WHETHER EXPRESS, IMPLIED, STATUTORY
# OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY IMPLIED WARRANTIES OF TITLE,
# NON-INFRINGEMENT, MERCHANTABILITY, SATISFACTORY QUALITY, ACCURACY OR FITNESS FOR
# A PARTICULAR PURPOSE.
#

LOCAL_PATH:= $(call my-dir)

ifeq (1,$(strip $(shell expr $(PLATFORM_SDK_VERSION) \< 28)))

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    core/utils/android_utils/src/android_utils.cpp \
    core/utils/drm_utils/src/drm_utils.cpp \
    core/utils/utils.cpp \
    core/RockchipRga.cpp \
    core/GrallocOps.cpp \
    core/NormalRga.cpp \
    core/NormalRgaApi.cpp \
    core/RgaApi.cpp \
    core/RgaUtils.cpp \
    core/RgaUtils_symbol.cpp \
    core/rga_gralloc.cpp \
    core/rga_sync.cpp \
    im2d_api/src/im2d_log.cpp \
    im2d_api/src/im2d_debugger.cpp \
    im2d_api/src/im2d_context.cpp \
    im2d_api/src/im2d_job.cpp \
    im2d_api/src/im2d_impl.cpp \
    im2d_api/src/im2d.cpp

LOCAL_MODULE := librga
LOCAL_PROPRIETARY_MODULE := true

LOCAL_EXPORT_C_INCLUDE_DIRS := \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/im2d_api

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/im2d_api \
    $(LOCAL_PATH)/core \
    $(LOCAL_PATH)/core/hardware \
    $(LOCAL_PATH)/core/utils \
    $(LOCAL_PATH)/core/3rdparty/libdrm/include/drm \
    $(LOCAL_PATH)/core/3rdparty/android_hal

LOCAL_C_INCLUDES += frameworks/native/libs/nativewindow/include

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libui \
    libcutils \
    libhardware

ifneq ($(strip $(TARGET_BOARD_PLATFORM)),rk3368)
    LOCAL_SHARED_LIBRARIES += libgralloc_drm
endif

LOCAL_CFLAGS := \
    -DLOG_TAG=\"librga\" \
    -DANDROID

LOCAL_CFLAGS += \
    -Wno-error \
    -Wno-missing-braces \
    -Wno-unused-parameter \
    -Wno-format

ifeq (1,$(strip $(shell expr $(PLATFORM_SDK_VERSION) \> 25)))
    LOCAL_CFLAGS += -DUSE_AHARDWAREBUFFER=1
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),rk3126c)
    LOCAL_CFLAGS += -DRK3126C
else ifeq ($(strip $(TARGET_BOARD_PLATFORM)),rk3188)
    LOCAL_CFLAGS += -DRK3188
else ifeq ($(strip $(TARGET_BOARD_PLATFORM)),rk3368)
    LOCAL_CFLAGS += -DRK3368
endif

ifneq (1,$(strip $(shell expr $(PLATFORM_VERSION) \< 6.9)))
    LOCAL_CFLAGS += -DANDROID_7_DRM
    LOCAL_CFLAGS += -DRK_DRM_GRALLOC=1
endif

ifneq (1,$(strip $(shell expr $(PLATFORM_VERSION) \< 8.0)))
    LOCAL_CFLAGS += -DANDROID_8
endif

LOCAL_MODULE_TAGS := optional
#LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := $(TARGET_SHLIB_SUFFIX)

include $(BUILD_SHARED_LIBRARY)

endif #end of PLATFORM_SDK_VERSION < 28
include $(call first-makefiles-under,$(LOCAL_PATH))
