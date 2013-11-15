#include <binder/ProcessState.h>

#include <private/gui/ComposerService.h>
#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/IGraphicBufferAlloc.h>

#include <ui/PixelFormat.h>
#include <hardware/gralloc.h>
#include <hal/hal_public.h>

#include <getopt.h>

using namespace android;

// #define HAL_PIXEL_FORMAT_BGRA_8888 5
// #define HAL_PIXEL_FORMAT_NV12 0x3231564E
#define OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar 0x7FA00E00
#define OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar_Tiled 0x7FA00F00

static hw_module_t const *gModule = NULL;
static gralloc_module_t *gAllocMod = NULL;	/* get by force
						 * hw_module_t */
static alloc_device_t *gAllocDev = NULL;

static int gfx_init(void)
{

	int err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &gModule);
	if (err) {
		printf("FATAL: can't find the %s module",
		       GRALLOC_HARDWARE_MODULE_ID);
		return -1;
	}
	
	gAllocMod = (gralloc_module_t *) gModule;

	return 0;
}

static int
gfx_alloc(uint32_t w, uint32_t h, int format,
	  int usage, buffer_handle_t * handle, int32_t * stride)
{

	int err;

	if (!gAllocDev) {
		if (!gModule) {
			if (gfx_init()) {
				printf("can't find the %s module",
				       GRALLOC_HARDWARE_MODULE_ID);
				return -1;
			}
		}

		err = gralloc_open(gModule, &gAllocDev);
		if (err) {
			printf("FATAL: gralloc open failed\n");
			return -1;
		}
	}

	err = gAllocDev->alloc(gAllocDev, w, h, format, usage, handle, stride);
	if (err) {
		printf("alloc(%u, %u, %d, %08x, ...) failed %d (%s)\n",
		       w, h, format, usage, err, strerror(-err));
	}

	return err;
}

static int gfx_free(buffer_handle_t handle)
{

	int err;

	if (!gAllocDev) {
		if (!gModule) {
			if (gfx_init()) {
				printf("can't find the %s module",
				       GRALLOC_HARDWARE_MODULE_ID);
				return -1;
			}
		}

		err = gralloc_open(gModule, &gAllocDev);
		if (err) {
			printf("FATAL: gralloc open failed\n");
			return -1;
		}
	}

	err = gAllocDev->free(gAllocDev, handle);
	if (err) {
		printf("free(...) failed %d (%s)\n", err, strerror(-err));
	}

	return err;
}

static int
gfx_lock(buffer_handle_t handle, int usage,
	 int left, int top, int width, int height, void **vaddr)
{

	int err;

	if (!gAllocMod) {
		if (gfx_init()) {
			printf("can't find the %s module",
			       GRALLOC_HARDWARE_MODULE_ID);
			return -1;
		}
	}

	err = gAllocMod->lock(gAllocMod, handle, usage,
			      left, top, width, height, vaddr);
//	printf("gfx_lock: handle is %x, usage is %x, vaddr is %x.\n",
//	       (unsigned int)handle, usage, (unsigned int)*vaddr);

	if (err) {
		printf("lock(...) failed %d (%s).\n", err, strerror(-err));
		return -1;
	}
	
	return err;
}

static int gfx_unlock(buffer_handle_t handle)
{

	int err;

	if (!gAllocMod) {
		if (gfx_init()) {
			printf("can't find the %s module",
			       GRALLOC_HARDWARE_MODULE_ID);
			return -1;
		}
	}

	err = gAllocMod->unlock(gAllocMod, handle);
	if (err) {
		printf("unlock(...) failed %d (%s)", err, strerror(-err));
		return -1;
	}

	return err;
}

static int
gfx_Blit(buffer_handle_t src, buffer_handle_t dest, int w, int h, int x, int y)
{
	int err;

	if (!gAllocMod) {
		if (gfx_init()) {
			printf("can't find the %s module",
			       GRALLOC_HARDWARE_MODULE_ID);
			return -1;
		}
	}

	IMG_gralloc_module_public_t *GrallocMod =
	    (IMG_gralloc_module_public_t *) gModule;

#ifdef MRFLD_GFX
	err = GrallocMod->Blit(GrallocMod, src, dest, w, h, 0, 0, 0);
#else
	err = GrallocMod->Blit2(GrallocMod, src, dest, w, h, 0, 0);
#endif

	if (err) {
		printf("Blit(...) failed %d (%s)", err, strerror(-err));
		return -1;
	}

	return err;
}

