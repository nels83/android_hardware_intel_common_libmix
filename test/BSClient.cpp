#include "IntelMetadataBuffer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <binder/MemoryDealer.h>
#include <binder/IMemory.h>
#include <binder/MemoryBase.h>
#include <private/gui/ComposerService.h>
#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/IGraphicBufferAlloc.h>
#include <getopt.h>

using namespace android;

int main(int argc, char* argv[])
{
    uint32_t tokenP =  0x80000000;
    uint32_t tokenC =  0x40000000;
    uint32_t tokenPC = 0xC0000000;
    uint32_t token;
#ifdef INTEL_VIDEO_XPROC_SHARING
    token = IntelMetadataBuffer::MakeSessionFlag(true, false, IntelMetadataBuffer::WEBRTC_BASE);
#endif
    int memmode = 0;
    int clearcontext = 1;
    int waittime = 0;
    int storefile = 0;
    int isProvider = 1;
    int service = 0;

    char c;
    const char *short_opts = "a:b:c:d:e:f:g:h:i:j:k:l:m:n:o:p:q:r:s:t:q:u:v:w:x:y:z:?";

    ProcessState::self()->startThreadPool();

    while ((c = getopt_long(argc, argv, short_opts, NULL, NULL) ) != EOF) {
        switch (c) {
            case 't':
                token = atoi(optarg);
                break;
            case 'm':
                memmode = atoi(optarg);
                break;
            case 'c':
                clearcontext = atoi(optarg); 
                break;
            case 'w':
                waittime = atoi(optarg); 
                break;
            case 'f':
                storefile = atoi(optarg); 
                break;
            case 'p':
                isProvider = atoi(optarg); 
                break;
            case 's':
                service = atoi(optarg); 
                break;
        }
    }

    if (service > 0) {
        printf("Setup Service ....\n");
#ifdef INTEL_VIDEO_XPROC_SHARING
        IntelBufferSharingService::instantiate();
#endif
    }

    if (isProvider) {	

        printf("Working as Provider ...\n");

        IntelMetadataBuffer *mb1;
        IMB_Result ret;
        int32_t value;
        uint8_t* bytes;
        uint32_t size;
        ValueInfo vi;

        if (memmode == 0) {
            sp<MemoryHeapBase> Heap = new MemoryHeapBase(10000);
            sp<MemoryBase> Buffer1 = new MemoryBase(Heap, 0, 1000);
//            sp<MemoryBase> Buffer2 = new MemoryBase(Heap, 1000, 1000);
            memset(Buffer1->pointer(), 0xAA, 1000);

            mb1 = new IntelMetadataBuffer();
            ret = mb1->SetType(IntelMetadataBufferTypeCameraSource);
#ifdef INTEL_VIDEO_XPROC_SHARING
            mb1->SetSessionFlag(token);
            if ((ret = mb1->ShareValue(Buffer1)) != IMB_SUCCESS) {
                printf("IntelMetadataBuffer shareValue MemBase ret = %d failed\n", ret);
                return 1;
            }
#else
            mb1->SetValue((intptr_t)Buffer1->pointer());
#endif
            ret = mb1->SetValueInfo(&vi);
            ret = mb1->Serialize(bytes, size);
 //                mb1->GetValue(value);
            printf("original MemBase1 pointer is %x\n", Buffer1->pointer());

        } else {
            sp<ISurfaceComposer> composer(ComposerService::getComposerService());
            sp<IGraphicBufferAlloc> GraphicBufferAlloc = composer->createGraphicBufferAlloc();

            uint32_t usage = GraphicBuffer::USAGE_SW_WRITE_OFTEN | GraphicBuffer::USAGE_HW_TEXTURE;
            int format = 0x3231564E; // = HAL_PIXEL_FORMAT_NV12
     //        int format = 0x7FA00E00; // = OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar
            int32_t error;
            void* usrptr[3];

            sp<GraphicBuffer> graphicBuffer(GraphicBufferAlloc->createGraphicBuffer(
                                    1280, 720, format, usage, &error));
            if (graphicBuffer.get() == NULL)
                printf("create graphicbuffer failed\n");

            status_t ret1=graphicBuffer->lock(usage, &usrptr[0]);
            memset(usrptr[0], 0xAA, 4);
            graphicBuffer->unlock();

            mb1 = new IntelMetadataBuffer();
            ret = mb1->SetType(IntelMetadataBufferTypeCameraSource);

#ifdef INTEL_VIDEO_XPROC_SHARING
            mb1->SetSessionFlag(token);
            if ((ret = mb1->ShareValue(graphicBuffer)) != IMB_SUCCESS) {
                printf("IntelMetadataBuffer shareValue graphicbuffer ret = %d failed\n", ret);
                return 1;
            }
#else
            mb1->SetValue((intptr_t)graphicBuffer->handle);
#endif
            ret = mb1->SetValueInfo(&vi);
            ret = mb1->Serialize(bytes, size);

      //       mb1->GetValue(value);
            printf("original graphicbuffer handle is %x\n", graphicBuffer->handle);
        }

        if (storefile > 0) {
            FILE* fp = fopen("/data/mdb.data", "wb+");
            if (fp != NULL) {
                fwrite(bytes, 1, size, fp);
                fclose(fp);
            }
        }

        delete mb1;
	
    }else {

        printf("Working as Consumer ...\n");

        uint8_t bytes[128];
        uint32_t size;
		
        FILE* fp = fopen("/data/mdb.data", "rb");
        if (fp != NULL)
        {
            size = fread(bytes, 1, 128, fp);
            fclose(fp);
        }

        IntelMetadataBuffer mb1;
	intptr_t value;
        IMB_Result res;
        res = mb1.UnSerialize(bytes,size);
	
        if (IMB_SUCCESS == res) {
	        res = mb1.GetValue(value);
            if (res != IMB_SUCCESS)
                printf("Consumer GetValue failed, result=%x\n", res);
            else
                printf("Consumer get value =%x\n", value);
        } else
            printf("unserialize failed, result=%x\n", res);

    }

    if (waittime > 0) {
        printf("waiting %d seconds .... \n", waittime);
		sleep(waittime);
    }

    if (clearcontext > 0) {
        printf("Clearing %s Context ... \n", (isProvider > 0) ? "Provider":"Consumer");
#ifdef INTEL_VIDEO_XPROC_SHARING
        IntelMetadataBuffer::ClearContext(token, isProvider > 0);
#endif
    }

    printf("Exit\n");
    return 1;
}

