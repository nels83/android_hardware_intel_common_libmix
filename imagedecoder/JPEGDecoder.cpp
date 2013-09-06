/* INTEL CONFIDENTIAL
* Copyright (c) 2012, 2013 Intel Corporation.  All rights reserved.
* Copyright (c) Imagination Technologies Limited, UK
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
* Authors:
*    Nana Guo <nana.n.guo@intel.com>
*    Yao Cheng <yao.cheng@intel.com>
*
*/
//#define LOG_NDEBUG 0

#include <va/va.h>
#include <va/va_tpi.h>
#include "JPEGDecoder.h"
#include "JPEGParser.h"
#include "JPEGBlitter.h"
#include "ImageDecoderTrace.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

//#define LOG_TAG "ImageDecoder"

#define JPEG_MAX_SETS_HUFFMAN_TABLES 2

#define TABLE_CLASS_DC  0
#define TABLE_CLASS_AC  1
#define TABLE_CLASS_NUM 2

// for config
#define HW_DECODE_MIN_WIDTH  100 // for JPEG smaller than this, use SW decode
#define HW_DECODE_MIN_HEIGHT 100 // for JPEG smaller than this, use SW decode

typedef uint32_t Display;

#define JD_CHECK(err, label) \
        if (err) { \
            ETRACE("%s::%d: failed: %d", __PRETTY_FUNCTION__, __LINE__, err); \
            goto label; \
        }

#define JD_CHECK_RET(err, label, retcode) \
        if (err) { \
            status = retcode; \
            ETRACE("%s::%d: failed: %d", __PRETTY_FUNCTION__, __LINE__, err); \
            goto label; \
        }

JpegDecoder::JpegDecoder()
    :mInitialized(false),
    mDisplay(0),
    mConfigId(VA_INVALID_ID),
    mContextId(VA_INVALID_ID),
    mParser(NULL),
    mBlitter(NULL)
{
    mParser = new CJPEGParse;
    mBlitter = new JpegBlitter;
    Display dpy;
    int va_major_version, va_minor_version;
    mDisplay = vaGetDisplay(&dpy);
    vaInitialize(mDisplay, &va_major_version, &va_minor_version);
}
JpegDecoder::~JpegDecoder()
{
    if (mInitialized) {
        WTRACE("Freeing JpegDecoder: not destroyed yet. Force destroy resource");
        deinit();
    }
    delete mBlitter;
    vaTerminate(mDisplay);
    delete mParser;
}

JpegDecoder::MapHandle JpegDecoder::mapData(RenderTarget &target, void ** data, uint32_t * offsets, uint32_t * pitches)
{
    JpegDecoder::MapHandle handle;
    handle.img = NULL;
    handle.valid = false;
    VASurfaceID surf_id = getSurfaceID(target);
    if (surf_id != VA_INVALID_ID) {
        handle.img = new VAImage();
        if (handle.img == NULL) {
            ETRACE("%s: create VAImage fail", __FUNCTION__);
            return handle;
        }
        VAStatus st;
        st = vaDeriveImage(mDisplay, surf_id, handle.img);
        if (st != VA_STATUS_SUCCESS) {
            delete handle.img;
            handle.img = NULL;
            ETRACE("%s: vaDeriveImage fail %d", __FUNCTION__, st);
            return handle;
        }
        st = vaMapBuffer(mDisplay, handle.img->buf, data);
        if (st != VA_STATUS_SUCCESS) {
            vaDestroyImage(mDisplay, handle.img->image_id);
            delete handle.img;
            handle.img = NULL;
            ETRACE("%s: vaMapBuffer fail %d", __FUNCTION__, st);
            return handle;
        }
        handle.valid = true;
        offsets[0] = handle.img->offsets[0];
        offsets[1] = handle.img->offsets[1];
        offsets[2] = handle.img->offsets[2];
        pitches[0] = handle.img->pitches[0];
        pitches[1] = handle.img->pitches[1];
        pitches[2] = handle.img->pitches[2];
        return handle;
    }
    ETRACE("%s: get Surface ID fail", __FUNCTION__);
    return handle;
}

void JpegDecoder::unmapData(RenderTarget &target, JpegDecoder::MapHandle maphandle)
{
    if (maphandle.valid == false)
        return;
    if (maphandle.img != NULL) {
        vaUnmapBuffer(mDisplay, maphandle.img->buf);
        vaDestroyImage(mDisplay, maphandle.img->image_id);
        delete maphandle.img;
    }
}

