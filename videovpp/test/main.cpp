#include <system/graphics.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <va/va.h>
#include <va/va_drmcommon.h>
#include <va/va_vpp.h>
#include <va/va_android.h>
#include <va/va_tpi.h>

#include <hardware/gralloc.h>

#include "VideoVPPBase.h"

enum {
    HAL_PIXEL_FORMAT_NV12_TILED_INTEL = 0x100,
    HAL_PIXEL_FORMAT_NV12_LINEAR_INTEL = 0x101,

// deprecated use HAL_PIXEL_FORMAT_NV12_TILED_INTEL
#if HAL_PIXEL_FORMAT_NV12_DEFINED
    HAL_PIXEL_FORMAT_INTEL_NV12 = HAL_PIXEL_FORMAT_NV12,
#else
    HAL_PIXEL_FORMAT_INTEL_NV12 = HAL_PIXEL_FORMAT_NV12_TILED_INTEL,
#endif

// deprecated use HAL_PIXEL_FORMAT_NV12_LINEAR_INTEL
    HAL_PIXEL_FORMAT_YUV420PackedSemiPlanar_INTEL = 0x7FA00E00,

// deprecated use HAL_PIXEL_FORMAT_NV12_TILED_INTEL
    HAL_PIXEL_FORMAT_YUV420PackedSemiPlanar_Tiled_INTEL = 0x7FA00F00,

};

struct mfx_gralloc_drm_handle_t {
    native_handle_t base;
    int magic;

    int width;
    int height;
    int format;
    int usage;

    int name;
    int stride;

    int data_owner;
    int data;
};

static void usage(const char *me) {
    fprintf(stderr, "color space conversion\n"
            "\t\tusage: %s -i input -o output\n"
                    "\t\t-w width -h height\n",
                    me);

    exit(1);
}

#define VPWRAPPER_NATIVE_DISPLAY  0x18c34078

#define CHECK_VA_STATUS(FUNC) \
    if (vret != VA_STATUS_SUCCESS) {\
        printf("[%d] " FUNC" failed with 0x%x\n", __LINE__, vret);\
        return vret;\
    }


static inline unsigned long GetTickCount()
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL))
        return 0;
    return tv.tv_usec / 1000 + tv.tv_sec * 1000;
}

int main(int argc, char *argv[])
{
    int width = 1280, height = 720;
    int i, j, res;
    const char *me = argv[0];
    char input[128], output[128];
    int has_input = 0;
    int has_output = 0;
    int has_width = 0;
    int has_height = 0;

    while ((res = getopt(argc, argv, "i:o:w:h:")) >= 0) {
        switch (res) {
            case 'i':
                {
                    strcpy(input, optarg);
                    has_input = 1;
                    break;
                }
            case 'o':
                {
                    strcpy(output, optarg);
                    has_output = 1;
                    break;
                }
            case 'w':
                {
                    width = atoi(optarg);
                    has_width = 1;
                    break;
                }
            case 'h':
                {
                    height = atoi(optarg);
                    has_height = 1;
                    break;
                }
            default:
                {
                    usage(me);
                }
        }
    }

    if (!has_input || !has_output || !has_width || !has_height)
        usage(me);

    hw_module_t const* module;
    alloc_device_t *mAllocDev;
    int32_t stride_YUY2, stride_NV12;
    buffer_handle_t handle_YUY2, handle_NV12;
    struct gralloc_module_t *gralloc_module;
    struct mfx_gralloc_drm_handle_t *pGrallocHandle;
    RenderTarget Src, Dst;

    res = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
    gralloc_module = (struct gralloc_module_t*)module;
    res = gralloc_open(module, &mAllocDev);
    res = mAllocDev->alloc(mAllocDev, width, height,
            HAL_PIXEL_FORMAT_YCbCr_422_I,
            GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_TEXTURE,
            &handle_YUY2, &stride_YUY2);
    if (res != 0)
        printf("%d: alloc()\n", __LINE__);
    else {
        pGrallocHandle = (struct mfx_gralloc_drm_handle_t *)handle_YUY2;
        printf("YUY2 %d %d %d\n", pGrallocHandle->width,
                pGrallocHandle->height, stride_YUY2);
        Src.width = pGrallocHandle->width;
        Src.height = pGrallocHandle->height;
        Src.stride = stride_YUY2;
        Src.type = RenderTarget::KERNEL_DRM;
        Src.handle = pGrallocHandle->name;
        Src.rect.x = Src.rect.y = 0;
        Src.rect.width = Src.width;
        Src.rect.height = Src.height;
    }
    res = mAllocDev->alloc(mAllocDev, width, height,
            HAL_PIXEL_FORMAT_NV12_TILED_INTEL,
            GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_TEXTURE,
            &handle_NV12, &stride_NV12);
    if (res != 0)
        printf("%d: alloc()\n", __LINE__);
    else {
        pGrallocHandle = (struct mfx_gralloc_drm_handle_t *)handle_NV12;
        printf("NV12 %d %d %d\n", pGrallocHandle->width,
                pGrallocHandle->height, stride_NV12);
        Dst.width = pGrallocHandle->width;
        Dst.height = pGrallocHandle->height;
        Dst.stride = stride_NV12;
        Dst.type = RenderTarget::KERNEL_DRM;
        Dst.handle = pGrallocHandle->name;
        Dst.rect.x = 0;
        Dst.rect.y = 0;
        Dst.rect.width = Dst.width;
        Dst.rect.height = Dst.height;
    }

    VAStatus vret;

    VideoVPPBase * p = new VideoVPPBase();

    p->start();

    VPParameters *vpp = VPParameters::create(p);

    vret = p->perform(Src, Dst, vpp, false);
    CHECK_VA_STATUS("doVp");

    vret = p->perform(Src, Dst, vpp, false);
    CHECK_VA_STATUS("doVp");

    p->stop();

    mAllocDev->free(mAllocDev, handle_YUY2);
    mAllocDev->free(mAllocDev, handle_NV12);

    gralloc_close(mAllocDev);

    return 0;
}
