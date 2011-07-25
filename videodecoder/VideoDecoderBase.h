/* INTEL CONFIDENTIAL
* Copyright (c) 2009 Intel Corporation.  All rights reserved.
*
* The source code contained or described herein and all documents
* related to the source code ("Material") are owned by Intel
* Corporation or its suppliers or licensors.  Title to the
* Material remains with Intel Corporation or its suppliers and
* licensors.  The Material contains trade secrets and proprietary
* and confidential information of Intel or its suppliers and
* licensors. The Material is protected by worldwide copyright and
* trade secret laws and treaty provisions.  No part of the Material
* may be used, copied, reproduced, modified, published, uploaded,
* posted, transmitted, distributed, or disclosed in any way without
* Intel's prior express written permission.
*
* No license under any patent, copyright, trade secret or other
* intellectual property right is granted to or conferred upon you
* by disclosure or delivery of the Materials, either expressly, by
* implication, inducement, estoppel or otherwise. Any license
* under such intellectual property rights must be express and
* approved by Intel in writing.
*
*/

#ifndef VIDEO_DECODER_BASE_H_
#define VIDEO_DECODER_BASE_H_

#include <va/va.h>
#include "VideoDecoderDefs.h"
#include "VideoDecoderInterface.h"

extern "C" {
#include "vbp_loader.h"
}

#ifndef Display
typedef unsigned int Display;
#endif


class VideoDecoderBase : public IVideoDecoder {
public:
    VideoDecoderBase(const char *mimeType, _vbp_parser_type type);
    virtual ~VideoDecoderBase();

    virtual Decode_Status start(VideoConfigBuffer *buffer);
    virtual void stop(void);
    //virtual Decode_Status decode(VideoDecodeBuffer *buffer);
    virtual void flush(void);
    virtual const VideoRenderBuffer* getOutput(bool draining = false);
    virtual const VideoFormatInfo* getFormatInfo(void);

protected:
    // each acquireSurfaceBuffer must be followed by a corresponding outputSurfaceBuffer or releaseSurfaceBuffer.
    // Only one surface buffer can be acquired at any given time
    virtual Decode_Status acquireSurfaceBuffer(void);
    // frame is successfully decoded to the acquired surface buffer and surface is ready for output
    virtual Decode_Status outputSurfaceBuffer(void);
    // acquired surface  buffer is not used
    virtual Decode_Status releaseSurfaceBuffer(void);
    virtual Decode_Status endDecodingFrame(bool dropFrame);
    virtual VideoSurfaceBuffer* findOutputByPoc(bool draining = false);
    virtual VideoSurfaceBuffer* findOutputByPct(bool draining = false);
    virtual VideoSurfaceBuffer* findOutputByPts(bool draining = false);
    virtual Decode_Status setupVA(int32_t numSurface, VAProfile profile);
    virtual Decode_Status terminateVA(void);
    virtual Decode_Status parseBuffer(uint8_t *buffer, int32_t size, bool config, void** vbpData);

    static inline int32_t alignMB(int32_t a) {
         return ((a + 15) & (~15));
    }

private:
    Decode_Status mapSurface(void);
    Decode_Status getRawDataFromSurface(void);
    void initSurfaceBuffer(bool reset);

protected:
    VideoFormatInfo mVideoFormatInfo;
    Display *mDisplay;
    VADisplay mVADisplay;
    VAContextID mVAContext;
    VAConfigID mVAConfig;
    bool mVAStarted;
    uint64_t mCurrentPTS; // current presentation time stamp (unit is unknown, depend on the framework: GStreamer 100-nanosec, Android: microsecond)
    // the following three member variables should be set using
    // acquireSurfaceBuffer/outputSurfaceBuffer/releaseSurfaceBuffer
    VideoSurfaceBuffer *mAcquiredBuffer;
    VideoSurfaceBuffer *mLastReference;
    VideoSurfaceBuffer *mForwardReference;
    VideoConfigBuffer  mConfigBuffer; // only store configure meta data.
    bool mDecodingFrame; // indicate whether a frame is being decoded
    bool mSizeChanged; // indicate whether video size is changed.

    enum {
        // TODO: move this to vbp_loader.h
        VBP_INVALID = 0xFF,
        // TODO: move this to va.h
        VAProfileSoftwareDecoding = 0xFF,
    };

    enum OUTPUT_METHOD {
        // output by Picture Coding Type (I, P, B)
         OUTPUT_BY_PCT,
        // output by Picture Order Count (for AVC only)
         OUTPUT_BY_POC,
         //OUTPUT_BY_POS,
         //OUTPUT_BY_PTS,
     };

private:
    bool mLowDelay; // when true, decoded frame is immediately output for rendering
    bool mRawOutput; // whether to output NV12 raw data
    bool mManageReference;  // this should stay true for VC1/MP4 decoder, and stay false for AVC decoder. AVC  handles reference frame using DPB
    OUTPUT_METHOD mOutputMethod;
    int32_t mOutputWindowSize; // indicate limit of number of outstanding frames for output

    int32_t mNumSurfaces;
    VideoSurfaceBuffer *mSurfaceBuffers;
    VideoSurfaceBuffer *mOutputHead; // head of output buffer list
    VideoSurfaceBuffer *mOutputTail;  // tail of output buffer list
    VASurfaceID *mSurfaces; // surfaces array
    uint8_t **mSurfaceUserPtr; // mapped user space pointer
    int32_t mSurfaceAcquirePos; // position of surface to start acquiring
    int32_t mNextOutputPOC; // Picture order count of next output
    _vbp_parser_type mParserType;
    void *mParserHandle;

protected:
    void ManageReference(bool enable) {mManageReference = enable;}
    void setOutputMethod(OUTPUT_METHOD method) {mOutputMethod = method;}
    void setOutputWindowSize(int32_t size) {mOutputWindowSize = size;}
};


#endif  // VIDEO_DECODER_BASE_H_