JpegDecodeStatus JpegDecoder::init(int w, int h, RenderTarget **targets, int num)
{
    if (mInitialized)
        return JD_ALREADY_INITIALIZED;
    Mutex::Autolock autoLock(mLock);
    mBlitter->setDecoder(*this);
    if (!mInitialized) {
        mGrallocSurfaceMap.clear();
        mDrmSurfaceMap.clear();
        mNormalSurfaceMap.clear();
        VAStatus st;
        VASurfaceID surfid;
        for (int i = 0; i < num; ++i) {
            JpegDecodeStatus st = createSurfaceFromRenderTarget(*targets[i], &surfid);
            if (st != JD_SUCCESS || surfid == VA_INVALID_ID) {
                ETRACE("%s failed to create surface from RenderTarget handle 0x%x",
                    __FUNCTION__, targets[i]->handle);
                return JD_RESOURCE_FAILURE;
            }
        }
        VAConfigAttrib attrib;

        attrib.type = VAConfigAttribRTFormat;
        st = vaGetConfigAttributes(mDisplay, VAProfileJPEGBaseline, VAEntrypointVLD, &attrib, 1);
        if (st != VA_STATUS_SUCCESS) {
            ETRACE("vaGetConfigAttributes failed. va_status = 0x%x", st);
            return JD_INITIALIZATION_ERROR;
        }
        st = vaCreateConfig(mDisplay, VAProfileJPEGBaseline, VAEntrypointVLD, &attrib, 1, &mConfigId);
        if (st != VA_STATUS_SUCCESS) {
            ETRACE("vaCreateConfig failed. va_status = 0x%x", st);
            return JD_INITIALIZATION_ERROR;
        }
        mContextId = VA_INVALID_ID;
        size_t gmsize = mGrallocSurfaceMap.size();
        size_t dmsize = mDrmSurfaceMap.size();
        size_t nmsize = mNormalSurfaceMap.size();
        VASurfaceID *surfaces = new VASurfaceID[gmsize + dmsize + nmsize];
        for (size_t i = 0; i < gmsize + dmsize + nmsize; ++i) {
            if (i < gmsize)
                surfaces[i] = mGrallocSurfaceMap.valueAt(i);
            else if (i < gmsize + dmsize)
                surfaces[i] = mDrmSurfaceMap.valueAt(i - gmsize);
            else
                surfaces[i] = mNormalSurfaceMap.valueAt(i - gmsize - dmsize);
        }
        st = vaCreateContext(mDisplay, mConfigId,
            w, h,
            0,
            surfaces, gmsize + dmsize + nmsize,
            &mContextId);
        delete[] surfaces;
        if (st != VA_STATUS_SUCCESS) {
            ETRACE("vaCreateContext failed. va_status = 0x%x", st);
            return JD_INITIALIZATION_ERROR;
        }

        VTRACE("vaconfig = %u, vacontext = %u", mConfigId, mContextId);
        mInitialized = true;
    }
    return JD_SUCCESS;
}

JpegDecodeStatus JpegDecoder::blit(RenderTarget &src, RenderTarget &dst)
{
    return mBlitter->blit(src, dst);
}

