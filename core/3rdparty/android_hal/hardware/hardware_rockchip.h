#ifndef LIBHARDWARE_HARDWARE_ROCKCHIP_H
#define LIBHARDWARE_HARDWARE_ROCKCHIP_H

__BEGIN_DECLS

#define HWC_BLENDING_DIM            0x0805
#define HWC_BLENDING_CLEAR_HOLE     0x0806
#define HWC_Layer_DEBUG
#define LayerNameLength             60

enum {
    HWC_TOWIN0 = 6,
    HWC_TOWIN1 = 7,
    HWC_LCDC = 8,
    HWC_NODRAW = 9,
    HWC_MIX = 10,
    HWC_MIX_V2 = 11
};

typedef enum {

    /*
     * sRGB color pixel formats:
     *
     * The red, green and blue components are stored in sRGB space, and converted
     * to linear space when read, using the standard sRGB to linear equation:
     *
     * Clinear = Csrgb / 12.92                  for Csrgb <= 0.04045
     *         = (Csrgb + 0.055 / 1.055)^2.4    for Csrgb >  0.04045
     *
     * When written the inverse transformation is performed:
     *
     * Csrgb = 12.92 * Clinear                  for Clinear <= 0.0031308
     *       = 1.055 * Clinear^(1/2.4) - 0.055  for Clinear >  0.0031308
     *
     *
     *  The alpha component, if present, is always stored in linear space and
     *  is left unmodified when read or written.
     *
     */

    /*
    这里定义 rk_private_hal_formats.
    它们的具体 value "不能" 和框架定义的 hal_formats 的 value 重叠.

    框架定义的 hal_formats 被定义在 system/core/libsystem/include/system/ 下的如下文件中:
    graphics-base-v1.0.h
    graphics-base-v1.1.h
    graphics-base-v1.2.h
    graphics-sw.h
    */

    HAL_PIXEL_FORMAT_sRGB_A_8888        = 0xC,  // 12
    HAL_PIXEL_FORMAT_sRGB_X_8888        = 0xD,  // 13

    HAL_PIXEL_FORMAT_YCrCb_NV12         = 0x15, // YUY2, 21
    HAL_PIXEL_FORMAT_YCrCb_NV12_VIDEO   = 0x16, // 22, 和 HAL_PIXEL_FORMAT_RGBA_FP16 的 value 重叠,
                                                // 但目前 HAL_PIXEL_FORMAT_YCrCb_NV12_VIDEO 已经不被实际使用.
    HAL_PIXEL_FORMAT_YCrCb_NV12_10      = 0x17, // YUY2_1obit, 23
    HAL_PIXEL_FORMAT_YCbCr_422_SP_10    = 0x18, // 24
    HAL_PIXEL_FORMAT_YCrCb_420_SP_10    = 0x19, // 25

    HAL_PIXEL_FORMAT_YUV420_8BIT_I      = 0x1A, // 420I 8bit, 26
    HAL_PIXEL_FORMAT_YUV420_10BIT_I     = 0x1B, // 420I 10bit, 27
    HAL_PIXEL_FORMAT_Y210               = 0x1C, // 422I 10bit, 28
    HAL_PIXEL_FORMAT_BGR_888            = 29,
    HAL_PIXEL_FORMAT_NV30               = 30,

    HAL_PIXEL_FORMAT_BPP_1              = 0x30, // 48
    HAL_PIXEL_FORMAT_BPP_2              = 0x31, // 49
    HAL_PIXEL_FORMAT_BPP_4              = 0x32, // 50
    HAL_PIXEL_FORMAT_BPP_8              = 0x33, // 51

    HAL_PIXEL_FORMAT_YUV420_8BIT_RFBC   = 0x200,
    HAL_PIXEL_FORMAT_YUV420_10BIT_RFBC  = 0x201,
    HAL_PIXEL_FORMAT_YUV422_8BIT_RFBC   = 0x202,
    HAL_PIXEL_FORMAT_YUV422_10BIT_RFBC  = 0x203,
    HAL_PIXEL_FORMAT_YUV444_8BIT_RFBC   = 0x204,
    HAL_PIXEL_FORMAT_YUV444_10BIT_RFBC  = 0x205,
} rk_pixel_format_t;

__END_DECLS
#endif // LIBHARDWARE_HARDWARE_ROCKCHIP_H
