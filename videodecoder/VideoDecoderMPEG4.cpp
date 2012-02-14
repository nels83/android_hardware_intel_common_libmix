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

#include "VideoDecoderMPEG4.h"
#include "VideoDecoderTrace.h"
#include <string.h>

VideoDecoderMPEG4::VideoDecoderMPEG4(const char *mimeType)
    : VideoDecoderBase(mimeType, VBP_MPEG4),
      mLastVOPTimeIncrement(0),
      mExpectingNVOP(false),
      mSendIQMatrixBuf(false),
      mLastVOPCodingType(MP4_VOP_TYPE_I) {
}

VideoDecoderMPEG4::~VideoDecoderMPEG4() {
    stop();
}

Decode_Status VideoDecoderMPEG4::start(VideoConfigBuffer *buffer) {
    Decode_Status status;

    status = VideoDecoderBase::start(buffer);
    CHECK_STATUS("VideoDecoderBase::start");

    if (buffer->data == NULL || buffer->size == 0) {
        WTRACE("No config data to start VA.");
        return DECODE_SUCCESS;
    }

    vbp_data_mp42 *data = NULL;
    status = VideoDecoderBase::parseBuffer(buffer->data, buffer->size, true, (void**)&data);
    CHECK_STATUS("VideoDecoderBase::parseBuffer");

    status = startVA(data);
    return status;
}

void VideoDecoderMPEG4::stop(void) {
    // drop the last frame and ignore return value
    endDecodingFrame(true);
    VideoDecoderBase::stop();

    mLastVOPTimeIncrement = 0;
    mExpectingNVOP = false;
    mLastVOPCodingType = MP4_VOP_TYPE_I;
}

Decode_Status VideoDecoderMPEG4::decode(VideoDecodeBuffer *buffer) {
    Decode_Status status;
    vbp_data_mp42 *data = NULL;
    if (buffer == NULL) {
        return DECODE_INVALID_DATA;
    }
    if (buffer->flag & IS_SYNC_FRAME) {
        mIsSyncFrame = true;
    } else {
        mIsSyncFrame = false;
    }
    status =  VideoDecoderBase::parseBuffer(
            buffer->data,
            buffer->size,
            false,
            (void**)&data);
    CHECK_STATUS("VideoDecoderBase::parseBuffer");

    if (!mVAStarted) {
        status = startVA(data);
        CHECK_STATUS("startVA");
    }

    status = decodeFrame(buffer, data);
    CHECK_STATUS("decodeFrame");
    if (mSizeChanged) {
        mSizeChanged = false;
        return DECODE_FORMAT_CHANGE;
    }

    return status;
}

void VideoDecoderMPEG4::flush(void) {
    VideoDecoderBase::flush();

    mExpectingNVOP = false;
    mLastVOPTimeIncrement = 0;
    mLastVOPCodingType = MP4_VOP_TYPE_I;
}