JpegDecodeStatus JpegDecoder::parse(JpegInfo &jpginfo)
{
    uint32_t component_order = 0 ;
    uint32_t dqt_ind = 0;
    uint32_t dht_ind = 0;
    uint32_t scan_ind = 0;
    bool frame_marker_found = false;
    int i;

    parserInitialize(mParser, jpginfo.buf, jpginfo.bufsize);

    uint8_t marker = mParser->getNextMarker(mParser);

    while (marker != CODE_EOI &&( !mParser->endOfBuffer(mParser))) {
        switch (marker) {
            case CODE_SOI: {
                 jpginfo.soi_offset = mParser->getByteOffset(mParser) - 2;
                break;
            }
            // If the marker is an APP marker skip over the data
            case CODE_APP0:
            case CODE_APP1:
            case CODE_APP2:
            case CODE_APP3:
            case CODE_APP4:
            case CODE_APP5:
            case CODE_APP6:
            case CODE_APP7:
            case CODE_APP8:
            case CODE_APP9:
            case CODE_APP10:
            case CODE_APP11:
            case CODE_APP12:
            case CODE_APP13:
            case CODE_APP14:
            case CODE_APP15: {

                uint32_t bytes_to_burn = mParser->readBytes(mParser, 2) - 2;
                mParser->burnBytes(mParser, bytes_to_burn);
                    break;
            }
            // Store offset to DQT data to avoid parsing bitstream in user mode
            case CODE_DQT: {
                if (dqt_ind < 4) {
                    jpginfo.dqt_byte_offset[dqt_ind] = mParser->getByteOffset(mParser) - jpginfo.soi_offset;
                    dqt_ind++;
                    uint32_t bytes_to_burn = mParser->readBytes(mParser, 2 ) - 2;
                    mParser->burnBytes( mParser, bytes_to_burn );
                } else {
                    ETRACE("ERROR: Decoder does not support more than 4 Quant Tables\n");
                    return JD_ERROR_BITSTREAM;
                }
                break;
            }
            // Throw exception for all SOF marker other than SOF0
            case CODE_SOF1:
            case CODE_SOF2:
            case CODE_SOF3:
            case CODE_SOF5:
            case CODE_SOF6:
            case CODE_SOF7:
            case CODE_SOF8:
            case CODE_SOF9:
            case CODE_SOF10:
            case CODE_SOF11:
            case CODE_SOF13:
            case CODE_SOF14:
            case CODE_SOF15: {
                ETRACE("ERROR: unsupport SOF\n");
                break;
            }
            // Parse component information in SOF marker
            case CODE_SOF_BASELINE: {
                frame_marker_found = true;

                mParser->burnBytes(mParser, 2); // Throw away frame header length
                uint8_t sample_precision = mParser->readNextByte(mParser);
                if (sample_precision != 8) {
                    ETRACE("sample_precision is not supported\n");
                    return JD_ERROR_BITSTREAM;
                }
                // Extract pic width and height
                jpginfo.picture_param_buf.picture_height = mParser->readBytes(mParser, 2);
                jpginfo.picture_param_buf.picture_width = mParser->readBytes(mParser, 2);
                jpginfo.picture_param_buf.num_components = mParser->readNextByte(mParser);

                if (jpginfo.picture_param_buf.num_components > JPEG_MAX_COMPONENTS) {
                    ETRACE("ERROR: reached max components\n");
                    return JD_ERROR_BITSTREAM;
                }
                if (jpginfo.picture_param_buf.picture_height < HW_DECODE_MIN_HEIGHT
                    || jpginfo.picture_param_buf.picture_width < HW_DECODE_MIN_WIDTH) {
                    VTRACE("PERFORMANCE: %ux%u JPEG will decode faster with SW\n",
                        jpginfo.picture_param_buf.picture_width,
                        jpginfo.picture_param_buf.picture_height);
                    return JD_ERROR_BITSTREAM;
                }
                uint8_t comp_ind = 0;
                for (comp_ind = 0; comp_ind < jpginfo.picture_param_buf.num_components; comp_ind++) {
                    jpginfo.picture_param_buf.components[comp_ind].component_id = mParser->readNextByte(mParser);

                    uint8_t hv_sampling = mParser->readNextByte(mParser);
                    jpginfo.picture_param_buf.components[comp_ind].h_sampling_factor = hv_sampling >> 4;
                    jpginfo.picture_param_buf.components[comp_ind].v_sampling_factor = hv_sampling & 0xf;
                    jpginfo.picture_param_buf.components[comp_ind].quantiser_table_selector = mParser->readNextByte(mParser);
                }


                break;
            }
            // Store offset to DHT data to avoid parsing bitstream in user mode
            case CODE_DHT: {
                if (dht_ind < 4) {
                    jpginfo.dht_byte_offset[dht_ind] = mParser->getByteOffset(mParser) - jpginfo.soi_offset;
                    dht_ind++;
                    uint32_t bytes_to_burn = mParser->readBytes(mParser, 2) - 2;
                    mParser->burnBytes(mParser, bytes_to_burn );
                } else {
                    ETRACE("ERROR: Decoder does not support more than 4 Huff Tables\n");
                    return JD_ERROR_BITSTREAM;
                }
                break;
            }
            // Parse component information in SOS marker
            case CODE_SOS: {
                mParser->burnBytes(mParser, 2);
                uint32_t component_in_scan = mParser->readNextByte(mParser);
                uint8_t comp_ind = 0;

                for (comp_ind = 0; comp_ind < component_in_scan; comp_ind++) {
                    uint8_t comp_id = mParser->readNextByte(mParser);
                    uint8_t comp_data_ind;
                    for (comp_data_ind = 0; comp_data_ind < jpginfo.picture_param_buf.num_components; comp_data_ind++) {
                        if (comp_id == jpginfo.picture_param_buf.components[comp_data_ind].component_id) {
                            jpginfo.slice_param_buf[scan_ind].components[comp_ind].component_selector = comp_data_ind + 1;
                            break;
                        }
                    }
                    uint8_t huffman_tables = mParser->readNextByte(mParser);
                    jpginfo.slice_param_buf[scan_ind].components[comp_ind].dc_table_selector = huffman_tables >> 4;
                    jpginfo.slice_param_buf[scan_ind].components[comp_ind].ac_table_selector = huffman_tables & 0xf;
                }
                uint32_t curr_byte = mParser->readNextByte(mParser); // Ss
                if (curr_byte != 0) {
                    ETRACE("ERROR: curr_byte 0x%08x != 0\n", curr_byte);
                    return JD_ERROR_BITSTREAM;
                }
                curr_byte = mParser->readNextByte(mParser);  // Se
                if (curr_byte != 0x3f) {
                    ETRACE("ERROR: curr_byte 0x%08x != 0x3f\n", curr_byte);
                    return JD_ERROR_BITSTREAM;
                }
                curr_byte = mParser->readNextByte(mParser);  // Ah, Al
                if (curr_byte != 0) {
                    ETRACE("ERROR: curr_byte 0x%08x != 0\n", curr_byte);
                    return JD_ERROR_BITSTREAM;
                }
                // Set slice control variables needed
                jpginfo.slice_param_buf[scan_ind].slice_data_offset = mParser->getByteOffset(mParser) - jpginfo.soi_offset;
                jpginfo.slice_param_buf[scan_ind].num_components = component_in_scan;
                if (scan_ind) {
                    /* If there is more than one scan, the slice for all but the final scan should only run up to the beginning of the next scan */
                    jpginfo.slice_param_buf[scan_ind - 1].slice_data_size =
                        (jpginfo.slice_param_buf[scan_ind].slice_data_offset - jpginfo.slice_param_buf[scan_ind - 1].slice_data_offset );;
                    }
                    scan_ind++;
                    jpginfo.scan_ctrl_count++;   // gsDXVA2Globals.uiScanCtrlCount
                    break;
                }
            case CODE_DRI: {
                uint32_t size =  mParser->readBytes(mParser, 2);
                jpginfo.slice_param_buf[scan_ind].restart_interval =  mParser->readBytes(mParser, 2);
                mParser->burnBytes(mParser, (size - 4));
                break;
            }
            default:
                break;
        }

        marker = mParser->getNextMarker(mParser);
        // If the EOI code is found, store the byte offset before the parsing finishes
        if( marker == CODE_EOI ) {
            jpginfo.eoi_offset = mParser->getByteOffset(mParser);
        }

    }

    jpginfo.quant_tables_num = dqt_ind;
    jpginfo.huffman_tables_num = dht_ind;

    /* The slice for the last scan should run up to the end of the picture */
    if (jpginfo.eoi_offset) {
        jpginfo.slice_param_buf[scan_ind - 1].slice_data_size = (jpginfo.eoi_offset - jpginfo.slice_param_buf[scan_ind - 1].slice_data_offset);
    }
    else {
        jpginfo.slice_param_buf[scan_ind - 1].slice_data_size = (jpginfo.bufsize - jpginfo.slice_param_buf[scan_ind - 1].slice_data_offset);
    }
    // throw AppException if SOF0 isn't found
    if (!frame_marker_found) {
        ETRACE("EEORR: Reached end of bitstream while trying to parse headers\n");
        return JD_ERROR_BITSTREAM;
    }

    JpegDecodeStatus status = parseTableData(jpginfo);
    if (status != JD_SUCCESS) {
        ETRACE("ERROR: Parsing table data returns %d", status);
        return JD_ERROR_BITSTREAM;
    }

    jpginfo.image_width = jpginfo.picture_param_buf.picture_width;
    jpginfo.image_height = jpginfo.picture_param_buf.picture_height;
    jpginfo.image_color_fourcc = sampFactor2Fourcc(jpginfo.picture_param_buf.components[0].h_sampling_factor,
        jpginfo.picture_param_buf.components[1].h_sampling_factor,
        jpginfo.picture_param_buf.components[2].h_sampling_factor,
        jpginfo.picture_param_buf.components[0].v_sampling_factor,
        jpginfo.picture_param_buf.components[1].v_sampling_factor,
        jpginfo.picture_param_buf.components[2].v_sampling_factor);
    jpginfo.image_pixel_format = fourcc2PixelFormat(jpginfo.image_color_fourcc);

    VTRACE("%s jpg %ux%u, fourcc=%s, pixelformat=0x%x",
        __FUNCTION__, jpginfo.image_width, jpginfo.image_height, fourcc2str(NULL, jpginfo.image_color_fourcc),
        jpginfo.image_pixel_format);

    if (!jpegColorFormatSupported(jpginfo))
        return JD_INPUT_FORMAT_UNSUPPORTED;
    return JD_SUCCESS;
}

