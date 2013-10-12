//#define LOG_NDEBUG 0
#define LOG_TAG "VideoVPPBase"

#include "VideoVPPBase.h"

#define NATIVE_DISPLAY  0x18c34078

#define CHECK_VA_STATUS(FUNC) \
    if (vret != VA_STATUS_SUCCESS) {\
        LOGE("[%d] " FUNC" failed with 0x%x\n", __LINE__, vret);\
        return vret;\
    }

VPParameters::VPParameters(VideoVPPBase *vvb) {
    va_display = vvb->va_display;
    va_context = vvb->va_context;

    vret = init();

    if (vret != VA_STATUS_SUCCESS)
        mInitialized = false;
    else
        mInitialized = true;
}

VPParameters* VPParameters::create(VideoVPPBase *vvb) {
    VPParameters* v = new VPParameters(vvb);

    if (v->mInitialized)
        return v;
    else
        return NULL;
}

VAStatus VPParameters::init() {
    num_supported_filters = VAProcFilterCount;
    vret = vaQueryVideoProcFilters(va_display,
            va_context, supported_filters,
            &num_supported_filters);
    CHECK_VA_STATUS("vaQueryVideoProcFilters");

    for (size_t i = 0; i < num_supported_filters; i++) {
        switch(supported_filters[i]) {
            case VAProcFilterNoiseReduction:
                num_denoise_caps = 1;
                vret = vaQueryVideoProcFilterCaps(va_display, va_context,
                        VAProcFilterNoiseReduction, &denoise_caps, &num_denoise_caps);
                CHECK_VA_STATUS("vaQueryVideoProcFilterCaps");

                nr.valid = true;
                nr.min = denoise_caps.range.min_value;
                nr.max = denoise_caps.range.max_value;
                nr.def = denoise_caps.range.default_value;
                nr.step = denoise_caps.range.step;
                nr.cur = 0.0;
                break;

            case VAProcFilterSharpening:
                num_sharpen_caps = 1;
                vret = vaQueryVideoProcFilterCaps(va_display, va_context,
                        VAProcFilterSharpening, &sharpen_caps, &num_sharpen_caps);
                CHECK_VA_STATUS("vaQueryVideoProcFilterCaps");

                sharpen.valid = true;
                sharpen.min = sharpen_caps.range.min_value;
                sharpen.max = sharpen_caps.range.max_value;
                sharpen.def = sharpen_caps.range.default_value;
                sharpen.step = sharpen_caps.range.step;
                sharpen.cur = 0.0;
                break;

            case VAProcFilterDeblocking:
                num_denoise_caps = 1;
                vret = vaQueryVideoProcFilterCaps(va_display, va_context,
                        VAProcFilterDeblocking, &deblock_caps, &num_deblock_caps);
                CHECK_VA_STATUS("vaQueryVideoProcFilterCaps");

                deblock.valid = true;
                deblock.min = deblock_caps.range.min_value;
                deblock.max = deblock_caps.range.max_value;
                deblock.def = deblock_caps.range.default_value;
                deblock.step = deblock_caps.range.step;
                deblock.cur = 0.0;
                break;

            case VAProcFilterColorBalance:
                num_color_balance_caps = VAProcColorBalanceCount;
                vret = vaQueryVideoProcFilterCaps(va_display, va_context,
                        VAProcFilterColorBalance, &color_balance_caps, &num_color_balance_caps);
                CHECK_VA_STATUS("vaQueryVideoProcFilterCaps");

                for (size_t i = 0; i< num_color_balance_caps; i++) {
                    colorbalance[i].type = color_balance_caps[i].type;
                    colorbalance[i].valid = true;
                    colorbalance[i].min = color_balance_caps[i].range.min_value;
                    colorbalance[i].max = color_balance_caps[i].range.max_value;
                    colorbalance[i].def = color_balance_caps[i].range.default_value;
                    colorbalance[i].step = color_balance_caps[i].range.step;
                    colorbalance[i].cur = 0.0;
                }
                break;

            default:
                break;
        }
    }

    vret = reset(true);
    CHECK_VA_STATUS("reset");

    return vret;
}