void help()
{
	printf("gfx-test:\n");
	printf("	-w/--srcW <width> -h/--srcH <height>\n");
	printf("	-k/--dstW <width> -g/--dstH <height>\n");
	printf("	-a/--blitW <width> -b/--blitH <height>\n");
	printf("	-m/--allocM <0: server, 1: native>\n");
}

#define ListSize 12

int main(int argc, char *argv[])
{
	//for allocator test
	int AllocMethod = 0;
	int Width;
	int Height;
	int Stride;
	int ColorFormat;
	char *Name;

	//for Blit test
	buffer_handle_t SrcHandle, DstHandle;
	int SrcWidth = 720, SrcHeight = 1280, SrcStride;
	int DstWidth = 720, DstHeight = 1280, DstStride;
	int BlitWidth = SrcWidth, BlitHeight = SrcHeight;
	void *SrcPtr[3];
	void *DstPtr[3];
	
    const char *short_opts = "a:b:c:d:e:f:g:h:i:j:k:l:m:n:o:p:q:r:s:t:q:u:v:w:x:y:z:?";
    const struct option long_opts[] = {
						{"help", no_argument, NULL, '?'},
						{"srcW", required_argument, NULL, 'w'},
						{"srcH", required_argument, NULL, 'h'},
						{"dstW", required_argument, NULL, 'k'},
						{"dstH", required_argument, NULL, 'g'},
						{"blitW", required_argument, NULL, 'a'},
						{"blitH", required_argument, NULL, 'b'},
						{"allocM", required_argument, NULL, 'm'},
						{0, 0, 0, 0}
    };

    char c;
    while ((c = getopt_long(argc, argv, short_opts, long_opts, NULL) ) != EOF) {
        switch (c) {
                case 'a':
                    BlitWidth = atoi(optarg);
                    break;

                case 'b':
                    BlitHeight = atoi(optarg);
                    break;

                case 'w':
                    SrcWidth = atoi(optarg);
                    break;

                case 'h':
                    SrcHeight = atoi(optarg);
                    break;

                case 'k':
                    DstWidth = atoi(optarg);
                    break;

                case 'g':
                    DstHeight = atoi(optarg);
                    break;

                case '?':
                    help();
                    return 1;

                case 'm':
                    AllocMethod = atoi(optarg);
                    break;

       }
    }

	android::ProcessState::self()->startThreadPool();

	struct Params {
		int Width;
		int Height;
		int ColorFormat;
		char *Name;
	};

	Params ParamList[ListSize] = {
		{320, 240, HAL_PIXEL_FORMAT_BGRA_8888,
		 "HAL_PIXEL_FORMAT_BGRA_8888"},
		{320, 240, HAL_PIXEL_FORMAT_NV12, "HAL_PIXEL_FORMAT_NV12"},
		{320, 240, OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar,
		 "OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar"},
		{720, 480, HAL_PIXEL_FORMAT_BGRA_8888,
		 "HAL_PIXEL_FORMAT_BGRA_8888"},
		{720, 480, HAL_PIXEL_FORMAT_NV12, "HAL_PIXEL_FORMAT_NV12"},
		{720, 480, OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar,
		 "OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar"},
		{1280, 720, HAL_PIXEL_FORMAT_BGRA_8888,
		 "HAL_PIXEL_FORMAT_BGRA_8888"},
		{1280, 720, HAL_PIXEL_FORMAT_NV12, "HAL_PIXEL_FORMAT_NV12"},
		{1280, 720, OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar,
		 "OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar"},
		{1920, 1080, HAL_PIXEL_FORMAT_BGRA_8888,
		 "HAL_PIXEL_FORMAT_BGRA_8888"},
		{1920, 1080, HAL_PIXEL_FORMAT_NV12, "HAL_PIXEL_FORMAT_NV12"},
		{1920, 1080, OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar,
		 "OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar"},
	};

	buffer_handle_t Handle = NULL;

	// format memory structure test
	for (int i = 0; i < ListSize; i++) {
		Width = ParamList[i].Width;
		Height = ParamList[i].Height;
		ColorFormat = ParamList[i].ColorFormat;
		Name = ParamList[i].Name;

		sp < GraphicBuffer > graphicBuffer;

		if (AllocMethod == 0) {
			uint32_t usage =
			    GraphicBuffer::USAGE_HW_TEXTURE | GraphicBuffer::
			    USAGE_HW_RENDER;
			int32_t error;

			sp < ISurfaceComposer >
			    composer(ComposerService::getComposerService());
			sp < IGraphicBufferAlloc > GraphicBufferAlloc =
			    composer->createGraphicBufferAlloc();

			graphicBuffer =
			    GraphicBufferAlloc->createGraphicBuffer(Width,
								    Height,
								    ColorFormat,
								    usage,
								    &error);

			if (graphicBuffer.get() == NULL) {
				printf("GFX createGraphicBuffer failed\n");
				return 0;
			}

			Stride = graphicBuffer->getStride();
			Handle = graphicBuffer->handle;

		} else {
			int usage =
			    GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER;

			if (gfx_alloc
			    (Width, Height, ColorFormat, usage, &Handle,
			     &Stride)
			    != 0) {
				printf("Gralloc allocate failed\n");
				return 0;
			}

		}

		printf("%s\n", Name);
		printf
		    ("	gfx handle 0x%08x, Width=%d, Height=%d, Stride=%d, ColorFormat=0x%08x\n",
		     Handle, Width, Height, Stride, ColorFormat);
		IMG_native_handle_t *h = (IMG_native_handle_t *) Handle;
		printf
		    ("	IMG_native_handle_t iWidth=%d, iHeight=%d, iFormat=0x%08x\n\n",
		     h->iWidth, h->iHeight, h->iFormat);

		if (AllocMethod > 0)
			gfx_free(Handle);
	}

	// Blit test, from HAL_PIXEL_FORMAT_BGRA_8888 to HAL_PIXEL_FORMAT_NV12
	printf("Start Blit test ............\n");
	int usage = GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER;

	// src rgba 
	ColorFormat = HAL_PIXEL_FORMAT_BGRA_8888;
	if (gfx_alloc(SrcWidth, SrcHeight, ColorFormat, usage, &SrcHandle, &SrcStride)
	    != 0) {
		printf
		    ("Gralloc allocate handle1 HAL_PIXEL_FORMAT_BGRA_8888 failed\n");
		return 0;
	}
	// Fill data
	if (gfx_lock
	    (SrcHandle, usage | GRALLOC_USAGE_SW_WRITE_OFTEN, 0, 0, SrcWidth,
	     SrcHeight, &SrcPtr[0]) != 0)
		return 0;
	int data = 0x00FF0000;	// AA RR GG BB
	for (int i = 0; i < SrcStride * SrcHeight; i++)
		memcpy(SrcPtr[0] + i * 4, &data, 4);
	// dump to file
	FILE *fp1 = fopen("/data/dump.rgb", "wb");
	fwrite(SrcPtr[0], 1, SrcStride * SrcHeight * 4, fp1);
	fclose(fp1);
	gfx_unlock(SrcHandle);
	//============================================================================

	// dest nv12
	ColorFormat = HAL_PIXEL_FORMAT_NV12;
//	ColorFormat = OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar;
	if (gfx_alloc(DstWidth, DstHeight, ColorFormat, usage, &DstHandle, &DstStride)
	    != 0) {
		printf
		    ("Gralloc allocate handle2 HAL_PIXEL_FORMAT_NV12 failed\n");
		return 0;
	}
	//===========================================================================

	printf("\nSRC size: %dx%d, stride:%d\n", SrcWidth, SrcHeight, SrcStride);
	printf("DST size: %dx%d, stride:%d\n", DstWidth, DstHeight, DstStride);
	printf("Blit RECT: %dx%d \n", BlitWidth, BlitHeight);

	// blit start
	if (gfx_Blit(SrcHandle, DstHandle, BlitWidth, BlitHeight, 0, 0) != 0) {
		printf("Gralloc Blit failed\n");
		return 0;
	}

	//============================================================================
	// dump data of dest
	if (gfx_lock
	    (DstHandle, usage | GRALLOC_USAGE_SW_WRITE_OFTEN, 0, 0, DstWidth,
	     DstHeight, &DstPtr[0]) != 0)
		return 0;
	FILE *fp2 = fopen("/data/dump.yuv", "wb");
	fwrite(DstPtr[0], 1, DstStride * DstHeight * 1.5, fp2);
	fclose(fp2);
	gfx_unlock(DstHandle);

	// release handles
	gfx_free(SrcHandle);
	gfx_free(DstHandle);

	printf("Complete Blit test ............\n");
	return 1;
}
