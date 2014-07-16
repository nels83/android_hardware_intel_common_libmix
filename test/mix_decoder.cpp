#define LOG_NDEBUG 0
#define LOG_TAG "mix_decoder"
#include <VideoDecoderHost.h>
#include <wrs_omxil_core/log.h>
#include <vbp_loader.h>
#include <va/va.h>
#include <stdlib.h>
#include <VideoFrameInfo.h>
#define INPUTSIZE   (4*1024*1024)
static int gImgWidth;
static int gImgHeight;
static int gCodec;
static int gOutputSize;
static int gFrame;

void CheckArgs(int argc, char* argv[])
{
    char c;
    while ((c =getopt(argc, argv,"c:w:h:f:?") ) != EOF) {
        switch (c) {
                case 'w':
                    gImgWidth = atoi(optarg);
                    break;
                case 'h':
                    gImgHeight = atoi(optarg);
                    break;
                case 'c':
                    gCodec = atoi(optarg);
                    break;
                case 'f':
                    gFrame = atoi(optarg);
                    break;
                case '?':
                default:
                    ALOGI("./mix_encode -c Codec -w SrcWidth -h SrcHeight");
                    exit(0);
        }
    }

    ALOGI("gImgWidth = %d, gImgHeight = %d, gCodec = %d", gImgWidth, gImgHeight, gCodec);

}


int main(int argc, char* argv[])
{
    FILE *fp_in, *fp_out;
    char inputfilename[512];
    char outputfilename[512];
    int  framenum = 1000;
    int  frameidx = 0;

    uint8 *inBuf = NULL;
    uint8 *outBuf = NULL;
    int  in_size;
    uint32_t out_size;
    char *codecname = NULL;

    uint8_t nalutype;

    char codecnamelist[2][32] = {"video/avc", "video/avc-secure"};

    CheckArgs(argc, argv);

    if (gImgWidth <= 0) {
        ALOGE("Err: wrong video width = %d", gImgWidth);
        return -1;
    }

    if (gImgHeight <= 0) {
        ALOGE("Err: wrong video height = %d", gImgHeight);
        return -1;
    }

    if (gCodec < 0) {
        ALOGE("Err: wrong codec type = %d", gCodec);
        return -1;
    }

    if (gFrame < 0) {
        ALOGE("Err: wrong frame number = %d", gFrame);
        return -1;
    }

    framenum = gFrame;

    gOutputSize = gImgWidth * gImgHeight * 3/2;

    VideoDecodeBuffer buffer;
    int outidx = 0;

    IVideoDecoder *testDecoder = createVideoDecoder(codecnamelist[gCodec]);
    if (testDecoder == NULL) {
        ALOGE("Fail to create testDecoder!");
        return -1;
    }

    ALOGV("Test decoder is starting...");
    VideoConfigBuffer configBuffer;
    memset(&configBuffer, 0, sizeof(VideoConfigBuffer));

    configBuffer.flag |= WANT_RAW_OUTPUT;

    configBuffer.width = gImgWidth;
    configBuffer.height = gImgHeight;
    configBuffer.flag |= IS_SUBSAMPLE_ENCRYPTION;

    testDecoder->start(&configBuffer);

    inBuf = (uint8 *)malloc(INPUTSIZE);
    if (inBuf == NULL) {
        ALOGE("Fail to malloc the input buffer!");
        return -1;
    }

    outBuf = (uint8 *)malloc(gOutputSize);
    if (outBuf == NULL) {
        ALOGE("Fail to malloc the output buffer!");
        return -1;
    }

    frame_info_t frame_info;

    for (frameidx = 0; frameidx < framenum; frameidx++) {

        memset(inBuf, 0, INPUTSIZE);
        sprintf(inputfilename, "/data/bitstream/frame_%04d.bin", frameidx);
        if((fp_in = fopen(inputfilename,"rb")) == NULL) {
            ALOGE("Fail to open inputfilename %s", inputfilename);
            return -1;
        }
        fseek(fp_in, 0, SEEK_END);
        in_size = ftell(fp_in);
        ALOGV("%d frame input size = %d", frameidx, in_size);
        rewind(fp_in);
        if (in_size > INPUTSIZE) {
            ALOGE("The bitstream size is bigger than the input buffer!");
            return -1;
        }
        fread(inBuf, 1, in_size, fp_in);
        fclose(fp_in);
        memset(&buffer, 0, sizeof(VideoDecodeBuffer));

        nalutype = inBuf[4] & 0x1F;
        if (nalutype == 0x07 || nalutype == 0x08) {
            ALOGV("Clear SPS/PPS is sent");
            frame_info.data = inBuf;
            frame_info.size = in_size;
            frame_info.num_nalus = 1;
            frame_info.nalus[0].data = inBuf;
            frame_info.nalus[0].length = in_size;
            frame_info.nalus[0].type = inBuf[4];
            frame_info.nalus[0].offset = 0;
            buffer.data = (uint8_t *)&frame_info;
            buffer.size = sizeof(frame_info_t);
            buffer.flag |= IS_SECURE_DATA;

   //         buffer.data = inBuf;
   //         buffer.size = in_size;
        } else {
#if 0
            ALOGV("Encrypted slice data is sent");
            frame_info.data = (uint8_t *) &inBuf[5];
            frame_info.size = in_size - 5;
            frame_info.subsamplenum = 1;
            frame_info.subsampletable[0].subsample_type = inBuf[4];
            frame_info.subsampletable[0].subsample_size = in_size - 5;
#endif
            ALOGV("Encrypted slice data is sent");
            frame_info.data = inBuf;
            frame_info.size = in_size;
            frame_info.num_nalus = 2;
            frame_info.nalus[0].offset = 0;
            frame_info.nalus[0].type = 0x06;
            frame_info.nalus[0].length = 5;
            frame_info.nalus[0].data = NULL;

            frame_info.nalus[1].offset = 5;
            frame_info.nalus[1].type = inBuf[4];
            frame_info.nalus[1].length = in_size - 5;
            frame_info.nalus[1].data = NULL;

            buffer.data = (uint8_t *)&frame_info;
            buffer.size = sizeof(frame_info_t);
            buffer.flag |= IS_SECURE_DATA;
        }

        buffer.rotationDegrees = 0;
        buffer.timeStamp = frameidx;

        testDecoder->decode(&buffer);

        const VideoRenderBuffer * renderbuf = testDecoder->getOutput(true);

        if (renderbuf != NULL) {
            out_size = 0;
            memset(outBuf, 0, gOutputSize);

            renderbuf->renderDone = true;
            ALOGV("Output frame %d, out_size = %d", outidx, out_size);
            sprintf(outputfilename, "/data/outputsurface/frame_%04d.bin", outidx++);
            if((fp_out = fopen(outputfilename,"wb")) == NULL) {
                ALOGE("Fail to open outputfile: %s", outputfilename);
                return -1;
            }

            fwrite(renderbuf->rawData->data, 1,renderbuf->rawData->size,fp_out);
            fflush(fp_out);
            fclose(fp_out);
        }
    }

    testDecoder->stop();
    releaseVideoDecoder(testDecoder);
    free(inBuf);
    free(outBuf);
    return 0;
}