Decode_Status VideoDecoderMPEG4::decodeFrame(VideoDecodeBuffer *buffer, vbp_data_mp42 *data) {
    Decode_Status status;
    // check if any slice is parsed, we may just receive configuration data
    if (data->number_picture_data == 0) {
        WTRACE("number_picture_data == 0");
        return DECODE_SUCCESS;
    }

    uint64_t lastPTS = mCurrentPTS;
    mCurrentPTS = buffer->timeStamp;

    if (lastPTS != mCurrentPTS) {
        // finish decoding the last frame
        status = endDecodingFrame(false);
        CHECK_STATUS("endDecodingFrame");

        // start decoding a new frame
        status = beginDecodingFrame(data, buffer);
        if (status != DECODE_SUCCESS && status != DECODE_MULTIPLE_FRAME) {
            endDecodingFrame(true);
        }
        CHECK_STATUS("beginDecodingFrame");
    } else {
        status = continueDecodingFrame(data, buffer);
        if (status != DECODE_SUCCESS && status != DECODE_MULTIPLE_FRAME) {
            endDecodingFrame(true);
        }
        CHECK_STATUS("continueDecodingFrame");
    }

    if (buffer->flag & HAS_COMPLETE_FRAME) {
        // finish decoding current frame
        status = endDecodingFrame(false);
        CHECK_STATUS("endDecodingFrame");
    }

    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderMPEG4::beginDecodingFrame(vbp_data_mp42 *data, VideoDecodeBuffer *buffer) {

    Decode_Status status = DECODE_SUCCESS;
    vbp_picture_data_mp42 *picData = data->picture_data;
    VAPictureParameterBufferMPEG4 *picParam = &(picData->picture_param);
    int codingType = picParam->vop_fields.bits.vop_coding_type;

    // start sanity checking
    if (mExpectingNVOP) {
        // if we are waiting for n-vop for packed frame, and the new frame is coded, the coding type
        // of this frame must be B
        // for example: {PB} B N P B B P...
        if (picData->vop_coded == 1 && codingType != MP4_VOP_TYPE_B) {
            WTRACE("Invalid coding type while waiting for n-vop for packed frame.");
            // timestamp of P frame in the queue is not correct.
            // TODO: handle timestamp
            flush();
            mExpectingNVOP = false;
        }
    }

    // handle N-VOP picuture, it could be a skipped frame or a simple placeholder of packed frame
    if (picData->vop_coded == 0) {
        if (mLastReference == NULL) {
            WTRACE("The last reference is unavailable to construct skipped frame.");
            flush();
            mExpectingNVOP = false;
            // TODO: handle this case
            return DECODE_SUCCESS;
        }

        if (mExpectingNVOP) {
            // P frame is already in queue, just need to update time stamp.
            mLastReference->renderBuffer.timeStamp = mCurrentPTS;
            mExpectingNVOP = false;
        }
        else {
            // Do nothing for skip frame as the last frame will be rendered agian by natively
            // No needs to handle reference frame neither
#if 0
            // this is skipped frame, use the last reference frame as output
            status = acquireSurfaceBuffer();
            CHECK_STATUS("acquireSurfaceBuffer");
            mAcquiredBuffer->renderBuffer.timeStamp = mCurrentPTS;
            mAcquiredBuffer->renderBuffer.flag = 0;
            mAcquiredBuffer->renderBuffer.scanFormat = mLastReference->renderBuffer.scanFormat;
            mAcquiredBuffer->renderBuffer.surface = mLastReference->renderBuffer.surface;
            // No need to update mappedData for HW decoding
            //mAcquiredBuffer->mappedData.data = mLastReference->mappedData.data;
            mAcquiredBuffer->referenceFrame = true;
            status = outputSurfaceBuffer();
            CHECK_STATUS("outputSurfaceBuffer");
#endif
        }

        if (data->number_picture_data > 1) {
            WTRACE("Unexpected to have more picture data following a non-coded VOP.");
            //picture data is thrown away. No issue if picture data is for N-VOP. if picture data is for
            // coded picture, a frame is lost.
            // TODO: handle this case
            // return DECODE_FAIL;
        }
        return DECODE_SUCCESS;
    }
    else {
        // Check if we have reference frame(s)  for decoding
        if (codingType == MP4_VOP_TYPE_B)  {
            if (mForwardReference ==  NULL ||
                mLastReference == NULL) {
                return DECODE_NO_REFERENCE;
            }
        } else if (codingType == MP4_VOP_TYPE_P || codingType == MP4_VOP_TYPE_S) {
            if (mLastReference == NULL&& mIsSyncFrame == false) {
                return DECODE_NO_REFERENCE;
            }
        }
        // all sanity checks pass, continue decoding through continueDecodingFrame
        status = continueDecodingFrame(data, buffer);
    }
    return status;
}

Decode_Status VideoDecoderMPEG4::continueDecodingFrame(vbp_data_mp42 *data, VideoDecodeBuffer *buffer) {
    Decode_Status status = DECODE_SUCCESS;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    bool useGraphicBuffer = mConfigBuffer.flag & USE_NATIVE_GRAPHIC_BUFFER;

    /*
         Packed Frame Assumption:

         1. In one packed frame, there's only one P or I frame and only one B frame.
         2. In packed frame, there's no skipped frame (vop_coded = 0)
         3. For one packed frame, there will be one N-VOP frame to follow the packed frame (may not immediately).
         4. N-VOP frame is the frame with vop_coded = 0.
         5. The timestamp of  N-VOP frame will be used for P or I frame in the packed frame


         I, P, {P, B}, B, N, P, N, I, ...
         I, P, {P, B}, N, P, N, I, ...

         The first N is placeholder for P frame in the packed frame
         The second N is a skipped frame
         */

    vbp_picture_data_mp42 *picData = data->picture_data;
    for (uint32_t i = 0; i < data->number_picture_data; i++, picData = picData->next_picture_data) {
        // each slice has its own picture data, video_packet_header following resync_marker may reset picture header, see MP4 spec
        VAPictureParameterBufferMPEG4 *picParam = &(picData->picture_param);
        int codingType = picParam->vop_fields.bits.vop_coding_type;
        if (codingType == MP4_VOP_TYPE_S && picParam->no_of_sprite_warping_points > 1) {
            WTRACE("Hardware only supports up to one warping point (stationary or translation)");
            // TODO:  we actually can't decode this frame
            return DECODE_FAIL;
        }

        if (picData->vop_coded == 0) {
            ETRACE("Unexpected to have non-coded VOP.");
            return DECODE_FAIL;
        }
        if (picData->new_picture_flag == 1 || mDecodingFrame == false) {
            // either condition indicates start of a new frame
            if (picData->new_picture_flag == 0) {
                WTRACE("First slice of picture is lost!");
                // TODO: handle this case
            }
            if (mDecodingFrame) {
                if (codingType == MP4_VOP_TYPE_B){
                    // this indicates the start of a new frame in the packed frame
                    // Update timestamp for P frame in the packed frame as timestamp here is for the B frame!
                    if (picParam->vop_time_increment_resolution){
                        uint64_t increment = mLastVOPTimeIncrement - picData->vop_time_increment +
                                picParam->vop_time_increment_resolution;
                        increment = increment % picParam->vop_time_increment_resolution;
                        // convert to micro-second
                        // TODO: unit of time stamp varies on different frame work
                        increment = increment * 1e6 / picParam->vop_time_increment_resolution;
                        mAcquiredBuffer->renderBuffer.timeStamp += increment;
                        if (useGraphicBuffer){
                           buffer->nextTimeStamp = mCurrentPTS;
                           mCurrentPTS = mAcquiredBuffer->renderBuffer.timeStamp;
                        }
                    }
                } else {
                    // this indicates the start of a new frame in the packed frame. no B frame int the packet
                    // Update the timestamp according the increment
                    if (picParam->vop_time_increment_resolution){
                        int64_t increment = picData->vop_time_increment - mLastVOPTimeIncrement + picParam->vop_time_increment_resolution;
                        increment = increment % picParam->vop_time_increment_resolution;
                        //convert to micro-second
                        increment = increment * 1e6 / picParam->vop_time_increment_resolution;
                        if (useGraphicBuffer) {
                            buffer->nextTimeStamp = mCurrentPTS + increment;
                        }
                        else {
                            mCurrentPTS += increment;
                        }

                    } else {
                        if (useGraphicBuffer) {
                            buffer->nextTimeStamp = mCurrentPTS + 30000;
                        }
                        else {
                            mCurrentPTS += 30000;
                        }
                    }
                }
                endDecodingFrame(false);
                mExpectingNVOP = true;
                if (codingType != MP4_VOP_TYPE_B) {
                    mExpectingNVOP = false;
                }
                if (useGraphicBuffer) {
                    buffer->hasNext  = true;
                    buffer->offSet = data->frameSize;
                    VTRACE("Report OMX to handle for Multiple frame offset=%d time=%lld",data->frameSize,buffer->nextTimeStamp);
                    return DECODE_MULTIPLE_FRAME;
                }
            }

            // acquire a new surface buffer
            status = acquireSurfaceBuffer();
            CHECK_STATUS("acquireSurfaceBuffer");

            // sprite is treated as P frame in the display order, so only B frame frame is not used as "reference"
            mAcquiredBuffer->referenceFrame = (codingType != MP4_VOP_TYPE_B);
            if (picData->picture_param.vol_fields.bits.interlaced) {
                // only MPEG-4 studio profile can have field coding. All other profiles
                // use frame coding only, i.e, there is no field VOP.  (see vop_structure in MP4 spec)
                mAcquiredBuffer->renderBuffer.scanFormat = VA_BOTTOM_FIELD | VA_TOP_FIELD;
            } else {
                mAcquiredBuffer->renderBuffer.scanFormat = VA_FRAME_PICTURE;
            }
            // TODO:  set discontinuity flag
            mAcquiredBuffer->renderBuffer.flag = 0;
            mAcquiredBuffer->renderBuffer.timeStamp = mCurrentPTS;

            if (codingType == MP4_VOP_TYPE_I || codingType == MP4_VOP_TYPE_P) {
                mLastVOPCodingType = codingType;
                mLastVOPTimeIncrement = picData->vop_time_increment;
            }

            // start decoding a frame
            vaStatus = vaBeginPicture(mVADisplay, mVAContext, mAcquiredBuffer->renderBuffer.surface);
            CHECK_VA_STATUS("vaBeginPicture");

            mDecodingFrame = true;
            mSendIQMatrixBuf = true;
        }

        status = decodeSlice(data, picData);
        CHECK_STATUS("decodeSlice");
    }

    return DECODE_SUCCESS;
}


Decode_Status VideoDecoderMPEG4::decodeSlice(vbp_data_mp42 *data, vbp_picture_data_mp42 *picData) {
    Decode_Status status;
    VAStatus vaStatus;
    uint32_t bufferIDCount = 0;
    // maximum 4 buffers to render a slice: picture parameter, IQMatrix, slice parameter, slice data
    VABufferID bufferIDs[4];

    VAPictureParameterBufferMPEG4 *picParam = &(picData->picture_param);
    vbp_slice_data_mp42 *sliceData = &(picData->slice_data);
    VASliceParameterBufferMPEG4 *sliceParam = &(sliceData->slice_param);

    // send picture parametre for each slice
    status = setReference(picParam);
    CHECK_STATUS("setReference");

    vaStatus = vaCreateBuffer(
        mVADisplay,
        mVAContext,
        VAPictureParameterBufferType,
        sizeof(VAPictureParameterBufferMPEG4),
        1,
        picParam,
        &bufferIDs[bufferIDCount]);
    CHECK_VA_STATUS("vaCreatePictureParameterBuffer");

    bufferIDCount++;
    if (picParam->vol_fields.bits.quant_type && mSendIQMatrixBuf)
    {
        // only send IQ matrix for the first slice in the picture
        vaStatus = vaCreateBuffer(
            mVADisplay,
            mVAContext,
            VAIQMatrixBufferType,
            sizeof(VAIQMatrixBufferMPEG4),
            1,
            &(data->iq_matrix_buffer),
            &bufferIDs[bufferIDCount]);
        CHECK_VA_STATUS("vaCreateIQMatrixBuffer");

        mSendIQMatrixBuf = false;
        bufferIDCount++;
    }

    vaStatus = vaCreateBuffer(
        mVADisplay,
        mVAContext,
        VASliceParameterBufferType,
        sizeof(VASliceParameterBufferMPEG4),
        1,
        sliceParam,
        &bufferIDs[bufferIDCount]);
    CHECK_VA_STATUS("vaCreateSliceParameterBuffer");

    bufferIDCount++;

    //slice data buffer pointer
    //Note that this is the original data buffer ptr;
    // offset to the actual slice data is provided in
    // slice_data_offset in VASliceParameterBufferMP42

    vaStatus = vaCreateBuffer(
        mVADisplay,
        mVAContext,
        VASliceDataBufferType,
        sliceData->slice_size, //size
        1,        //num_elements
        sliceData->buffer_addr + sliceData->slice_offset,
        &bufferIDs[bufferIDCount]);
    CHECK_VA_STATUS("vaCreateSliceDataBuffer");

    bufferIDCount++;

    vaStatus = vaRenderPicture(
        mVADisplay,
        mVAContext,
        bufferIDs,
        bufferIDCount);
    CHECK_VA_STATUS("vaRenderPicture");


    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderMPEG4::setReference(VAPictureParameterBufferMPEG4 *picParam) {
    switch (picParam->vop_fields.bits.vop_coding_type) {
        case MP4_VOP_TYPE_I:
            picParam->forward_reference_picture = VA_INVALID_SURFACE;
            picParam->backward_reference_picture = VA_INVALID_SURFACE;
            break;
        case MP4_VOP_TYPE_P:
            if (mLastReference == NULL&& mIsSyncFrame == false) {
                return DECODE_NO_REFERENCE;
            }
            if (mLastReference != NULL) {
                picParam->forward_reference_picture = mLastReference->renderBuffer.surface;
            } else {
                picParam->forward_reference_picture = VA_INVALID_SURFACE;
            }
            picParam->backward_reference_picture = VA_INVALID_SURFACE;
            break;
        case MP4_VOP_TYPE_B:
            picParam->vop_fields.bits.backward_reference_vop_coding_type = mLastVOPCodingType;
            // WEIRD, CHECK AGAIN !!!!!!!
            picParam->forward_reference_picture = mLastReference->renderBuffer.surface;
            picParam->backward_reference_picture = mForwardReference->renderBuffer.surface;
            break;
        case MP4_VOP_TYPE_S:
            // WEIRD, CHECK AGAIN!!!! WAS using mForwardReference
            if (mLastReference == NULL) {
                return DECODE_NO_REFERENCE;
            }
            picParam->forward_reference_picture = mLastReference->renderBuffer.surface;
            picParam->backward_reference_picture = VA_INVALID_SURFACE;
            break;

        default:
            // Will never reach here;
            return DECODE_PARSER_FAIL;
    }
    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderMPEG4::startVA(vbp_data_mp42 *data) {
    updateFormatInfo(data);

    VAProfile vaProfile;

    if ((data->codec_data.profile_and_level_indication & 0xF8) == 0xF0) {
        vaProfile = VAProfileMPEG4AdvancedSimple;
    } else {
        vaProfile = VAProfileMPEG4Simple;
    }

    return VideoDecoderBase::setupVA(MP4_SURFACE_NUMBER, vaProfile);
}

void VideoDecoderMPEG4::updateFormatInfo(vbp_data_mp42 *data) {
    ITRACE("updateFormatInfo: current size: %d x %d, new size: %d x %d",
        mVideoFormatInfo.width, mVideoFormatInfo.height,
        data->codec_data.video_object_layer_width,
        data->codec_data.video_object_layer_height);

    if (mVideoFormatInfo.width != (int32_t)data->codec_data.video_object_layer_width ||
        mVideoFormatInfo.height != (int32_t)data->codec_data.video_object_layer_height) {
        // update  encoded image size
        mVideoFormatInfo.width = data->codec_data.video_object_layer_width;
        mVideoFormatInfo.height = data->codec_data.video_object_layer_height;
        mSizeChanged = true;
        ITRACE("Video size is changed.");
    }

    // video_range has default value of 0. Y ranges from 16 to 235.
    mVideoFormatInfo.videoRange = data->codec_data.video_range;

    switch (data->codec_data.matrix_coefficients) {
        case 1:
            mVideoFormatInfo.colorMatrix = VA_SRC_BT709;
            break;

        // ITU-R Recommendation BT.470-6 System B, G (MP4), same as
        // SMPTE 170M/BT601
        case 5:
        case 6:
            mVideoFormatInfo.colorMatrix = VA_SRC_BT601;
            break;

        default:
            // unknown color matrix, set to 0 so color space flag will not be set.
            mVideoFormatInfo.colorMatrix = 0;
            break;
    }

    mVideoFormatInfo.aspectX = data->codec_data.par_width;
    mVideoFormatInfo.aspectY = data->codec_data.par_height;
    //mVideoFormatInfo.bitrate = data->codec_data.bit_rate;
    mVideoFormatInfo.valid = true;
}
