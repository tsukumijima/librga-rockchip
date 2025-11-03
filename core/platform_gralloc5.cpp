/*
 * Copyright (C) 2025 RockChip Limited. All rights reserved.
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

/*  --------------------------------------------------------------------------------------------------------
 *  File:   platform_gralloc5.c
 *
 *  Desc:   具体实现 platform_gralloc5.h 定义的接口, 基于 stablec_mapper.
 *
 *          -----------------------------------------------------------------------------------
 *          < 习语 和 缩略语 > :
 *
 *          -----------------------------------------------------------------------------------
 *          < 内部模块 or 对象的层次结构 > :
 *
 *          -----------------------------------------------------------------------------------
 *          < 功能实现的基本流程 > :
 *
 *          -----------------------------------------------------------------------------------
 *          < 关键标识符 > :
 *
 *          -----------------------------------------------------------------------------------
 *          < 本模块实现依赖的外部模块 > :
 *              ...
 *          -----------------------------------------------------------------------------------
 *  Note:
 *
 *  Author: ChenZhen
 *
 *  Log:
 *          init
    ----Tue Sep 16 18:17:30 2025
 */

/* ---------------------------------------------------------------------------------------------------------
 * Include Files
 * ---------------------------------------------------------------------------------------------------------
 */

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "platform_gralloc5"
#endif

#include <dlfcn.h>
#include <inttypes.h>

#include <sync/sync.h>

#include <drm_fourcc.h>

#ifdef USE_HARDWARE_ROCKCHIP
#include <hardware/hardware_rockchip.h>
#endif

#include <aidl/android/hardware/graphics/allocator/IAllocator.h>
#include <android/binder_manager.h>
#include <android/hardware/graphics/mapper/IMapper.h>
#include <android/hardware/graphics/mapper/utils/IMapperMetadataTypes.h>

// #include <aidl/arm/graphics/ArmMetadataType.h>

#include "platform_gralloc5.h"

/*
using aidl::android::hardware::graphics::common::Dataspace;
using aidl::android::hardware::graphics::common::ExtendableType;
using aidl::android::hardware::graphics::common::PlaneLayout;
using aidl::android::hardware::graphics::common::Rect;
 */
using namespace aidl::android::hardware::graphics::common;
// using aidl::android::hardware::graphics::common::PixelFormat;

using namespace aidl::android::hardware::graphics::allocator;
// using aidl::android::hardware::graphics::allocator::IAllocator;
using namespace android;
using namespace android::hardware;
using namespace ::android::hardware::graphics::mapper;

/*---------------------------------------------------------------------------*/

struct metadata_descriptor {
    std::string name;
    int64_t value;
};

#define GRALLOC_ARM_METADATA_TYPE_NAME "arm.graphics.ArmMetadataType"
const static metadata_descriptor ArmMetadataType_PLANE_FDS = {GRALLOC_ARM_METADATA_TYPE_NAME, 1};
const static metadata_descriptor ArmMetadataType_FORMAT_DATA_TYPE = {GRALLOC_ARM_METADATA_TYPE_NAME, 2};

/*---------------------------------------------------------------------------*/

#ifndef OFFSET_OF_DYNAMIC_HDR_METADATA
#define OFFSET_OF_DYNAMIC_HDR_METADATA (1)
#define GRALLOC_RK_METADATA_TYPE_NAME "rk.graphics.RkMetadataType"
const static metadata_descriptor RkMetadataType_OFFSET_OF_DYNAMIC_HDR_METADATA = {GRALLOC_RK_METADATA_TYPE_NAME,
                                                                                  OFFSET_OF_DYNAMIC_HDR_METADATA};
/* 直接将 hdr_metadata_buffer 扩展为 pq_metadata_buffer. */
#define RkMetadataType_OFFSET_OF_PQ_METADATA RkMetadataType_OFFSET_OF_DYNAMIC_HDR_METADATA
#endif

#define FPS (2)
const static metadata_descriptor RkMetadataType_FPS = {
        GRALLOC_RK_METADATA_TYPE_NAME,
        FPS,
};

