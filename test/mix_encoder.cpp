#include <va/va_tpi.h>
#include <va/va_android.h>
#include <VideoEncoderHost.h>
#include <stdio.h>
#include <getopt.h>
#include <IntelMetadataBuffer.h>

#include <private/gui/ComposerService.h>
#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/IGraphicBufferAlloc.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>

#include <ui/PixelFormat.h>
#include <hardware/gralloc.h>
#include <hal/hal_public.h>
#include "loadsurface.h"

static const char *AVC_MIME_TYPE = "video/h264";
static const char *MPEG4_MIME_TYPE = "video/mpeg4";
static const char *H263_MIME_TYPE = "video/h263";
static const char *VPX_MIME_TYPE = "video/x-webm";

static const int box_width = 128;

static VideoParamsAVC gVideoParamsAVC;
static IVideoEncoder *gVideoEncoder = NULL;
static VideoParamsCommon gEncoderParams;
static VideoParamsStoreMetaDataInBuffers gStoreMetaDataInBuffers;
static VideoRateControl gRC = RATE_CONTROL_CBR;

static int gCodec = 0;  //0: H264, 1: MPEG4, 2: H263 3: VPX
static int gRCMode = 1; //0: NO_RC, 1: CBR, 2: VBR, 3: VCM
static int gBitrate = 1280000;

static bool gSyncEncMode = false;
static uint32_t gEncFrames = 15;
static const int gSrcFrames = 15;

static uint32_t gAllocatedSize;
static uint32_t gSrcWidth = 1280;
static uint32_t gSrcHeight = 720;
static uint32_t gSrcStride = 1280;
static uint32_t gFrameRate = 30;
static uint32_t gEncodeWidth = 0;
static uint32_t gEncodeHeight = 0;

static char* gFile = (char*)"out.264";

static uint32_t gMode = 0; //0:Camera malloc , 1: WiDi clone, 2: WiDi ext, 3: WiDi user, 4: Raw, 5: SurfaceMediaSource
static const char* gModeString[10] = {"Camera malloc", "WiDi clone", "WiDi ext", "WiDi user", "Raw", "GrallocSource(Composer)", "GrallocSource(Gralloc)", "GrallocSource(Camerav2)", "MappingSurfaceForCI", "Camera malloc For Extra Value"};
static const char* gRCModeString[4] ={"NO_RC", "CBR", "VBR", "VCM"};


static uint32_t gOutPutFormat = 0;
static const char* gOutPutFormatString[6] = {"OUTPUT_EVERYTHING","OUTPUT_CODEC_DATA","OUTPUT_FRAME_DATA","OUTPUT_ONE_NAL","OUTPUT_ONE_NAL_WITHOUT_STARTCODE","OUTPUT_LENGTH_PREFIXED"};

static uint32_t gOutPutBufferSize = 1;
//for uploading src pictures, also for Camera malloc, WiDi clone, raw mode usrptr storage
static uint8_t* gUsrptr[gSrcFrames];
static uint8_t* gMallocPtr[gSrcFrames];

//for metadatabuffer transfer
static IntelMetadataBuffer* gIMB[gSrcFrames] = {NULL};

//for WiDi user mode
static VADisplay gVADisplay;
static VASurfaceID gSurface[gSrcFrames];

//for WiDi ext mode
static uint32_t gkBufHandle[gSrcFrames];

//for gfxhandle
static sp<IGraphicBufferAlloc> gGraphicBufferAlloc;
static sp<GraphicBuffer> gGraphicBuffer[gSrcFrames];

static int ev1[10];

struct VideoConfigTypeIDRReq: VideoParamConfigSet {

    VideoConfigTypeIDRReq() {
        type = VideoConfigTypeIDRRequest;
        size = sizeof(VideoConfigTypeIDRReq);
    }
};

extern "C" {
VAStatus vaLockSurface(VADisplay dpy,
    VASurfaceID surface,
    unsigned int *fourcc,
    unsigned int *luma_stride,
    unsigned int *chroma_u_stride,
    unsigned int *chroma_v_stride,
    unsigned int *luma_offset,
    unsigned int *chroma_u_offset,
    unsigned int *chroma_v_offset,
    unsigned int *buffer_name,
    void **buffer
);

VAStatus vaUnlockSurface(VADisplay dpy,
    VASurfaceID surface
);
}

static hw_module_t const *gModule;
static gralloc_module_t const *gAllocMod; /* get by force hw_module_t */
static alloc_device_t  *gAllocDev; /* get by gralloc_open */