JpegDecodeStatus JpegDecoder::createSurfaceFromRenderTarget(RenderTarget &target, VASurfaceID *surfid)
{
    if (target.type == RENDERTARGET_INTERNAL_BUFFER) {
        JpegDecodeStatus st = createSurfaceInternal(target.width,
            target.height,
            target.pixel_format,
            target.handle,
            surfid);
        if (st != JD_SUCCESS)
            return st;
        mNormalSurfaceMap.add(target.handle, *surfid);
        VTRACE("%s added surface %u (internal buffer id %d) to SurfaceList",
            __PRETTY_FUNCTION__, *surfid, target.handle);
    }
    else {
        switch (target.type) {
        case RenderTarget::KERNEL_DRM:
            {
                JpegDecodeStatus st = createSurfaceDrm(target.width,
                    target.height,
                    target.pixel_format,
                    (unsigned long)target.handle,
                    target.stride,
                    surfid);
                if (st != JD_SUCCESS)
                    return st;
                mDrmSurfaceMap.add((unsigned long)target.handle, *surfid);
                VTRACE("%s added surface %u (Drm handle %d) to DrmSurfaceMap",
                    __PRETTY_FUNCTION__, *surfid, target.handle);
            }
            break;
        case RenderTarget::ANDROID_GRALLOC:
            {
                JpegDecodeStatus st = createSurfaceGralloc(target.width,
                    target.height,
                    target.pixel_format,
                    (buffer_handle_t)target.handle,
                    target.stride,
                    surfid);
                if (st != JD_SUCCESS)
                    return st;
                mGrallocSurfaceMap.add((buffer_handle_t)target.handle, *surfid);
                VTRACE("%s added surface %u (Gralloc handle %d) to DrmSurfaceMap",
                    __PRETTY_FUNCTION__, *surfid, target.handle);
            }
            break;
        default:
            return JD_RENDER_TARGET_TYPE_UNSUPPORTED;
        }
    }
    return JD_SUCCESS;
}