#define SIZE_OF_PQ_METADATA (3)
const static metadata_descriptor RkMetadataType_SIZE_OF_PQ_METADATA = {
        GRALLOC_RK_METADATA_TYPE_NAME,
        SIZE_OF_PQ_METADATA,
};

/*---------------------------------------------------------------------------*/

/**
 * @brief Metadata types that can be queried by the mapper implementation
 */
enum class metadata_type {
    Usage,
    PlaneLayouts,
    FormatFourcc,
    FormatModifier,
    Crop,
    Width,
    Height,
    AllocationSize,
    LayerCount,
    Dataspace,
    ChromaSiting,
    Compression,
    Smpte2094_40,
    ArmPlaneFds,
    ArmFormatDataType,
    FormatRequested,
    Stride,
    BufferId,
    Name,
    RkOffsetOfPqMetadata,
    RkFps,
    RkSizeOfPqMetadata,
};

enum class mapper_error : int32_t {
    /**
     * No error.
     */
    NONE = 0,
    /**
     * Invalid BufferDescriptor.
     */
    BAD_DESCRIPTOR = 1,
    /**
     * Invalid buffer handle.
     */
    BAD_BUFFER = 2,
    /**
     * Invalid HardwareBufferDescription.
     */
    BAD_VALUE = 3,
    /**
     * Invalid type.
     */
    BAD_TYPE = 4,
    /**
     * Resource unavailable.
     */
    NO_RESOURCES = 5,
    /**
     * Permanent failure.
     */
    UNSUPPORTED = 7,
};

/*---------------------------------------------------------------------------*/

const static AIMapper_MetadataType ArmMetadataType_PLANE_FDS_c{ArmMetadataType_PLANE_FDS.name.c_str(),
                                                               ArmMetadataType_PLANE_FDS.value};

const static AIMapper_MetadataType ArmMetadataType_FORMAT_DATA_TYPE_c{ArmMetadataType_FORMAT_DATA_TYPE.name.c_str(),
                                                                      ArmMetadataType_FORMAT_DATA_TYPE.value};

/*---------------------------------------------------------------------------*/

const static AIMapper_MetadataType RkMetadataType_OFFSET_OF_DYNAMIC_HDR_METADATA_c{
        RkMetadataType_OFFSET_OF_DYNAMIC_HDR_METADATA.name.c_str(),
        RkMetadataType_OFFSET_OF_DYNAMIC_HDR_METADATA.value};

const static AIMapper_MetadataType RkMetadataType_FPS_c{RkMetadataType_FPS.name.c_str(), RkMetadataType_FPS.value};

const static AIMapper_MetadataType RkMetadataType_SIZE_OF_PQ_METADATA_c{RkMetadataType_SIZE_OF_PQ_METADATA.name.c_str(),
                                                                        RkMetadataType_SIZE_OF_PQ_METADATA.value};

/*---------------------------------------------------------------------------*/

#ifndef DRM_FORMAT_NV15
#define DRM_FORMAT_NV15 fourcc_code('N', 'V', '1', '5') /* 2x2 subsampled Cr:Cb plane */
#endif

#define RK_HAL_PIXEL_FORMAT_RGBX_1010102 0x300

/*---------------------------------------------------------------------------*/