static int gCodeCoverageTestErrorCase = 0;
static void gfx_init()
{
    int err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &gModule);
    if (err) {
        printf("FATAL: can't find the %s module", GRALLOC_HARDWARE_MODULE_ID);
        exit(-1);
    }

    gAllocMod = (gralloc_module_t const *)gModule;

    err = gralloc_open(gModule, &gAllocDev);
    if (err) {
        printf("FATAL: gralloc open failed\n");
        exit(-1);
    }

}

static int gfx_alloc(uint32_t w, uint32_t h, int format,
          int usage, buffer_handle_t* handle, int32_t* stride)
{
    int err;

    err = gAllocDev->alloc(gAllocDev, w, h, format, usage, handle, stride);
    if (err) {
        printf("alloc(%u, %u, %d, %08x, ...) failed %d (%s)\n",
               w, h, format, usage, err, strerror(-err));
        exit(-1);
    }

    return err;
}

static int gfx_free(buffer_handle_t handle)
{
    int err;

    err = gAllocDev->free(gAllocDev, handle);
    if (err) {
        printf("free(...) failed %d (%s)\n", err, strerror(-err));
        exit(-1);
    }

    return err;
}

static int gfx_lock(buffer_handle_t handle,
                int usage, int left, int top, int width, int height,
                void** vaddr)
{
    int err;

    err = gAllocMod->lock(gAllocMod, handle, usage,
                          left, top, width, height,
                          vaddr);

    if (err){
        printf("lock(...) failed %d (%s)", err, strerror(-err));
        exit(-1);
    }

    return err;
}


static int gfx_unlock(buffer_handle_t handle)
{
    int err;

    err = gAllocMod->unlock(gAllocMod, handle);
    if (err) {
        printf("unlock(...) failed %d (%s)\n", err, strerror(-err));
        exit(-1);
    }

    return err;
}

Encode_Status SetVideoEncoderParam() {

    Encode_Status ret = ENCODE_SUCCESS;

    ret = gVideoEncoder->getParameters(&gEncoderParams);
    CHECK_ENCODE_STATUS("getParameters");

    printf("Set Encoding Width=%d, Height=%d\n", gEncodeWidth, gEncodeHeight);
    gEncoderParams.resolution.height = gEncodeHeight;
    gEncoderParams.resolution.width = gEncodeWidth;
    gEncoderParams.frameRate.frameRateDenom = 1;
    gEncoderParams.frameRate.frameRateNum = gFrameRate;
    gEncoderParams.rcMode = gRC;
    gEncoderParams.syncEncMode = gSyncEncMode;

    switch(gCodec)
    {
        case 0:
            break;
        case 1:
            gEncoderParams.profile = (VAProfile)VAProfileMPEG4Simple;
            break;
        case 2:
            gEncoderParams.profile = (VAProfile)VAProfileH263Baseline;
            break;
        default:
            break;
    }

    gEncoderParams.rcParams.bitRate = gBitrate;
#if 0
    gEncoderParams->intraPeriod = 15;
    gEncoderParams->rawFormat = RAW_FORMAT_NV12;
    gEncoderParams->rcParams.initQP = 0;
    gEncoderParams->rcParams.minQP = 0;
    gEncoderParams->rcParams.windowSize = 0;
    gEncoderParams->rcParams.targetPercentage = 0;
    gEncoderParams->rcParams.bitRate = 10000;
    gEncoderParams->rcMode = RATE_CONTROL_CBR;
    gEncoderParams->refreshType = VIDEO_ENC_NONIR;
#endif

    ret = gVideoEncoder->setParameters(&gEncoderParams);
    CHECK_ENCODE_STATUS("setParameters VideoParamsCommon");

    gVideoEncoder->getParameters(&gVideoParamsAVC);
    gVideoParamsAVC.crop.TopOffset = 1;
    gVideoParamsAVC.VUIFlag = 1;
    gVideoParamsAVC.SAR.SarWidth = 1;
    gVideoEncoder->setParameters(&gVideoParamsAVC);

    VideoParamsStoreMetaDataInBuffers tmpStoreMetaDataInBuffers;
    gVideoEncoder->getParameters(&tmpStoreMetaDataInBuffers);
    memset(&tmpStoreMetaDataInBuffers,0x00,sizeof(VideoParamsStoreMetaDataInBuffers));
    gVideoEncoder->getParameters(&tmpStoreMetaDataInBuffers);
    gVideoEncoder->setParameters(&tmpStoreMetaDataInBuffers);
#if 0
    VideoParamsUpstreamBuffer tmpVideoParamsUpstreamBuffer;
    tmpVideoParamsUpstreamBuffer.bufCnt = 0;
    gVideoEncoder->setParameters(&tmpVideoParamsUpstreamBuffer);

    tmpVideoParamsUpstreamBuffer.bufCnt = gSrcFrames;
    tmpVideoParamsUpstreamBuffer.bufAttrib = NULL;
    gVideoEncoder->setParameters(&tmpVideoParamsUpstreamBuffer);
/*
    ExternalBufferAttrib attrib;
    tmpVideoParamsUpstreamBuffer.bufCnt = gSrcFrames;
    tmpVideoParamsUpstreamBuffer.bufAttrib = &attrib;
    tmpVideoParamsUpstreamBuffer.bufferMode = BUFFER_LAST;
    gVideoEncoder->setParameters(&tmpVideoParamsUpstreamBuffer);
*/
    VideoParamsUsrptrBuffer tmpVideoParamsUsrptrBuffer;
    tmpVideoParamsUsrptrBuffer.width = 0;
    gVideoEncoder->getParameters(&tmpVideoParamsUsrptrBuffer);
#endif
        // VideoConfigTypeCyclicFrameInterval setConfig and getConfig
        VideoConfigCyclicFrameInterval configCyclicFrameInterval;
        configCyclicFrameInterval.cyclicFrameInterval = 30;
        gVideoEncoder->setConfig(&configCyclicFrameInterval);
        gVideoEncoder->getConfig(&configCyclicFrameInterval);

        // for VideoConfigTypeAVCIntraPeriod derivedSetConfig && derivedGetConfig
        VideoConfigAVCIntraPeriod configAVCIntraPeriod;
        gVideoEncoder->getConfig(&configAVCIntraPeriod);
        configAVCIntraPeriod.ipPeriod = 1;
        configAVCIntraPeriod.intraPeriod = 30;
        configAVCIntraPeriod.idrInterval = 1;
        gVideoEncoder->setConfig(&configAVCIntraPeriod);
        VideoConfigTypeIDRReq tmpVideoConfigTypeIDRReq;
        gVideoEncoder->setConfig(&tmpVideoConfigTypeIDRReq);
 

    if (gMode != 4)
    {
        gStoreMetaDataInBuffers.isEnabled = true;

        ret = gVideoEncoder->setParameters(&gStoreMetaDataInBuffers);
        CHECK_ENCODE_STATUS("setParameters StoreMetaDataInBuffers");
    }

    return ret;
}