VAStatus VPParameters::buildfilters() {
    for (size_t i = 0; i < num_supported_filters; i++) {
        switch (supported_filters[i])  {
            case VAProcFilterNoiseReduction:
                if (nr.cur != -1) {
                    denoise_buf.type = VAProcFilterNoiseReduction;
                    denoise_buf.value = nr.min + nr.cur * nr.step;
                    vret = vaCreateBuffer(va_display, va_context,
                            VAProcFilterParameterBufferType,
                            sizeof(denoise_buf), 1, &denoise_buf, &denoise_buf_id);
                    CHECK_VA_STATUS("vaCreateBuffer");
                    filter_bufs[num_filter_bufs] = denoise_buf_id;
                    num_filter_bufs++;
                }
                break;

            case VAProcFilterDeblocking:
                if (deblock.cur != -1) {
                    deblock_buf.type = VAProcFilterDeblocking;
                    deblock_buf.value = deblock.min + deblock.cur * deblock.step;
                    vret = vaCreateBuffer(va_display, va_context,
                            VAProcFilterParameterBufferType,
                            sizeof(deblock_buf), 1, &deblock_buf, &deblock_buf_id);
                    CHECK_VA_STATUS("vaCreateBuffer");
                    filter_bufs[num_filter_bufs] = deblock_buf_id;
                    num_filter_bufs++;
                }
                break;

            case VAProcFilterSharpening:
                if (sharpen.cur != -1) {
                    sharpen_buf.type = VAProcFilterSharpening;
                    sharpen_buf.value = sharpen.cur;
                    vret = vaCreateBuffer(va_display, va_context,
                            VAProcFilterParameterBufferType,
                            sizeof(sharpen_buf), 1, &sharpen_buf, &sharpen_buf_id);
                    CHECK_VA_STATUS("vaCreateBuffer");
                    filter_bufs[num_filter_bufs] = sharpen_buf_id;
                    num_filter_bufs++;
                }
                break;

            case VAProcFilterColorBalance:
                break;

            default:
                break;
        }
    }

    return vret;
}

VAStatus VPParameters::reset(bool start) {
    for (size_t i = 0; i < VAProcFilterCount; i++) {
        if (start) {
            filter_bufs[i] = VA_INVALID_ID;
        } else {
            if (filter_bufs[i] != VA_INVALID_ID) {
                vret = vaDestroyBuffer(va_display, filter_bufs[i]);
                CHECK_VA_STATUS("vaDestroyBuffer");
                filter_bufs[i] = VA_INVALID_ID;
            }
        }
    }
    num_filter_bufs = 0;

    sharpen_buf_id = denoise_buf_id = deblock_buf_id = balance_buf_id = VA_INVALID_ID;
    memset(&deblock_buf, 0, sizeof(VAProcFilterParameterBuffer));
    memset(&sharpen_buf, 0, sizeof(VAProcFilterParameterBuffer));
    memset(&denoise_buf, 0, sizeof(VAProcFilterParameterBuffer));
    memset(&balance_buf, 0,
            sizeof(VAProcFilterParameterBufferColorBalance)* VAProcColorBalanceCount);

    nr.cur = deblock.cur = sharpen.cur = -1;
    for (size_t i = 0; i < VAProcColorBalanceCount; i++)
        colorbalance[i].cur = -1;

    return vret;
}

VPParameters::~VPParameters() {
    reset(false);
}

VideoVPPBase::VideoVPPBase()
    : mInitialized(false),
      width(1280),
      height(720),
      va_display(NULL),
      va_config(VA_INVALID_ID),
      va_context(VA_INVALID_ID),
      vpp_pipeline_buf(VA_INVALID_ID),
      SrcSurf(VA_INVALID_SURFACE),
      DstSurf(VA_INVALID_SURFACE) {

}