namespace gralloc5 {

/*
 * 闫孝军反馈“4.19内核里面没这个format，要从linux 主线 5.2 以后里面反向porting 回来。4.19和5.2差别很大。
 * 反向porting有很多冲突要解决”，所以从上层HWC模块去规避这个问题，HWC实现如下：
 * 1.格式转换：
 *   DRM_FORMAT_YUV420_10BIT => DRM_FORMAT_NV12_10
 *   DRM_FORMAT_YUV420_8BIT  => DRM_FORMAT_NV12
 *   DRM_FORMAT_YUYV         => DRM_FORMAT_NV16
 *
 * 2.Byte Stride 转换：
 *   DRM_FORMAT_NV12_10 / DRM_FORMAT_NV12:
 *       Byte stride = Byte stride / 1.5
 *
 *   DRM_FORMAT_NV16:
 *       Byte stride = Byte stride / 2
 *
 * 按上述实现，可以在当前版本保证视频送显正常，提供开关 WORKROUND_FOR_VOP2_DRIVER
 */
static int DrmVersion = 0;
void set_drm_version(int version) {
    DrmVersion = version;
}

// vendor.hwc.disable_gralloc4_use_vir_height = true
bool use_vir_height = true;
void init_env_property() {
    // char value[PROPERTY_VALUE_MAX];
    // property_get("vendor.hwc.disable_gralloc4_use_vir_height", value, "0");
    // use_vir_height = (atoi(value) == 0);
}

/*---------------------------------------------------------------------------*/

static mapper_error stablec_error_to_mapper_error(AIMapper_Error stablec_error) {
    switch (stablec_error) {
        case AIMapper_Error::AIMAPPER_ERROR_NONE:
            return mapper_error::NONE;
        case AIMapper_Error::AIMAPPER_ERROR_BAD_BUFFER:
            return mapper_error::BAD_BUFFER;
        case AIMapper_Error::AIMAPPER_ERROR_BAD_VALUE:
            return mapper_error::BAD_VALUE;
        case AIMapper_Error::AIMAPPER_ERROR_NO_RESOURCES:
            return mapper_error::NO_RESOURCES;
        default:
            return mapper_error::UNSUPPORTED;
    }
}

const std::string aidl_allocator_service_name = std::string{IAllocator::descriptor} + "/default";
std::shared_ptr<IAllocator> get_aidl_allocator(int min_version) {
    auto allocator =
            IAllocator::fromBinder(ndk::SpAIBinder(AServiceManager_checkService(aidl_allocator_service_name.c_str())));

    int32_t version = 0;
    if (allocator != nullptr && allocator->getInterfaceVersion(&version).isOk()) {
        if (version >= min_version) {
            return allocator;
        }
    }

    return nullptr;
}

static AIMapper* get_stable_c_mapper_service() {
    const int min_version_required = 2;
    /* AIDL allocator 2 supports the calls we require for C IMapper */
    auto allocator = get_aidl_allocator(min_version_required);
    if (allocator == nullptr) {
        /* No AIDL allocator V2 loaded */
        return nullptr;
    }

    std::string mapper_suffix;
    auto status = allocator->getIMapperLibrarySuffix(&mapper_suffix);
    if (!status.isOk()) {
        ALOGE("Failed to get IMapper library suffix");
        return nullptr;
    }

    std::string lib_dir_name = "lib";
#if defined(__aarch64__)
    lib_dir_name += "64";
#endif

    std::string lib_name = "/vendor/" + lib_dir_name + "/hw/mapper." + mapper_suffix + ".so";
    void* mapper_library = dlopen(lib_name.c_str(), RTLD_LOCAL | RTLD_NOW);
    if (mapper_library == nullptr) {
        ALOGE("Failed to locate stable-C mapper library");
        return nullptr;
    }

    typedef AIMapper_Error (*AIMapper_loadIMapperFn)(AIMapper* _Nullable* _Nonnull outImplementation);
    auto mapper_loader = reinterpret_cast<AIMapper_loadIMapperFn>(dlsym(mapper_library, "AIMapper_loadIMapper"));
    if (mapper_loader == nullptr) {
        ALOGE("Failed to locate stable-C mapper library load function");
        return nullptr;
    }

    AIMapper* mapper = nullptr;
    auto result = mapper_loader(&mapper);
    if (result != AIMAPPER_ERROR_NONE) {
        ALOGE("Failed to call stable-C mapper library function with error: %d", static_cast<int>(result));
        return nullptr;
    }

    return mapper;
}

static AIMapper& get_service() {
    static AIMapper* cached_service = get_stable_c_mapper_service();
    return *cached_service;
}

mapper_error convert_to_stablec_metadata_type(metadata_type type, AIMapper_MetadataType& mapper_type) {
#define AIMapper_METADATA(TYPE)                                             \
    {                                                                       \
        StandardMetadata<StandardMetadataType::TYPE>::Header::name,         \
                StandardMetadata<StandardMetadataType::TYPE>::Header::value \
    }

    static std::unordered_map<metadata_type, AIMapper_MetadataType> metadata_types_to_imapper_types = {
            {metadata_type::Usage, AIMapper_METADATA(USAGE)},
            {metadata_type::PlaneLayouts, AIMapper_METADATA(PLANE_LAYOUTS)},
            {metadata_type::FormatFourcc, AIMapper_METADATA(PIXEL_FORMAT_FOURCC)},
            {metadata_type::FormatModifier, AIMapper_METADATA(PIXEL_FORMAT_MODIFIER)},
            {metadata_type::Crop, AIMapper_METADATA(CROP)},
            {metadata_type::Width, AIMapper_METADATA(WIDTH)},
            {metadata_type::Height, AIMapper_METADATA(HEIGHT)},
            {metadata_type::AllocationSize, AIMapper_METADATA(ALLOCATION_SIZE)},
            {metadata_type::LayerCount, AIMapper_METADATA(LAYER_COUNT)},
            {metadata_type::Dataspace, AIMapper_METADATA(DATASPACE)},
            {metadata_type::ChromaSiting, AIMapper_METADATA(CHROMA_SITING)},
            {metadata_type::Compression, AIMapper_METADATA(COMPRESSION)},
            {metadata_type::Smpte2094_40, AIMapper_METADATA(SMPTE2094_40)},
            {metadata_type::ArmPlaneFds, ArmMetadataType_PLANE_FDS_c},
            {metadata_type::ArmFormatDataType, ArmMetadataType_FORMAT_DATA_TYPE_c},
            {metadata_type::FormatRequested, AIMapper_METADATA(PIXEL_FORMAT_REQUESTED)},
            {metadata_type::Stride, AIMapper_METADATA(STRIDE)},
            {metadata_type::BufferId, AIMapper_METADATA(BUFFER_ID)},
            {metadata_type::Name, AIMapper_METADATA(NAME)},
            {metadata_type::RkOffsetOfPqMetadata, RkMetadataType_OFFSET_OF_DYNAMIC_HDR_METADATA_c},
            {metadata_type::RkFps, RkMetadataType_FPS_c},
            {metadata_type::RkSizeOfPqMetadata, RkMetadataType_SIZE_OF_PQ_METADATA_c},
    };

    auto iter = metadata_types_to_imapper_types.find(type);
    if (iter == metadata_types_to_imapper_types.end()) {
        return mapper_error::UNSUPPORTED;
    }

    mapper_type = iter->second;
    return mapper_error::NONE;
}

static mapper_error get_metadata_internal(metadata_type type, buffer_handle_t handle, std::vector<uint8_t>& output) {
    auto& mapper = get_service();
    AIMapper_MetadataType mapper_type;
    auto err = convert_to_stablec_metadata_type(type, mapper_type);
    if (err != mapper_error::NONE) {
        return err;
    }
    std::vector<uint8_t> tmp_output(512); /* Pre-allocate some memory for the query */
    int32_t size_required = mapper.v5.getMetadata(handle, mapper_type, tmp_output.data(), tmp_output.capacity());
    if (size_required < 0) {
        return stablec_error_to_mapper_error(static_cast<AIMapper_Error>(-size_required));
    } else if (size_required > static_cast<int32_t>(tmp_output.capacity())) {
        tmp_output.resize(size_required);
        size_required = mapper.v5.getMetadata(handle, mapper_type, tmp_output.data(), tmp_output.capacity());
    }

    if (size_required < 0) {
        return mapper_error::BAD_VALUE;
    }

    tmp_output.resize(size_required);
    output = std::move(tmp_output);
    return mapper_error::NONE;
}

static mapper_error decode_arm_plane_fds(const std::vector<uint8_t>& input, std::vector<int64_t>* fds) {
    assert(fds != nullptr);

    int64_t size = 0;
    auto input_size = input.size();
    if (input_size < sizeof(int64_t)) {
        ALOGE("%s: bad input size %zu", __FUNCTION__, input_size);
        return mapper_error::BAD_VALUE;
    }

    memcpy(&size, input.data(), sizeof(int64_t));
    if (size < 0) {
        ALOGE("%s: bad fds size decoded %" PRId64, __FUNCTION__, size);
        return mapper_error::BAD_VALUE;
    }

    auto fds_size = size * sizeof(int64_t);
    if (input_size - sizeof(int64_t) < fds_size) {
        ALOGE("%s: bad input size %d to expected %" PRId64, __FUNCTION__,
              static_cast<int>(input_size - sizeof(int64_t)), fds_size);
        return mapper_error::BAD_VALUE;
    }

    fds->resize(size);

    const uint8_t* fds_start = input.data() + sizeof(int64_t);
    memcpy(fds->data(), fds_start, fds_size);

    return mapper_error::NONE;
}

static mapper_error decode_format_datatype(const std::vector<uint8_t>& input, int64_t* dt) {
    assert(dt != nullptr);
    if (input.size() < sizeof(int64_t)) {
        ALOGE("%s: bad input size %zu", __FUNCTION__, input.size());
        return mapper_error::BAD_VALUE;
    }

    std::memcpy(dt, input.data(), sizeof(int64_t));
    return mapper_error::NONE;
}

static mapper_error decodeRkFps(const std::vector<uint8_t>& input, uint32_t* dt) {
    assert(dt != nullptr);
    if (input.size() < sizeof(uint32_t)) {
        ALOGE("%s: bad input size %zu", __FUNCTION__, input.size());
        return mapper_error::BAD_VALUE;
    }

    std::memcpy(dt, input.data(), sizeof(uint32_t));
    return mapper_error::NONE;
}

static mapper_error decodeRkSizeOfPqMetadata(const std::vector<uint8_t>& input, int64_t* dt) {
    assert(dt != nullptr);
    if (input.size() < sizeof(int64_t)) {
        ALOGE("%s: bad input size %zu", __FUNCTION__, input.size());
        return mapper_error::BAD_VALUE;
    }

    std::memcpy(dt, input.data(), sizeof(int64_t));
    return mapper_error::NONE;
}

static mapper_error decodeRkOffsetOfVideoMetadata(const std::vector<uint8_t>& input, int64_t* dt) {
    assert(dt != nullptr);
    if (input.size() < sizeof(int64_t)) {
        ALOGE("%s: bad input size %zu", __FUNCTION__, input.size());
        return mapper_error::BAD_VALUE;
    }

    std::memcpy(dt, input.data(), sizeof(int64_t));
    return mapper_error::NONE;
}

static std::function<mapper_error(std::vector<uint8_t>&, void*)> get_decode_function(metadata_type type) {
    /* Macro to make use of predefined decode functions provided by the stableC interface */
#define DECODE_FUNCTION(standard_metadata_type)                                                                        \
    [](std::vector<uint8_t>& arr, void* out) {                                                                         \
        auto value =                                                                                                   \
                StandardMetadata<StandardMetadataType::standard_metadata_type>::value::decode(arr.data(), arr.size()); \
        if (!value.has_value()) {                                                                                      \
            return mapper_error::BAD_VALUE;                                                                            \
        }                                                                                                              \
                                                                                                                       \
        using value_type = decltype(&*value);                                                                          \
        *reinterpret_cast<value_type>(out) = *value;                                                                   \
                                                                                                                       \
        return mapper_error::NONE;                                                                                     \
    }

    switch (type) {
        case metadata_type::Usage:
            return DECODE_FUNCTION(USAGE);
        case metadata_type::PlaneLayouts:
            return DECODE_FUNCTION(PLANE_LAYOUTS);
        case metadata_type::FormatFourcc:
            return DECODE_FUNCTION(PIXEL_FORMAT_FOURCC);
        case metadata_type::FormatModifier:
            return DECODE_FUNCTION(PIXEL_FORMAT_MODIFIER);
        case metadata_type::Crop:
            return DECODE_FUNCTION(CROP);
        case metadata_type::Width:
            return DECODE_FUNCTION(WIDTH);
        case metadata_type::Height:
            return DECODE_FUNCTION(HEIGHT);
        case metadata_type::AllocationSize:
            return DECODE_FUNCTION(ALLOCATION_SIZE);
        case metadata_type::LayerCount:
            return DECODE_FUNCTION(LAYER_COUNT);
        case metadata_type::Dataspace:
            return DECODE_FUNCTION(DATASPACE);
        case metadata_type::ChromaSiting:
            return DECODE_FUNCTION(CHROMA_SITING);
        case metadata_type::Compression:
            return DECODE_FUNCTION(COMPRESSION);
        case metadata_type::Smpte2094_40:
            return DECODE_FUNCTION(SMPTE2094_40);
        case metadata_type::ArmPlaneFds:
            return [](std::vector<uint8_t>& input, void* output) {
                return decode_arm_plane_fds(input, reinterpret_cast<std::vector<int64_t>*>(output));
            };
        case metadata_type::ArmFormatDataType:
            return [](std::vector<uint8_t>& input, void* output) {
                return decode_format_datatype(input, reinterpret_cast<int64_t*>(output));
            };
        case metadata_type::FormatRequested:
            return DECODE_FUNCTION(PIXEL_FORMAT_REQUESTED);
        case metadata_type::Stride:
            return DECODE_FUNCTION(STRIDE);
        case metadata_type::BufferId:
            return DECODE_FUNCTION(BUFFER_ID);
        case metadata_type::Name:
            return DECODE_FUNCTION(NAME);
        case metadata_type::RkOffsetOfPqMetadata:
            return [](std::vector<uint8_t>& input, void* output) {
                return decodeRkOffsetOfVideoMetadata(input, reinterpret_cast<int64_t*>(output));
            };
        case metadata_type::RkSizeOfPqMetadata:
            return [](std::vector<uint8_t>& input, void* output) {
                return decodeRkSizeOfPqMetadata(input, reinterpret_cast<int64_t*>(output));
            };
        case metadata_type::RkFps:
            return [](std::vector<uint8_t>& input, void* output) {
                return decodeRkFps(input, reinterpret_cast<uint32_t*>(output));
            };
        default:
            return [](std::vector<uint8_t>&, void*) { return mapper_error::UNSUPPORTED; };
    }
}

template <typename T>
static mapper_error get_metadata(metadata_type type, buffer_handle_t handle, T& value) {
    std::vector<uint8_t> output;

    auto err = get_metadata_internal(type, handle, output);
    if (err != mapper_error::NONE) {
        return err;
    }

    auto fnc = get_decode_function(type);
    return fnc(output, &value);
}

/* ---------------------------------------------------------------------------------------------------------
 * Global Functions Implementation
 * ---------------------------------------------------------------------------------------------------------
 */

uint64_t get_format_modifier(buffer_handle_t handle) {
    uint64_t modifier;

    /* 获取 format_modifier. */
    auto err = get_metadata(metadata_type::FormatModifier, handle, modifier);
    assert(err == mapper_error::NONE);

    return modifier;
}

/*---------------------------------------------------------------------------*/

uint32_t get_fourcc_format(buffer_handle_t handle) {
    auto& mapper = get_service();
    uint32_t fourcc;

    /* 获取 format_fourcc. */
    auto err = get_metadata(metadata_type::FormatFourcc, handle, fourcc);
    assert(err == mapper_error::NONE);

    return fourcc;
}

int get_width(buffer_handle_t handle, uint64_t* width) {
    auto& mapper = get_service();

    auto err = get_metadata(metadata_type::Width, handle, *width);
    if (err != mapper_error::NONE) {
        ALOGE("err : %d", err);
    }

    return (int)err;
}

int get_height(buffer_handle_t handle, uint64_t* height) {
    auto& mapper = get_service();
    auto err = get_metadata(metadata_type::Height, handle, *height);
    if (err != mapper_error::NONE) {
        ALOGE("err : %d", err);
    }

    return (int)err;
}

int get_height_stride(buffer_handle_t handle, uint64_t* height_stride) {
    auto& mapper = get_service();
    std::vector<PlaneLayout> layouts;
    auto err = get_metadata(metadata_type::PlaneLayouts, handle, layouts);
    if (err != mapper_error::NONE || layouts.size() < 1) {
        ALOGE("Failed to get plane layouts. err : %d", err);
        return (int)err;
    }

    if (layouts.size() > 1) {
        // W("it's not reasonable to get global pixel_stride of buffer with planes more than 1.");
    }

    *height_stride = layouts[0].heightInSamples;
    return (int)err;
}

int get_bit_per_pixel(buffer_handle_t handle, int* bit_per_pixel) {
    auto& mapper = get_service();
    std::vector<PlaneLayout> layouts;
    int format_requested;

    int ret = get_format_requested(handle, &format_requested);
    if (ret != android::OK) {
        ALOGE("err : %d", ret);
        return ret;
    }

    auto err = get_metadata(metadata_type::PlaneLayouts, handle, layouts);
    if (err != mapper_error::NONE || layouts.size() < 1) {
        ALOGE("Failed to get plane layouts. err : %d", err);
        return (int)err;
    }

    if (layouts.size() > 1) {
        // W("it's not reasonable to get global pixel_stride of buffer with planes more than 1.");
    }

    *bit_per_pixel = (layouts[0].sampleIncrementInBits);

    return (int)err;
}

int get_pixel_stride(buffer_handle_t handle, int* pixel_stride) {
    int byte_stride = 0;

    int err = get_byte_stride(handle, &byte_stride);
    if (err != android::OK) {
        ALOGE("err : %d", err);
        return err;
    }

    int bit_per_pixel = 0;
    err = get_bit_per_pixel(handle, &bit_per_pixel);
    if (err != android::OK) {
        ALOGE("err : %d", err);
        return err;
    }

    *pixel_stride = byte_stride * 8 / bit_per_pixel;
    return err;
}

int get_byte_stride(buffer_handle_t handle, int* byte_stride) {
    auto& mapper = get_service();
    std::vector<PlaneLayout> layouts;
    int format_requested;
    mapper_error err = mapper_error::NONE;

    int ret = get_format_requested(handle, &format_requested);
    if (ret != android::OK) {
        ALOGE("err : %d", ret);
        return ret;
    }

    /* 若 'format_requested' "不是" HAL_PIXEL_FORMAT_YCrCb_NV12_10, 则 ... */
    if (format_requested != HAL_PIXEL_FORMAT_YCrCb_NV12_10) {
        err = get_metadata(metadata_type::PlaneLayouts, handle, layouts);
        if (err != mapper_error::NONE || layouts.size() < 1) {
            ALOGE("Failed to get plane layouts. err : %d", err);
            return (int)err;
        }

        if (layouts.size() > 1) {
            // W("it's not reasonable to get global byte_stride of buffer with planes more than 1.");
        }
        *byte_stride = (layouts[0].strideInBytes);
    }
    /* 否则, 即 'format_requested' "是" HAL_PIXEL_FORMAT_YCrCb_NV12_10, 则 ... */
    else {
        uint32_t fourcc_format = get_fourcc_format(handle);
        // RK3588 mali 支持NV15格式，故 byte_stride采用正确的值
        if (fourcc_format == DRM_FORMAT_NV15) {
            err = get_metadata(metadata_type::PlaneLayouts, handle, layouts);
            if (err != mapper_error::NONE || layouts.size() < 1) {
                ALOGE("Failed to get plane layouts. err : %d", err);
                return (int)err;
            }

            if (layouts.size() > 1) {
                // W("it's not reasonable to get global byte_stride of buffer with planes more than 1.");
            }
            *byte_stride = (layouts[0].strideInBytes);
        }
        // 对于 fourcc不为 DRM_FORMAT_NV15 的情况，认为 Mali不支持 NV15格式,采用 width 作为 byte_stride.
        else {
            uint64_t width;

            ret = get_width(handle, &width);
            if (ret != android::OK) {
                ALOGE("err : %d", ret);
                return ret;
            }

            // .KP : from CSY : 分配 rk_video_decoder 输出 buffers 时, 要求的 byte_stride of buffer in NV12_10, 已经通过 width 传入.
            *byte_stride = (int)width;
        }
    }

    return (int)err;
}

int get_format_requested(buffer_handle_t handle, int* format_requested) {
    auto& mapper = get_service();
    aidl::android::hardware::graphics::common::PixelFormat format;  // *format_requested

    auto err = get_metadata(metadata_type::FormatRequested, handle, format);
    if (err != mapper_error::NONE) {
        ALOGE("Failed to get pixel_format_requested. err : %d", err);
        return (int)err;
    }

#if 0
    // HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED 格式无法获得真实的 hal format
    // 所以这里需要获取 drm_forcc format作转换
    if ((int)format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) {
        uint32_t drm_fourcc = get_fourcc_format(handle);
        uint32_t modifier = get_format_modifier(handle);
        int hal_format = FourccConvertToHalFormat(drm_fourcc, modifier);
        *format_requested = (hal_format > 0 ? hal_format : (int)format);
        ALOGW_IF(LogLevel(android::DBG_DEBUG),
                 "HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED ConvertFormat : drm_fourcc=%c%c%c%c => hal format=%d ",
                 modifier, modifier >> 8, modifier >> 16, modifier >> 24, *format_requested);
    } else {
        *format_requested = (int)format;
    }
#endif
    *format_requested = (int)format;

    return (int)err;
}

int get_usage(buffer_handle_t handle, uint64_t* usage) {
    auto& mapper = get_service();

    auto err = get_metadata(metadata_type::Usage, handle, *usage);
    if (err != mapper_error::NONE) {
        ALOGE("Failed to get pixel_format_requested. err : %d", err);
        return (int)err;
    }

    return (int)err;
}

int get_allocation_size(buffer_handle_t handle, uint64_t* allocation_size) {
    auto& mapper = get_service();

    auto err = get_metadata(metadata_type::AllocationSize, handle, *allocation_size);
    if (err != mapper_error::NONE) {
        ALOGE("Failed to get allocation_size. err : %d", err);
        return (int)err;
    }

    return (int)err;
}

int get_share_fd(buffer_handle_t handle, int* share_fd) {
    auto& mapper = get_service();
    std::vector<int64_t> fds;

    auto err = get_metadata(metadata_type::ArmPlaneFds, handle, fds);
    if (err != mapper_error::NONE) {
        ALOGE("Failed to get plane_fds. err : %d", err);
        return (int)err;
    }
    assert(fds.size() > 0);

    *share_fd = (int)(fds[0]);
    // #error
    ALOGI("cz: %s: %d: *share_fd: %d", __FUNCTION__, __LINE__, *share_fd);

    return (int)err;
}

status_t importBuffer(buffer_handle_t rawHandle, buffer_handle_t* outHandle) {
    auto& mapper = get_service();

    return static_cast<status_t>(stablec_error_to_mapper_error(mapper.v5.importBuffer(rawHandle, outHandle)));
}

status_t freeBuffer(buffer_handle_t handle) {
    auto& mapper = get_service();

    return static_cast<status_t>(stablec_error_to_mapper_error(mapper.v5.freeBuffer(handle)));
}

status_t lock(buffer_handle_t bufferHandle, uint64_t usage, int x, int y, int w, int h, void** outData) {
    auto& mapper = get_service();
    ARect access_region = {
            x,      // 'left'
            y,      // 'top'
            x + w,  // 'right'
            y + h,  // 'bottom'
    };
    int acquire_fence_fd = -1;

    return static_cast<status_t>(stablec_error_to_mapper_error(
            mapper.v5.lock(bufferHandle, usage, access_region, acquire_fence_fd, outData)));
}

void unlock(buffer_handle_t bufferHandle) {
    auto& mapper = get_service();
    int out_release_fence_fd;

    mapper.v5.unlock(bufferHandle, &out_release_fence_fd);
    // 预期 unlock() 返回后, 'out_release_fence_fd' 总是 -1.
}
}  // namespace gralloc5