static int YUV_generator_planar(int width, int height,
                  unsigned char *Y_start, int Y_pitch,
		  unsigned char *U_start, int U_pitch,
                  unsigned char *V_start, int V_pitch,
		  int UV_interleave)
{
    static int row_shift = 0;
    int row;

    /* copy Y plane */
    for (row=0;row<height;row++) {
        unsigned char *Y_row = Y_start + row * Y_pitch;
        int jj, xpos, ypos;

        ypos = (row / box_width) & 0x1;

        for (jj=0; jj<width; jj++) {
            xpos = ((row_shift + jj) / box_width) & 0x1;

            if ((xpos == 0) && (ypos == 0))
                Y_row[jj] = 0xeb;
            if ((xpos == 1) && (ypos == 1))
                Y_row[jj] = 0xeb;

            if ((xpos == 1) && (ypos == 0))
                Y_row[jj] = 0x10;
            if ((xpos == 0) && (ypos == 1))
                Y_row[jj] = 0x10;
        }
    }

    /* copy UV data */
    for( row =0; row < height/2; row++) {
        if (UV_interleave) {
            unsigned char *UV_row = U_start + row * U_pitch;
            memset (UV_row,0x80,width);
        } else {
            unsigned char *U_row = U_start + row * U_pitch;
            unsigned char *V_row = V_start + row * V_pitch;

            memset (U_row,0x80,width/2);
            memset (V_row,0x80,width/2);
        }
    }

    row_shift += 2;
    if (row_shift==box_width) row_shift = 0;

    YUV_blend_with_pic(width,height,
                       Y_start, Y_pitch,
                       U_start, U_pitch,
                       V_start, V_pitch,
                       1, 70);
 

    return 0;
}

