#define LOG_NDEBUG 0
#define LOG_TAG "mix_decoder"
#include <VideoDecoderHost.h>
#include <utils/Log.h>
#include <vbp_loader.h>
#include <va/va.h>
#include <stdlib.h>

#define INPUTSIZE   (4*1024*1024)
static int gImgWidth;
static int gImgHeight;
static int gCodec;
static int gOutputSize;

void CheckArgs(int argc, char* argv[])
{
    char c;
    while ((c =getopt(argc, argv,"c:w:h:?") ) != EOF) {
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

    for (frameidx = 0; frameidx < framenum; frameidx++) {
        sprintf(inputfilename, "/data/decrypted_frame/decrypted_frame_%d.h264", frameidx);
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
        buffer.data = inBuf;
        buffer.size = in_size;
        buffer.rotationDegrees = 0;
        buffer.timeStamp = frameidx;

        testDecoder->decode(&buffer);

        const VideoRenderBuffer * renderbuf = testDecoder->getOutput(true);

        if (renderbuf != NULL) {
            out_size = 0;
            memset(outBuf, 0, gOutputSize);

            renderbuf->renderDone = true;
            ALOGV("Output frame %d, out_size = %d", outidx, out_size);
            sprintf(outputfilename, "/data/decodedframe/frame_%d.bin", outidx++);
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