VAStatus VideoVPPBase::start() {
    if (mInitialized)
        return VA_STATUS_SUCCESS;

    int va_major_version, va_minor_version;
    unsigned int nativeDisplay = NATIVE_DISPLAY;
    VAConfigAttrib vaAttrib;

    va_display = vaGetDisplay(&nativeDisplay);

    vret = vaInitialize(va_display, &va_major_version, &va_minor_version);
    CHECK_VA_STATUS("vaInitialize");

    vaAttrib.type = VAConfigAttribRTFormat;
    vaAttrib.value = VA_RT_FORMAT_YUV420;
    vret = vaCreateConfig(va_display, VAProfileNone,
            VAEntrypointVideoProc, &vaAttrib, 1, &va_config);
    CHECK_VA_STATUS("vaCreateConfig");

    vret = vaCreateContext(va_display, va_config, width,
            height, 0, NULL, 0, &va_context);
    CHECK_VA_STATUS("vaCreateContext");

    mInitialized = true;

    return vret;
}

VAStatus VideoVPPBase::stop() {
    if (!mInitialized)
        return VA_STATUS_SUCCESS;

    int c = SrcSurfHandleMap.size();
    for (int i = 0; i < c; i++) {
        SrcSurf = SrcSurfHandleMap.valueAt(i);
        if (SrcSurf != VA_INVALID_SURFACE) {
            vret = vaDestroySurfaces(va_display, &SrcSurf, 1);
            CHECK_VA_STATUS("vaDestroySurfaces");
        }
        LOGV("remove src surf %x\n", SrcSurf);
    }
    SrcSurfHandleMap.clear();

    c = DstSurfHandleMap.size();
    for (int i = 0; i < c; i++) {
        DstSurf = DstSurfHandleMap.valueAt(i);
        if (DstSurf != VA_INVALID_SURFACE) {
            vret = vaDestroySurfaces(va_display, &DstSurf, 1);
            CHECK_VA_STATUS("vaDestroySurfaces");
        }
        LOGV("remove dst surf %x\n", DstSurf);
    }
    DstSurfHandleMap.clear();

    if (vpp_pipeline_buf != VA_INVALID_ID) {
        vret = vaDestroyBuffer(va_display, vpp_pipeline_buf);
        CHECK_VA_STATUS("vaDestroyBuffer");
        vpp_pipeline_buf = VA_INVALID_ID;
    }

    if (va_context != VA_INVALID_ID) {
        vret = vaDestroyContext(va_display, va_context);
        CHECK_VA_STATUS("vaDestroyContext");
        va_context = VA_INVALID_ID;
    }

    if (va_config != VA_INVALID_ID) {
        vret = vaDestroyConfig(va_display, va_config);
        CHECK_VA_STATUS("vaDestroyConfig");
        va_config = VA_INVALID_ID;
    }

    if (va_display != NULL) {
        vret = vaTerminate(va_display);
        CHECK_VA_STATUS("vaTerminate");
        va_display = NULL;
    }

    mInitialized = false;

    return vret;
}

VAStatus VideoVPPBase::_CreateSurfaceFromGrallocHandle(RenderTarget rt, VASurfaceID *surf) {
    unsigned int buffer;
    VASurfaceAttrib SurfAttrib;
    VASurfaceAttribExternalBuffers  SurfExtBuf;

    SurfExtBuf.pixel_format = rt.pixel_format;
    SurfExtBuf.width = rt.width;
    SurfExtBuf.height = rt.height;
    SurfExtBuf.pitches[0] = rt.stride;
    buffer = rt.handle;
    SurfExtBuf.buffers = (unsigned long*)&buffer;
    SurfExtBuf.num_buffers = 1;
    if (rt.type == RenderTarget::KERNEL_DRM)
        SurfExtBuf.flags = VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM;
    else
        SurfExtBuf.flags = VA_SURFACE_ATTRIB_MEM_TYPE_ANDROID_GRALLOC;

    SurfAttrib.type = (VASurfaceAttribType)VASurfaceAttribExternalBufferDescriptor;
    SurfAttrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
    SurfAttrib.value.type = VAGenericValueTypePointer;
    SurfAttrib.value.value.p = &SurfExtBuf;

    vret = vaCreateSurfaces(va_display, rt.format,
            rt.width, rt.height, surf, 1,
            &SurfAttrib, 1);
    CHECK_VA_STATUS("vaCreateSurfaces");

    return vret;
}