//malloc external memory, and not need to set into encoder before start()
void MallocExternalMemoryWithExtraValues()
{
    uint32_t size = gSrcWidth * gSrcHeight * 3 /2;

    ValueInfo* vinfo = new ValueInfo;
    vinfo->mode = MEM_MODE_MALLOC;
    vinfo->handle = 0;
    vinfo->size = size;
    vinfo->width = gSrcWidth;
    vinfo->height = gSrcHeight;
    vinfo->lumaStride = gSrcStride;
    vinfo->chromStride = gSrcStride;
    vinfo->format = STRING_TO_FOURCC("NV12");
    vinfo->s3dformat = 0xFFFFFFFF;

    for(int i = 0; i < gSrcFrames; i ++)
    {
        gUsrptr[i] = (uint8_t*)malloc(size);

        gIMB[i] = new IntelMetadataBuffer(MetadataBufferTypeCameraSource, (int32_t)gUsrptr[i]);

        gIMB[i]->SetValueInfo(vinfo);
    }
    delete vinfo;

    gIMB[0]->SetExtraValues(ev1, 10);
}
//malloc external memory, and not need to set into encoder before start()
void MallocExternalMemory()
{
    uint32_t size = gSrcStride * gSrcHeight * 3 /2;

    ValueInfo* vinfo = new ValueInfo;
    vinfo->mode = MEM_MODE_MALLOC;
    vinfo->handle = 0;
    vinfo->size = size;
    vinfo->width = gSrcWidth;
    vinfo->height = gSrcHeight;
    vinfo->lumaStride = gSrcStride;
    vinfo->chromStride = gSrcStride;
    vinfo->format = STRING_TO_FOURCC("NV12");
    vinfo->s3dformat = 0xFFFFFFFF;

    for(int i = 0; i < gSrcFrames; i ++)
    {
        gMallocPtr[i] = (uint8_t*)malloc(size + 4095);
        gUsrptr[i] = (uint8_t*)((((uint32_t )gMallocPtr[i] + 4095) / 4096 ) * 4096);

        gIMB[i] = new IntelMetadataBuffer(MetadataBufferTypeCameraSource, (int32_t)gUsrptr[i]);

        gIMB[i]->SetValueInfo(vinfo);
    }
    delete vinfo;
}

//apply memory from encoder, and get usrptr to upload pictures
void GetAllUsrptr()
{
    Encode_Status ret = ENCODE_SUCCESS;
    VideoParamsUsrptrBuffer paramsUsrptrBuffer;

    paramsUsrptrBuffer.type = VideoParamsTypeUsrptrBuffer;
    paramsUsrptrBuffer.size =  sizeof(VideoParamsUsrptrBuffer);
    paramsUsrptrBuffer.expectedSize = gSrcWidth * gSrcHeight * 3 / 2;
    paramsUsrptrBuffer.format = STRING_TO_FOURCC("NV12");
    paramsUsrptrBuffer.width = gSrcWidth;
    paramsUsrptrBuffer.height = gSrcHeight;

    for(int i = 0; i < gSrcFrames; i ++)
    {
        ret = gVideoEncoder->getParameters(&paramsUsrptrBuffer);
        if(ret != ENCODE_SUCCESS  ) {
            printf("could not allocate input surface from the encoder %d", ret);
            ret = ENCODE_NO_MEMORY;
            break;
        }
        gAllocatedSize = paramsUsrptrBuffer.actualSize;
        gUsrptr[i] = paramsUsrptrBuffer.usrPtr;
        gSrcStride = paramsUsrptrBuffer.stride;

        gIMB[i] = new IntelMetadataBuffer(MetadataBufferTypeEncoder, (int32_t)gUsrptr[i]);
    }

}