JpegDecodeStatus JpegDecoder::createSurfaceInternal(int width, int height, int pixel_format, int handle, VASurfaceID *surf_id)
{
    VAStatus va_status;
    VASurfaceAttrib attrib;
    attrib.type = VASurfaceAttribPixelFormat;
    attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
    attrib.value.type = VAGenericValueTypeInteger;
    uint32_t fourcc = pixelFormat2Fourcc(pixel_format);
    uint32_t vaformat = fourcc2VaFormat(fourcc);
    attrib.value.value.i = fourcc;
    VTRACE("enter %s, pixel_format 0x%x, fourcc %s", __FUNCTION__, pixel_format, fourcc2str(NULL, fourcc));
    va_status = vaCreateSurfaces(mDisplay,
                                vaformat,
                                width,
                                height,
                                surf_id,
                                1,
                                &attrib,
                                1);
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("%s: createSurface (format %u, fourcc %s) returns %d", __PRETTY_FUNCTION__, vaformat, fourcc2str(NULL, fourcc), va_status);
        return JD_RESOURCE_FAILURE;
    }
    return JD_SUCCESS;
}

VASurfaceID JpegDecoder::getSurfaceID(RenderTarget &target) const
{
    int index;
    if (target.type == RENDERTARGET_INTERNAL_BUFFER) {
        index = mNormalSurfaceMap.indexOfKey(target.handle);
        if (index < 0)
            return VA_INVALID_ID;
        else
            return mNormalSurfaceMap.valueAt(index);
    }
    switch (target.type) {
    case RenderTarget::KERNEL_DRM:
        index = mDrmSurfaceMap.indexOfKey((unsigned long)target.handle);
        if (index < 0)
            return VA_INVALID_ID;
        else
            return mDrmSurfaceMap.valueAt(index);
    case RenderTarget::ANDROID_GRALLOC:
        index = mGrallocSurfaceMap.indexOfKey((buffer_handle_t)target.handle);
        if (index < 0)
            return VA_INVALID_ID;
        else
            return mGrallocSurfaceMap.valueAt(index);
    default:
        assert(false);
    }
    return VA_INVALID_ID;
}

JpegDecodeStatus JpegDecoder::sync(RenderTarget &target)
{
    VASurfaceID surf_id = getSurfaceID(target);
    if (surf_id == VA_INVALID_ID)
        return JD_INVALID_RENDER_TARGET;
    vaSyncSurface(mDisplay, surf_id);
    return JD_SUCCESS;
}
bool JpegDecoder::busy(RenderTarget &target) const
{
    VASurfaceStatus surf_st;
    VASurfaceID surf_id = getSurfaceID(target);
    if (surf_id == VA_INVALID_ID)
        return false;
    VAStatus st = vaQuerySurfaceStatus(mDisplay, surf_id, &surf_st);
    if (st != VA_STATUS_SUCCESS)
        return false;
    return surf_st != VASurfaceReady;
}