VAStatus VideoVPPBase::_perform(VASurfaceID SrcSurf, VARectangle SrcRect,
        VASurfaceID DstSurf, VARectangle DstRect, bool no_wait) {
    vpp_param.surface = SrcSurf;
    vpp_param.output_region = &DstRect;
    vpp_param.surface_region = &SrcRect;
    vpp_param.surface_color_standard = VAProcColorStandardBT601;
    vpp_param.output_background_color = 0;
    vpp_param.output_color_standard = VAProcColorStandardBT601;
    vpp_param.filter_flags = VA_FRAME_PICTURE;
    vpp_param.filters = filter_bufs;
    vpp_param.num_filters = num_filter_bufs;
    vpp_param.forward_references = NULL;
    vpp_param.num_forward_references = 0;
    vpp_param.backward_references = NULL;
    vpp_param.num_backward_references = 0;
    vpp_param.blend_state = NULL;
    vpp_param.rotation_state = VA_ROTATION_NONE;
    vpp_param.mirror_state = VA_MIRROR_NONE;

    vret = vaCreateBuffer(va_display, va_context,
            VAProcPipelineParameterBufferType,
            sizeof(VAProcPipelineParameterBuffer),
            1, &vpp_param, &vpp_pipeline_buf);
    CHECK_VA_STATUS("vaCreateBuffer");

    vret = vaBeginPicture(va_display, va_context, DstSurf);
    CHECK_VA_STATUS("vaBeginPicture");

    vret = vaRenderPicture(va_display, va_context, &vpp_pipeline_buf, 1);
    CHECK_VA_STATUS("vaRenderPicture");

    vret = vaEndPicture(va_display, va_context);
    CHECK_VA_STATUS("vaEndPicture");

    if (!no_wait) {
        vret = vaSyncSurface(va_display, DstSurf);
        CHECK_VA_STATUS("vaSyncSurface");
    }

    vret = vaDestroyBuffer(va_display, vpp_pipeline_buf);
    CHECK_VA_STATUS("vaDestroyBuffer");
    vpp_pipeline_buf = VA_INVALID_ID;

    return vret;
}

VAStatus VideoVPPBase::perform(RenderTarget Src, RenderTarget Dst, VPParameters *vpp, bool no_wait) {
    if (!mInitialized) {
        vret = start();
        CHECK_VA_STATUS("start");
    }

    ssize_t i = SrcSurfHandleMap.indexOfKey(Src.handle);
    if (i >= 0) {
        SrcSurf = SrcSurfHandleMap.valueAt(i);
    } else {
        vret = _CreateSurfaceFromGrallocHandle(Src, &SrcSurf);
        CHECK_VA_STATUS("_CreateSurfaceFromGrallocHandle");
        SrcSurfHandleMap.add(Src.handle, SrcSurf);
        LOGV("add src surface %x\n", SrcSurf);
    }

    i = DstSurfHandleMap.indexOfKey(Dst.handle);
    if (i >= 0) {
        DstSurf = DstSurfHandleMap.valueAt(i);
    } else {
        vret = _CreateSurfaceFromGrallocHandle(Dst, &DstSurf);
        CHECK_VA_STATUS("_CreateSurfaceFromGrallocHandle");
        DstSurfHandleMap.add(Dst.handle, DstSurf);
        LOGV("add dst surface %x\n", DstSurf);
    }

    if (vpp != NULL && (vpp->num_filter_bufs > 0 && vpp->num_filter_bufs < VAProcFilterCount)) {
        memcpy(filter_bufs, vpp->filter_bufs, sizeof(VABufferID) * vpp->num_filter_bufs);
        num_filter_bufs = vpp->num_filter_bufs;
    } else {
        num_filter_bufs = 0;
    }

    vret = _perform(SrcSurf, Src.rect, DstSurf, Dst.rect, no_wait);
    CHECK_VA_STATUS("_perform");

    return vret;
}

VideoVPPBase::~VideoVPPBase() {
    stop();
}