void CreateUserSurfaces(int mode)
{
    unsigned int display = 0;
    int majorVersion = -1;
    int minorVersion = -1;
    VAStatus vaStatus;

    gVADisplay = vaGetDisplay(&display);

    if (gVADisplay == NULL) {
        printf("vaGetDisplay failed.");
    }

    vaStatus = vaInitialize(gVADisplay, &majorVersion, &minorVersion);
    if (vaStatus != VA_STATUS_SUCCESS) {
        printf( "Failed vaInitialize, vaStatus = %d\n", vaStatus);
    }

    VASurfaceAttributeTPI attribute_tpi;

    attribute_tpi.size = gSrcWidth * gSrcHeight * 3 /2;
    attribute_tpi.luma_stride = gSrcWidth;
    attribute_tpi.chroma_u_stride = gSrcWidth;
    attribute_tpi.chroma_v_stride = gSrcWidth;
    attribute_tpi.luma_offset = 0;
    attribute_tpi.chroma_u_offset = gSrcWidth * gSrcHeight;
    attribute_tpi.chroma_v_offset = gSrcWidth * gSrcHeight;
    attribute_tpi.pixel_format = VA_FOURCC_NV12;
    attribute_tpi.type = VAExternalMemoryNULL;

    vaStatus = vaCreateSurfacesWithAttribute(gVADisplay, gSrcWidth, gSrcHeight, VA_RT_FORMAT_YUV420,
            gSrcFrames, gSurface, &attribute_tpi);

    if (vaStatus != VA_STATUS_SUCCESS) {
        printf( "Failed vaCreateSurfaces, vaStatus = %d\n", vaStatus);
    }

    VideoParamsUpstreamBuffer upstreamParam;
    if (mode == 0)
        upstreamParam.bufferMode = BUFFER_SHARING_SURFACE;
    else
        upstreamParam.bufferMode = BUFFER_SHARING_KBUFHANDLE;

    ExternalBufferAttrib attrib;
    attrib.realWidth = gSrcWidth;
    attrib.realHeight = gSrcHeight;
    attrib.lumaStride = gSrcStride;
    attrib.chromStride = gSrcStride;
    attrib.format = VA_FOURCC_NV12;
    upstreamParam.bufAttrib = &attrib;

    uint32_t *list = new uint32_t[gSrcFrames];
    if (mode == 1){
        uint32_t fourCC = 0;
        uint32_t lumaStride = 0;
        uint32_t chromaUStride = 0;
        uint32_t chromaVStride = 0;
        uint32_t lumaOffset = 0;
        uint32_t chromaUOffset = 0;
        uint32_t chromaVOffset = 0;

        for(int i = 0; i < gSrcFrames; i++) {
            vaStatus = vaLockSurface(
                    gVADisplay, (VASurfaceID)gSurface[i],
                    &fourCC, &lumaStride, &chromaUStride, &chromaVStride,
                    &lumaOffset, &chromaUOffset, &chromaVOffset, &gkBufHandle[i], NULL);
            if (vaStatus != VA_STATUS_SUCCESS) {
                 printf( "Failed vaLockSurface, vaStatus = %d\n", vaStatus);
            }
#if 0
            printf("lumaStride = %d", lumaStride);
            printf("chromaUStride = %d", chromaUStride);
            printf("chromaVStride = %d", chromaVStride);
            printf("lumaOffset = %d", lumaOffset);
            printf("chromaUOffset = %d", chromaUOffset);
            printf("chromaVOffset = %d", chromaVOffset);
            printf("kBufHandle = 0x%08x", gkBufHandle[i]);
            printf("fourCC = %d\n", fourCC);
#endif
            vaStatus = vaUnlockSurface(gVADisplay, (VASurfaceID)gSurface[i]);
            list[i] = gkBufHandle[i];
        }

    }else{

        for (int i = 0; i < gSrcFrames; i++)
            list[i] = gSurface[i];
    }

    upstreamParam.bufList = list;
    upstreamParam.bufCnt = gSrcFrames;
    upstreamParam.display = gVADisplay;
    Encode_Status ret;
    ret = gVideoEncoder->setParameters((VideoParamConfigSet *)&upstreamParam);
    if (ret != ENCODE_SUCCESS) {
        printf("Failed setParameters, Status = %d\n", ret);
    }
    delete list;

    //get usrptr for uploading src pictures
    VAImage surface_image;
    for (int i=0; i<gSrcFrames; i++) {
        vaStatus = vaDeriveImage(gVADisplay, gSurface[i], &surface_image);
        if (vaStatus != VA_STATUS_SUCCESS) {
            printf("Failed vaDeriveImage, vaStatus = %d\n", vaStatus);
        }

        vaMapBuffer(gVADisplay, surface_image.buf, (void**)&gUsrptr[i]);
        if (vaStatus != VA_STATUS_SUCCESS) {
            printf("Failed vaMapBuffer, vaStatus = %d\n", vaStatus);
        }

        vaUnmapBuffer(gVADisplay, surface_image.buf);
        vaDestroyImage(gVADisplay, surface_image.image_id);

        if (mode == 0)
            gIMB[i] = new IntelMetadataBuffer(MetadataBufferTypeUser, gSurface[i]);
        else
            gIMB[i] = new IntelMetadataBuffer(MetadataBufferTypeUser, gkBufHandle[i]);
    }
}