JpegDecodeStatus JpegDecoder::decode(JpegInfo &jpginfo, RenderTarget &target)
{
    VAStatus va_status = VA_STATUS_SUCCESS;
    VASurfaceStatus surf_status;
    VABufferID desc_buf[5];
    uint32_t bitstream_buffer_size = 0;
    uint32_t scan_idx = 0;
    uint32_t buf_idx = 0;
    uint32_t chopping = VA_SLICE_DATA_FLAG_ALL;
    uint32_t bytes_remaining;
    VASurfaceID surf_id = getSurfaceID(target);
    if (surf_id == VA_INVALID_ID)
        return JD_RENDER_TARGET_NOT_INITIALIZED;
    va_status = vaQuerySurfaceStatus(mDisplay, surf_id, &surf_status);
    if (surf_status != VASurfaceReady)
        return JD_RENDER_TARGET_BUSY;

    if (jpginfo.eoi_offset)
        bytes_remaining = jpginfo.eoi_offset - jpginfo.soi_offset;
    else
        bytes_remaining = jpginfo.bufsize - jpginfo.soi_offset;
    uint32_t src_offset = jpginfo.soi_offset;
    uint32_t cpy_row;
    bitstream_buffer_size = jpginfo.bufsize;//cinfo->src->bytes_in_buffer;//1024*1024*5;

    Vector<VABufferID> buf_list;
    va_status = vaBeginPicture(mDisplay, mContextId, surf_id);
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaBeginPicture failed. va_status = 0x%x", va_status);
        return JD_DECODE_FAILURE;
    }
    va_status = vaCreateBuffer(mDisplay, mContextId, VAPictureParameterBufferType, sizeof(VAPictureParameterBufferJPEGBaseline), 1, &jpginfo.picture_param_buf, &desc_buf[buf_idx]);
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaCreateBuffer VAPictureParameterBufferType failed. va_status = 0x%x", va_status);
        return JD_RESOURCE_FAILURE;
    }
    buf_list.add(desc_buf[buf_idx++]);
    va_status = vaCreateBuffer(mDisplay, mContextId, VAIQMatrixBufferType, sizeof(VAIQMatrixBufferJPEGBaseline), 1, &jpginfo.qmatrix_buf, &desc_buf[buf_idx]);

    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaCreateBuffer VAIQMatrixBufferType failed. va_status = 0x%x", va_status);
        return JD_RESOURCE_FAILURE;
    }
    buf_list.add(desc_buf[buf_idx++]);
    va_status = vaCreateBuffer(mDisplay, mContextId, VAHuffmanTableBufferType, sizeof(VAHuffmanTableBufferJPEGBaseline), 1, &jpginfo.hufman_table_buf, &desc_buf[buf_idx]);
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaCreateBuffer VAHuffmanTableBufferType failed. va_status = 0x%x", va_status);
        return JD_RESOURCE_FAILURE;
    }
    buf_list.add(desc_buf[buf_idx++]);

    do {
        /* Get Bitstream Buffer */
        uint32_t bytes = ( bytes_remaining < bitstream_buffer_size ) ? bytes_remaining : bitstream_buffer_size;
        bytes_remaining -= bytes;
        /* Get Slice Control Buffer */
        VASliceParameterBufferJPEGBaseline dest_scan_ctrl[JPEG_MAX_COMPONENTS];
        uint32_t src_idx = 0;
        uint32_t dest_idx = 0;
        memset(dest_scan_ctrl, 0, sizeof(dest_scan_ctrl));
        for (src_idx = scan_idx; src_idx < jpginfo.scan_ctrl_count ; src_idx++) {
            if (jpginfo.slice_param_buf[ src_idx ].slice_data_offset) {
                /* new scan, reset state machine */
                chopping = VA_SLICE_DATA_FLAG_ALL;
                VTRACE("Scan:%i FileOffset:%x Bytes:%x \n", src_idx,
                    jpginfo.slice_param_buf[ src_idx ].slice_data_offset,
                    jpginfo.slice_param_buf[ src_idx ].slice_data_size );
                /* does the slice end in the buffer */
                if (jpginfo.slice_param_buf[ src_idx ].slice_data_offset + jpginfo.slice_param_buf[ src_idx ].slice_data_size > bytes + src_offset) {
                    chopping = VA_SLICE_DATA_FLAG_BEGIN;
                }
            } else {
                if (jpginfo.slice_param_buf[ src_idx ].slice_data_size > bytes) {
                    chopping = VA_SLICE_DATA_FLAG_MIDDLE;
                } else {
                    if ((chopping == VA_SLICE_DATA_FLAG_BEGIN) || (chopping == VA_SLICE_DATA_FLAG_MIDDLE)) {
                        chopping = VA_SLICE_DATA_FLAG_END;
                    }
                }
            }
            dest_scan_ctrl[dest_idx].slice_data_flag = chopping;

            if ((chopping == VA_SLICE_DATA_FLAG_ALL) || (chopping == VA_SLICE_DATA_FLAG_BEGIN))
                dest_scan_ctrl[dest_idx].slice_data_offset = jpginfo.slice_param_buf[ src_idx ].slice_data_offset;
            else
                dest_scan_ctrl[dest_idx].slice_data_offset = 0;

            const int32_t bytes_in_seg = bytes - dest_scan_ctrl[dest_idx].slice_data_offset;
            const uint32_t scan_data = (bytes_in_seg < jpginfo.slice_param_buf[src_idx].slice_data_size) ? bytes_in_seg : jpginfo.slice_param_buf[src_idx].slice_data_size ;
            jpginfo.slice_param_buf[src_idx].slice_data_offset = 0;
            jpginfo.slice_param_buf[src_idx].slice_data_size -= scan_data;
            dest_scan_ctrl[dest_idx].slice_data_size = scan_data;
            dest_scan_ctrl[dest_idx].num_components = jpginfo.slice_param_buf[src_idx].num_components;
            dest_scan_ctrl[dest_idx].restart_interval = jpginfo.slice_param_buf[src_idx].restart_interval;
            memcpy(&dest_scan_ctrl[dest_idx].components, & jpginfo.slice_param_buf[ src_idx ].components,
                sizeof(jpginfo.slice_param_buf[ src_idx ].components) );
            dest_idx++;
            if ((chopping == VA_SLICE_DATA_FLAG_ALL) || (chopping == VA_SLICE_DATA_FLAG_END)) { /* all good good */
            } else {
                break;
            }
        }
        scan_idx = src_idx;
        /* Get Slice Control Buffer */
        va_status = vaCreateBuffer(mDisplay, mContextId, VASliceParameterBufferType, sizeof(VASliceParameterBufferJPEGBaseline) * dest_idx, 1, dest_scan_ctrl, &desc_buf[buf_idx]);
        if (va_status != VA_STATUS_SUCCESS) {
            ETRACE("vaCreateBuffer VASliceParameterBufferType failed. va_status = 0x%x", va_status);
            return JD_RESOURCE_FAILURE;
        }
        buf_list.add(desc_buf[buf_idx++]);
        va_status = vaCreateBuffer(mDisplay, mContextId, VASliceDataBufferType, bytes, 1, &jpginfo.buf[ src_offset ], &desc_buf[buf_idx]);
        buf_list.add(desc_buf[buf_idx++]);
        if (va_status != VA_STATUS_SUCCESS) {
            ETRACE("vaCreateBuffer VASliceDataBufferType (%u bytes) failed. va_status = 0x%x", bytes, va_status);
            return JD_RESOURCE_FAILURE;
        }
        va_status = vaRenderPicture( mDisplay, mContextId, desc_buf, buf_idx);
        if (va_status != VA_STATUS_SUCCESS) {
            ETRACE("vaRenderPicture failed. va_status = 0x%x", va_status);
            return JD_DECODE_FAILURE;
        }
        buf_idx = 0;

        src_offset += bytes;
    } while (bytes_remaining);

    va_status = vaEndPicture(mDisplay, mContextId);

    while(buf_list.size() > 0) {
        vaDestroyBuffer(mDisplay, buf_list.top());
        buf_list.pop();
    }
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaEndPicture failed. va_status = 0x%x", va_status);
        return JD_DECODE_FAILURE;
    }
    return JD_SUCCESS;
}
void JpegDecoder::deinit()
{
    if (mInitialized) {
        Mutex::Autolock autoLock(mLock);
        if (mInitialized) {
            vaDestroyContext(mDisplay, mContextId);
            vaDestroyConfig(mDisplay, mConfigId);
            mInitialized = false;
            size_t gralloc_size = mGrallocSurfaceMap.size();
            size_t drm_size = mDrmSurfaceMap.size();
            size_t internal_surf_size = mNormalSurfaceMap.size();
            for (size_t i = 0; i < gralloc_size; ++i) {
                VASurfaceID surf_id = mGrallocSurfaceMap.valueAt(i);
                vaDestroySurfaces(mDisplay, &surf_id, 1);
            }
            for (size_t i = 0; i < drm_size; ++i) {
                VASurfaceID surf_id = mDrmSurfaceMap.valueAt(i);
                vaDestroySurfaces(mDisplay, &surf_id, 1);
            }
            for (size_t i = 0; i < internal_surf_size; ++i) {
                VASurfaceID surf_id = mNormalSurfaceMap.valueAt(i);
                vaDestroySurfaces(mDisplay, &surf_id, 1);
            }
            mGrallocSurfaceMap.clear();
            mDrmSurfaceMap.clear();
            mNormalSurfaceMap.clear();
        }
    }
}

