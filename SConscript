# RT-Thread building script for component
Import('RTT_ROOT')
Import('rtconfig')

from building import *

cwd = GetCurrentDir()

im2d_source = [
    'core/utils/android_utils/src/android_utils.cpp',
    'core/utils/drm_utils/src/drm_utils.cpp',
    'core/utils/utils.cpp',
    'core/NormalRgaApi.cpp',
    'core/RgaUtils.cpp',
    'core/rga_sync.cpp',
    'im2d_api/src/im2d_log.cpp',
    'im2d_api/src/im2d_debugger.cpp',
    'im2d_api/src/im2d_context.cpp',
    'im2d_api/src/im2d_job.cpp',
    'im2d_api/src/im2d_impl.cpp',
    'im2d_api/src/im2d.cpp'
]

im2d_include = [
    'im2d_api',
    'include',
    'core',
    'core/hardware',
    'core/utils',
    'core/adapter'
]

im2d_3rdparty_include = [
    'core/3rdparty/libdrm/include/drm'
]

im2d_public_include = [
    'im2d_api',
    'include'
]

im2d_3rdparty_include = [
    'core/3rdparty/libdrm/include/drm'
]

src = []
for source in im2d_source:
    src.extend(Glob(source))

LOCAL_CPPPATH = [cwd] + im2d_include + im2d_3rdparty_include
LOCAL_CCFLAGS = ' -x c -DLINUX -DRT_THREAD -DRGA_SYNC_DISABLE -DRGA_UTILS_DRM_DISABLE'
LOCAL_CCFLAGS += ' -lm -D_USE_MATH_DEFINES -Wno-unused-command-line-argument'
LOCAL_CCFLAGS += ' -Wno-maybe-uninitialized -Wno-unused-but-set-variable -Wno-unused-variable -Wno-incompatible-pointer-types -Wno-format -Wno-initializer-overrides'
LOCAL_CCFLAGS += ' -w'

group = DefineGroup('librga', src, depend = ['RT_USING_RGA'], LOCAL_CPPPATH = LOCAL_CPPPATH, LOCAL_CCFLAGS = LOCAL_CCFLAGS, CPPPATH = im2d_public_include)

list = os.listdir(cwd)
for d in list:
    path = os.path.join(cwd, d)
    if os.path.isfile(os.path.join(path, 'SConscript')):
        group = group + SConscript(os.path.join(d, 'SConscript'))

Return('group')