void CreateSurfaceMappingForCI()
{
    uint32_t size = gSrcWidth * gSrcHeight * 3 /2;

    ValueInfo* vinfo = new ValueInfo;
    vinfo->mode = MEM_MODE_CI;
    vinfo->handle = 0;
    vinfo->size = size;
    vinfo->width = gSrcWidth;
    vinfo->height = gSrcHeight;
    vinfo->lumaStride = gSrcStride;
    vinfo->chromStride = gSrcStride;
    vinfo->format = STRING_TO_FOURCC("NV12");
    vinfo->s3dformat = 0xFFFFFFFF;

    for(int i = 0; i < gSrcFrames; i ++)
    {
        gUsrptr[i] = (uint8_t*)malloc(size);

        gIMB[i] = new IntelMetadataBuffer(MetadataBufferTypeCameraSource, (int32_t)gUsrptr[i]);

        gIMB[i]->SetValueInfo(vinfo);
    }
    delete vinfo;
}
void CreateGfxhandle(int color)
{
    sp<ISurfaceComposer> composer(ComposerService::getComposerService());
    gGraphicBufferAlloc = composer->createGraphicBufferAlloc();

    uint32_t usage = GraphicBuffer::USAGE_HW_TEXTURE | GraphicBuffer::USAGE_HW_COMPOSER;
    int format = HAL_PIXEL_FORMAT_NV12;
    if (color == 1)
        format = 0x7FA00E00; // = OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar in OMX_IVCommon.h

    int32_t error;

    for(int i = 0; i < gSrcFrames; i ++)
    {
        sp<GraphicBuffer> graphicBuffer(
                gGraphicBufferAlloc->createGraphicBuffer(
                                gSrcWidth, gSrcHeight, format, usage, &error));

        gGraphicBuffer[i] = graphicBuffer;
        graphicBuffer->lock(usage | GraphicBuffer::USAGE_SW_WRITE_OFTEN, (void**)(&gUsrptr[i]));
        gIMB[i] = new IntelMetadataBuffer(MetadataBufferTypeGrallocSource, (int32_t)gGraphicBuffer[i]->handle);
        graphicBuffer->unlock();

        IMG_native_handle_t* h = (IMG_native_handle_t*) gGraphicBuffer[i]->handle;
        gSrcStride = h->iWidth;
        gSrcHeight = h->iHeight;
    }
}

void CreateGralloc(int color)
{
    int usage = GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_COMPOSER;
    int format = HAL_PIXEL_FORMAT_NV12;
    if (color == 1)
        format = 0x7FA00E00; // = OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar in OMX_IVCommon.h

    gfx_init();

    buffer_handle_t handle;

    for(int i = 0; i < gSrcFrames; i ++)
    {
        gfx_alloc(gSrcWidth, gSrcHeight, format, usage, &handle, (int32_t*)&gSrcStride);
        gfx_lock(handle, usage | GRALLOC_USAGE_SW_WRITE_OFTEN, 0, 0, gSrcWidth, gSrcHeight, (void**)(&gUsrptr[i]));
        gIMB[i] = new IntelMetadataBuffer(MetadataBufferTypeGrallocSource, (int32_t)handle);
        gfx_unlock(handle);
        IMG_native_handle_t* h = (IMG_native_handle_t*) handle;
        gSrcHeight = h->iHeight;
    }
}