JpegDecodeStatus JpegDecoder::parseTableData(JpegInfo &jpginfo) {
    parserInitialize(mParser, jpginfo.buf, jpginfo.bufsize);
    // Parse Quant tables
    memset(&jpginfo.qmatrix_buf, 0, sizeof(jpginfo.qmatrix_buf));
    uint32_t dqt_ind = 0;
    for (dqt_ind = 0; dqt_ind < jpginfo.quant_tables_num; dqt_ind++) {
        if (mParser->setByteOffset(mParser, jpginfo.dqt_byte_offset[dqt_ind])) {
            // uint32_t uiTableBytes = mParser->readBytes( 2 ) - 2;
            uint32_t table_bytes = mParser->readBytes( mParser, 2 ) - 2;
            do {
                uint32_t table_info = mParser->readNextByte(mParser);
                table_bytes--;
                uint32_t table_length = table_bytes > 64 ? 64 : table_bytes;
                uint32_t table_precision = table_info >> 4;
                if (table_precision != 0) {
                    ETRACE("%s ERROR: Parsing table data returns %d", __FUNCTION__, JD_ERROR_BITSTREAM);
                    return JD_ERROR_BITSTREAM;
                }
                uint32_t table_id = table_info & 0xf;

                jpginfo.qmatrix_buf.load_quantiser_table[table_id] = 1;

                if (table_id < JPEG_MAX_QUANT_TABLES) {
                    // Pull Quant table data from bitstream
                    uint32_t byte_ind;
                    for (byte_ind = 0; byte_ind < table_length; byte_ind++) {
                        jpginfo.qmatrix_buf.quantiser_table[table_id][byte_ind] = mParser->readNextByte(mParser);
                    }
                } else {
                    ETRACE("%s DQT table ID is not supported", __FUNCTION__);
                    mParser->burnBytes(mParser, table_length);
                }
                table_bytes -= table_length;
            } while (table_bytes);
        }
    }

    // Parse Huffman tables
    memset(&jpginfo.hufman_table_buf, 0, sizeof(jpginfo.hufman_table_buf));
    uint32_t dht_ind = 0;
    for (dht_ind = 0; dht_ind < jpginfo.huffman_tables_num; dht_ind++) {
        if (mParser->setByteOffset(mParser, jpginfo.dht_byte_offset[dht_ind])) {
            uint32_t table_bytes = mParser->readBytes( mParser, 2 ) - 2;
            do {
                uint32_t table_info = mParser->readNextByte(mParser);
                table_bytes--;
                uint32_t table_class = table_info >> 4; // Identifies whether the table is for AC or DC
                uint32_t table_id = table_info & 0xf;
                jpginfo.hufman_table_buf.load_huffman_table[table_id] = 1;

                if ((table_class < TABLE_CLASS_NUM) && (table_id < JPEG_MAX_SETS_HUFFMAN_TABLES)) {
                    if (table_class == 0) {
                        uint8_t* bits = mParser->getCurrentIndex(mParser);
                        // Find out the number of entries in the table
                        uint32_t table_entries = 0;
                        uint32_t bit_ind;
                        for (bit_ind = 0; bit_ind < 16; bit_ind++) {
                            jpginfo.hufman_table_buf.huffman_table[table_id].num_dc_codes[bit_ind] = bits[bit_ind];
                            table_entries += jpginfo.hufman_table_buf.huffman_table[table_id].num_dc_codes[bit_ind];
                        }

                        // Create table of code values
                        mParser->burnBytes(mParser, 16);
                        table_bytes -= 16;
                        uint32_t tbl_ind;
                        for (tbl_ind = 0; tbl_ind < table_entries; tbl_ind++) {
                            jpginfo.hufman_table_buf.huffman_table[table_id].dc_values[tbl_ind] = mParser->readNextByte(mParser);
                            table_bytes--;
                        }

                    } else { // for AC class
                        uint8_t* bits = mParser->getCurrentIndex(mParser);
                        // Find out the number of entries in the table
                        uint32_t table_entries = 0;
                        uint32_t bit_ind = 0;
                        for (bit_ind = 0; bit_ind < 16; bit_ind++) {
                            jpginfo.hufman_table_buf.huffman_table[table_id].num_ac_codes[bit_ind] = bits[bit_ind];
                            table_entries += jpginfo.hufman_table_buf.huffman_table[table_id].num_ac_codes[bit_ind];
                        }

                        // Create table of code values
                        mParser->burnBytes(mParser, 16);
                        table_bytes -= 16;
                        uint32_t tbl_ind = 0;
                        for (tbl_ind = 0; tbl_ind < table_entries; tbl_ind++) {
                            jpginfo.hufman_table_buf.huffman_table[table_id].ac_values[tbl_ind] = mParser->readNextByte(mParser);
                            table_bytes--;
                        }
                    }//end of else
                } else {
                    // Find out the number of entries in the table
                    ETRACE("%s DHT table ID is not supported", __FUNCTION__);
                    uint32_t table_entries = 0;
                    uint32_t bit_ind = 0;
                    for(bit_ind = 0; bit_ind < 16; bit_ind++) {
                        table_entries += mParser->readNextByte(mParser);
                        table_bytes--;
                    }
                    mParser->burnBytes(mParser, table_entries);
                    table_bytes -= table_entries;
                }

            } while (table_bytes);
        }
    }

    return JD_SUCCESS;
}

