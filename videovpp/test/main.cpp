#include <system/graphics.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <va/va.h>
#include <va/va_drmcommon.h>
#include <va/va_vpp.h>
#include <va/va_android.h>
#include <va/va_tpi.h>
#include <assert.h>

#include <hardware/gralloc.h>

#include "VideoVPPBase.h"
#include <intel_bufmgr.h>
#include <drm_fourcc.h>


#define ALIGN(x, align)                  (((x) + (align) - 1) & (~((align) - 1)))

enum {
    HAL_PIXEL_FORMAT_NV12_TILED_INTEL = 0x100,
    HAL_PIXEL_FORMAT_NV12_LINEAR_INTEL = 0x101,

// deprecated use HAL_PIXEL_FORMAT_NV12_TILED_INTEL
#if HAL_PIXEL_FORMAT_NV12_DEFINED
    HAL_PIXEL_FORMAT_INTEL_NV12 = HAL_PIXEL_FORMAT_NV12,
#else
    HAL_PIXEL_FORMAT_INTEL_NV12 = HAL_PIXEL_FORMAT_NV12_TILED_INTEL,
#endif

    HAL_PIXEL_FORMAT_YCrCb_422_H_INTEL = 0x102, // YV16

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
    int pid;    // creator

    mutable int other;                                       // registered owner (pid)
    mutable union { int data1; mutable drm_intel_bo *bo; };  // drm buffer object 
    union { int data2; uint32_t fb; };                       // framebuffer id
    int pitch;                                               // buffer pitch (in bytes)
    int allocWidth;                                          // Allocated buffer width in pixels.
    int allocHeight;                                         // Allocated buffer height in lines.
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
    FILE *fIn, *fOut;