int CheckArgs(int argc, char* argv[])
{
    char c;

    while ((c =getopt(argc, argv,"b:c:r:w:h:k:g:m:f:n:s:o:e:z:?") ) != EOF) {
        switch (c) {
                case 'w':
                    gSrcWidth = atoi(optarg);
                    gSrcStride = gSrcWidth;
                    break;
                case 'h':
                    gSrcHeight = atoi(optarg);
                    break;
                case 'n':
                    gEncFrames = atoi(optarg);
                    break;
                case 'm':
                    gMode = atoi(optarg);
                    break;
                case 'f':
                    gFile = optarg;
                    break;
                case 'c':
                    gCodec = atoi(optarg);
                    break;
                case 'r':
                    gRCMode = atoi(optarg);
                    break;
                case 'b':
                    gBitrate = atoi(optarg);
                    break;
                case 's':
                    gSyncEncMode = atoi(optarg);
                    break;
                case 'k':
                    gEncodeWidth = atoi(optarg);
                    break;
                case 'g':
                    gEncodeHeight = atoi(optarg);
                    break;
                case 'o':
                    gOutPutFormat = atoi(optarg);
                    break;
                case 'e':
                    gCodeCoverageTestErrorCase = atoi(optarg);
                    break;
                case 'z':
                    gOutPutBufferSize = atoi(optarg);
                    break;
                case '?':
                default:
                    printf("\n./mix_encode -c <Codec> -b <Bit rate> -r <Rate control> -w <SrcWidth> -h <SrcHeight> -k <EncodeWidth> -g <EncodeHight> -n <Frame_num> -m <Mode> -s <Sync mode> -f <Output file>\n");
              	     printf("\nCodec:\n");
                    printf("0: H264 (default)\n1: MPEG4\n2: H263\n3: VPX\n");
                    printf("\nRate control:\n");
              	     printf("0: NO_RC \n1: CBR (default)\n2: VBR\n3: VCM\n");
              	     printf("\nMode:\n");
              	     printf("0: Camera malloc (default)\n1: WiDi clone\n2: WiDi ext\n3: WiDi user\n4: Raw\n5: GrallocSource(Composer)\n6: GrallocSource(Gralloc)\n7: GrallocSource(Camera)\n");
                     exit(0);
        }
    }

    if (gEncodeWidth == 0 || gEncodeHeight == 0)
    {
        gEncodeWidth = gSrcWidth;
        gEncodeHeight = gSrcHeight;
    }

    gSrcWidth = ((gSrcWidth + 15 ) / 16 ) * 16;
    gSrcHeight = ((gSrcHeight + 15 ) / 16 ) * 16;
    gSrcStride = gSrcWidth;
    
    if (gMode == 4)
    {
        gEncodeWidth = gSrcWidth;
        gEncodeHeight = gSrcHeight;
        gSrcStride = gSrcWidth;
    }

    return 0;
}