    while ((res = getopt(argc, argv, "i:o:w:h:")) >= 0) {
        switch (res) {
            case 'i':
                {
                    strcpy(input, optarg);
                    has_input = 1;
                    fIn = fopen(input, "r");
                    break;
                }
            case 'o':
                {
                    strcpy(output, optarg);
                    has_output = 1;
                    fOut = fopen(output, "w+");
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

    if (!has_input || !has_output || !has_width || !has_height || !fIn || !fOut)
        usage(me);

    hw_module_t const* module;
    alloc_device_t *mAllocDev;
    int32_t stride_YUY2, stride_NV12;
    buffer_handle_t handle_YUY2, handle_NV12;
    struct gralloc_module_t *gralloc_module;
    struct mfx_gralloc_drm_handle_t *pGrallocHandle;
    RenderTarget Src, Dst;
    void *vaddr[3];

    res = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
    gralloc_module = (struct gralloc_module_t*)module;
    res = gralloc_open(module, &mAllocDev);
    res = mAllocDev->alloc(mAllocDev, width, height,
            //HAL_PIXEL_FORMAT_YCbCr_422_I,
            HAL_PIXEL_FORMAT_YCrCb_422_H_INTEL,
            GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_TEXTURE,// |
            //GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK,
            &handle_YUY2, &stride_YUY2);
    if (res != 0)
        printf("%d: alloc()\n", __LINE__);
    else {
        pGrallocHandle = (struct mfx_gralloc_drm_handle_t *)handle_YUY2;
        printf("YUY2 %d %d %d\n", pGrallocHandle->width,
                pGrallocHandle->height, pGrallocHandle->pitch);
        res =  gralloc_module->lock(gralloc_module, handle_YUY2, 
                GRALLOC_USAGE_SW_WRITE_MASK,
                0, 0, width, height, (void**)&vaddr);
        if (res != 0) {
            printf("lock error\n");
        } else {
            //res = fread(vaddr[0], 1, width * height * 2, fIn);
            for (i = 0; i < height; i++)
                res += fread(vaddr[0] + i * pGrallocHandle->pitch, 1, width, fIn);
            for (i = 0; i < height; i++)
                res += fread(vaddr[0] + (2 * height + i) * pGrallocHandle->pitch, 1,
                        width / 2, fIn);
            for (i = 0; i < height; i++)
                res += fread(vaddr[0] + (height + i) * pGrallocHandle->pitch, 1,
                        width / 2, fIn);
            printf("fread %d\n", res);
            gralloc_module->unlock(gralloc_module, handle_YUY2);
        }
        Src.width = pGrallocHandle->width;
        Src.height = pGrallocHandle->height;
        Src.stride = pGrallocHandle->pitch;
        //Src.format = VA_RT_FORMAT_YUV422;
        //Src.pixel_format = VA_FOURCC_YUY2;
        Src.format = VA_RT_FORMAT_YUV422;
        Src.pixel_format = VA_FOURCC_422H;
        //Src.type = RenderTarget::KERNEL_DRM;
        //Src.handle = pGrallocHandle->name;
        Src.type = RenderTarget::ANDROID_GRALLOC;
        Src.handle = (unsigned int)handle_YUY2;
        Src.rect.x = Src.rect.y = 0;
        Src.rect.width = Src.width;
        Src.rect.height = Src.height;
    }
    res = mAllocDev->alloc(mAllocDev, width, height,
            //HAL_PIXEL_FORMAT_NV12_TILED_INTEL,
            HAL_PIXEL_FORMAT_YCbCr_422_I,
            GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_TEXTURE,// |
            //GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK,
            &handle_NV12, &stride_NV12);
    if (res != 0)
        printf("%d: alloc()\n", __LINE__);
    else {
        pGrallocHandle = (struct mfx_gralloc_drm_handle_t *)handle_NV12;
        printf("NV12 %d %d %d\n", pGrallocHandle->width,
                pGrallocHandle->height, pGrallocHandle->pitch);
        Dst.width = pGrallocHandle->width;
        Dst.height = pGrallocHandle->height;
        Dst.stride = pGrallocHandle->pitch;
        //Dst.format = VA_RT_FORMAT_YUV420;
        //Dst.pixel_format = VA_FOURCC_NV12;
        Dst.format = VA_RT_FORMAT_YUV422;
        Dst.pixel_format = VA_FOURCC_YUY2;
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

    if (vpp) {
        FilterConfig filter;
        vpp->getNR(filter);
        printf("valid %d def %f step %f\n", filter.valid, filter.def, filter.step);
        filter.cur = 0.5;
        vpp->setNR(FilterConfig::LOW);

        vpp->buildfilters();

        vret = p->perform(Src, Dst, vpp, false);
        CHECK_VA_STATUS("doVp");

        vret = p->perform(Src, Dst, vpp, false);
        CHECK_VA_STATUS("doVp");

        vpp->reset(false);
    }

    p->stop();

    {
        res = gralloc_module->lock(gralloc_module, handle_NV12,
                GRALLOC_USAGE_SW_READ_MASK,
                0, 0, width, height, (void**)&vaddr);
        if (res != 0) {
            printf("lock error\n");
        } else {
            unsigned char *pY = (unsigned char*)vaddr[0];
            unsigned char *pUV = pY + stride_NV12 * ALIGN(height, 32);
            //unsigned char *pUV = pY + stride_NV12 * height;
            /*
            for (res =0, i = 0; i < height; i++) {
                res += fwrite(pY, 1, width, fOut);
                pY += stride_NV12;
            }
            printf("fwrite %d\n", res);
            for (res =0, i = 0; i < height / 2; i++) {
                res += fwrite(pUV, 1, width, fOut);
                pUV += stride_NV12;
            }
            */
            for (i = 0; i < height; i++)
                res += fwrite(vaddr[0] + i * Dst.stride, 1, width * 2, fOut);
            printf("fwrite %d\n", res);
            gralloc_module->unlock(gralloc_module, handle_NV12);
        }
    }

    mAllocDev->free(mAllocDev, handle_YUY2);
    mAllocDev->free(mAllocDev, handle_NV12);

    gralloc_close(mAllocDev);

    return 0;
}