int main(int argc, char* argv[])
{
    Encode_Status ret;
    const char *codec;
    VideoOutputFormat goutputformat = OUTPUT_EVERYTHING;

    CheckArgs(argc, argv);

    sp<ProcessState> proc(ProcessState::self());

    ProcessState::self()->startThreadPool();

    switch(gCodec)
    {
        case 0:
            codec = AVC_MIME_TYPE;
            break;
        case 1:
            codec = MPEG4_MIME_TYPE;
            break;
        case 2:
            codec = H263_MIME_TYPE;
            break;
	case 3:
	    codec = VPX_MIME_TYPE;
	    break;

        default:
            printf("Not support this type codec\n");
            return 1;
    }

    switch(gRCMode)
    {
        case 0:
            gRC = RATE_CONTROL_NONE;
            break;
        case 1:
            gRC = RATE_CONTROL_CBR;
            break;
        case 2:
            gRC = RATE_CONTROL_VBR;
            break;
        case  3:
            gRC = RATE_CONTROL_VCM;
            break;
        default:
            printf("Not support this rate control mode\n");
            return 1;
    }

    switch(gOutPutFormat)
    {
        case 0:
            goutputformat = OUTPUT_EVERYTHING;
            break;
        case 1:
            goutputformat = OUTPUT_CODEC_DATA;
            break;
        case 2:
            goutputformat = OUTPUT_FRAME_DATA;
            break;
        case 3:
            goutputformat = OUTPUT_ONE_NAL;
            break;
        case 4:
            goutputformat = OUTPUT_ONE_NAL_WITHOUT_STARTCODE;
            break;
        case 5:
            goutputformat = OUTPUT_LENGTH_PREFIXED;
            break;
        case 6:
            goutputformat = OUTPUT_BUFFER_LAST;
            break;
        default:
            printf("Not support this Out Put Format\n");
            return 1;
    }

    if(codec != NULL)
       printf("\nStart %s Encoding ....\n", codec);
    else
       printf("\nStart codec is null only for code coverage test  ....\n");

    printf("Mode is %s, RC mode is %s, Width=%d, Height=%d, Bitrate=%dbps, EncodeFrames=%d, SyncMode=%d, file is %s, outputnalformat is %s\n\n", gModeString[gMode], gRCModeString[gRCMode], gSrcWidth, gSrcHeight, gBitrate, gEncFrames, gSyncEncMode, gFile,gOutPutFormatString[gOutPutFormat]);

//sleep(10);

for(int i=0; i<1; i++)
{
    gVideoEncoder = createVideoEncoder(codec);

    if(gVideoEncoder == NULL)
    {
        printf("Finishing code coverage test  ....\n");
        return 1;
    }

    //set parameter
    SetVideoEncoderParam();

    //prepare src pictures, get user ptrs for uploading picture and prepare metadatabuffer in different mode

    switch (gMode)
    {
        case 0:  //Camera malloc
            MallocExternalMemory();
            break;
        case 1:  //WiDi clone
            GetAllUsrptr();
            break;
        case 2:  //WiDi ext
            CreateUserSurfaces(1);
            break;
        case 3:  //WiDi user
            CreateUserSurfaces(0);
            break;
        case 4:  //Raw
            MallocExternalMemory();
            break;
        case 5: //SurfaceMediaSource
            CreateGfxhandle(1);
            break;
        case 6: //Gralloc
            CreateGralloc(1);
            break;
        case 7: //Gralloc nv12 format
            CreateGfxhandle(0);
            break;
        case 8: //SurfaceMappingForCI
            CreateSurfaceMappingForCI();
            break;
        case 9:  //Camera malloc with extra values
            MallocExternalMemoryWithExtraValues();
            break;
        default:
            break;
    }

    //upload src data
    printf("Fill src picture width=%d, Height=%d\n", gSrcStride, gSrcHeight);
    for(int i=0; i<gSrcFrames; i++)
        YUV_generator_planar(gSrcStride, gSrcHeight, gUsrptr[i], gSrcStride, gUsrptr[i]+gSrcStride*gSrcHeight, gSrcStride, 0, 0, 1);

    //start
    ret = gVideoEncoder->start();
    CHECK_ENCODE_STATUS("start");

    //open out file
    FILE* file = fopen(gFile, "w");
    if (!file)
    {
    	printf("create out file failed\n");
    	return 1;
    }

    //input buffers
    VideoEncRawBuffer InBuf;
    uint8_t *data;
    uint32_t size;

    //output buffers
    VideoEncOutputBuffer OutBuf;
    uint32_t maxsize;
    //for error hanlding
    gVideoEncoder->getMaxOutSize(NULL);
    gVideoEncoder->getMaxOutSize(&maxsize);
    uint8_t out[maxsize];
    if(gOutPutBufferSize == 0)
       OutBuf.bufferSize = 0;
    else
       OutBuf.bufferSize = maxsize;
    OutBuf.dataSize = 0;
    OutBuf.data = out;
    OutBuf.format = goutputformat;

    printf("\n");
    for(unsigned int i=0; i<gEncFrames; i++)
    {
        if (gMode != 4)
        {
            gIMB[i % gSrcFrames]->Serialize(data, size);
     //       printf("srcno =%d, data=%x, size=%d\n", i % gSrcFrames, data, size);
        }else
        {
            data = gUsrptr[i % gSrcFrames];
            size = gSrcWidth * gSrcHeight * 3 /2;
        }
        InBuf.data = data;
        InBuf.size = size;
        InBuf.bufAvailable = true;
        InBuf.type = FTYPE_UNKNOWN;
        InBuf.flag = 0;

        ret = gVideoEncoder->encode(&InBuf);
        CHECK_ENCODE_STATUS("encode");

        if (i > 0) {
        ret = gVideoEncoder->getOutput(&OutBuf);
        CHECK_ENCODE_STATUS("getOutput");
//        printf("OutBuf.dataSize = %d, flag=0x%08x  .........\n", OutBuf.dataSize, OutBuf.flag);
        fwrite(OutBuf.data, 1, OutBuf.dataSize, file);
        }
        printf("Encoding %d Frames \r", i+1);
        fflush(stdout);
    }
        ret = gVideoEncoder->getOutput(&OutBuf);
    fclose(file);

    gVideoEncoder->stop();
    releaseVideoEncoder(gVideoEncoder);
    gVideoEncoder = NULL;

    switch(gMode)
    {
        case 0: //camera malloc
        case 4: //Raw
            for(int i=0; i<gSrcFrames; i++)
            {
                free(gMallocPtr[i]);
            }
            break;
        case 1: //WiDi clone
            //nothing to do
            break;
        case 2: //WiDi ext
        case 3: //WiDi user
            //release surfaces
            vaDestroySurfaces(gVADisplay, gSurface, gSrcFrames);
            break;
        case 5: //SurfaceMediaSource
            for(int i=0; i<gSrcFrames; i++)
            {
                gGraphicBuffer[i] = 0;
            }
            break;
        case 6: //Gralloc
            buffer_handle_t handle;
            for(int i=0; i<gSrcFrames; i++)
            {
                if (gIMB[i] != NULL)
                {
                    gIMB[i]->GetValue((int32_t&)handle);
                    gfx_free(handle);
                }
            }
            break;
    }

    for(int i=0; i<gSrcFrames; i++)
    {
        if (gIMB[i] != NULL)
            delete gIMB[i];
    }

    printf("\nComplete Encoding, ByeBye ....\n");

}

    return 1;
}

