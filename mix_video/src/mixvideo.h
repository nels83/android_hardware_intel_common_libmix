/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __MIX_VIDEO_H__
#define __MIX_VIDEO_H__



#include <mixdrmparams.h>
#include "mixvideoinitparams.h"
#include "mixvideoconfigparamsdec.h"
#include "mixvideodecodeparams.h"
#include "mixvideoconfigparamsenc.h"
#include "mixvideoencodeparams.h"
#include "mixvideorenderparams.h"
#include "mixvideocaps.h"
#include "mixbuffer.h"
#include "mixvideo_private.h"


class MixVideo;
/*
 * Virtual methods typedef
 */

typedef MIX_RESULT (*MixVideoGetVersionFunc)(MixVideo * mix, uint * major,
        uint * minor);

typedef MIX_RESULT (*MixVideoInitializeFunc)(MixVideo * mix, MixCodecMode mode,
        MixVideoInitParams * init_params, MixDrmParams * drm_init_params);

typedef MIX_RESULT (*MixVideoDeinitializeFunc)(MixVideo * mix);

typedef MIX_RESULT (*MixVideoConfigureFunc)(MixVideo * mix,
        MixVideoConfigParams * config_params,
        MixDrmParams * drm_config_params);

typedef MIX_RESULT (*MixVideoGetConfigFunc)(MixVideo * mix,
        MixVideoConfigParams ** config_params);

typedef MIX_RESULT (*MixVideoDecodeFunc)(MixVideo * mix, MixBuffer * bufin[],
        int bufincnt, MixVideoDecodeParams * decode_params);

typedef MIX_RESULT (*MixVideoGetFrameFunc)(MixVideo * mix,
        MixVideoFrame ** frame);

typedef MIX_RESULT (*MixVideoReleaseFrameFunc)(MixVideo * mix,
        MixVideoFrame * frame);

typedef MIX_RESULT (*MixVideoRenderFunc)(MixVideo * mix,
        MixVideoRenderParams * render_params, MixVideoFrame *frame);


typedef MIX_RESULT (*MixVideoEncodeFunc)(MixVideo * mix, MixBuffer * bufin[],
        int bufincnt, MixIOVec * iovout[], int iovoutcnt,
        MixVideoEncodeParams * encode_params);


typedef MIX_RESULT (*MixVideoFlushFunc)(MixVideo * mix);

typedef MIX_RESULT (*MixVideoEOSFunc)(MixVideo * mix);

typedef MIX_RESULT (*MixVideoGetStateFunc)(MixVideo * mix, MixState * state);

typedef MIX_RESULT (*MixVideoGetMixBufferFunc)(MixVideo * mix, MixBuffer ** buf);

typedef MIX_RESULT (*MixVideoReleaseMixBufferFunc)(MixVideo * mix,
        MixBuffer * buf);

typedef MIX_RESULT (*MixVideoGetMaxCodedBufferSizeFunc) (MixVideo * mix,
        uint *max_size);


typedef MIX_RESULT (*MixVideoSetDynamicEncConfigFunc) (MixVideo * mix,
        MixEncParamsType params_type, MixEncDynamicParams * dynamic_params);

typedef MIX_RESULT (*MixVideoGetNewUsrptrForSurfaceBufferFunc) (MixVideo * mix,
        uint width, uint height, uint format, 	uint expected_size,
        uint *outsize, uint * stride, uint8 **usrptr);

/**
 * MixVideo:
 * @parent: Parent object.
 *
 * MI-X Video object
 */
class MixVideo {
public:
    MixVideo();
    ~MixVideo();

public:
    /*< private > */
    void* context;
    uint ref_count;
    MixVideoPrivate mPriv;

public:
    /*< virtual public >*/
    MixVideoGetVersionFunc get_version_func;
    MixVideoInitializeFunc initialize_func;
    MixVideoDeinitializeFunc deinitialize_func;
    MixVideoConfigureFunc configure_func;
    MixVideoGetConfigFunc get_config_func;
    MixVideoDecodeFunc decode_func;
    MixVideoGetFrameFunc get_frame_func;
    MixVideoReleaseFrameFunc release_frame_func;
    MixVideoRenderFunc render_func;
    MixVideoEncodeFunc encode_func;
    MixVideoFlushFunc flush_func;
    MixVideoEOSFunc eos_func;
    MixVideoGetStateFunc get_state_func;
    MixVideoGetMixBufferFunc get_mix_buffer_func;
    MixVideoReleaseMixBufferFunc release_mix_buffer_func;
    MixVideoGetMaxCodedBufferSizeFunc get_max_coded_buffer_size_func;
    MixVideoSetDynamicEncConfigFunc set_dynamic_enc_config_func;
    MixVideoGetNewUsrptrForSurfaceBufferFunc get_new_usrptr_for_surface_buffer;
};

/**
 * mix_video_new:
 * @returns: A newly allocated instance of #MixVideo
 *
 * Use this method to create new instance of #MixVideo
 */
MixVideo *mix_video_new(void);

/**
 * mix_video_ref:
 * @mix: object to add reference
 * @returns: the MixVideo instance where reference count has been increased.
 *
 * Add reference count.
 */
MixVideo *mix_video_ref(MixVideo * mix);

/**
 * mix_video_unref:
 * @obj: object to unref.
 *
 * Decrement reference count of the object.
 */
MixVideo *
mix_video_unref(MixVideo * mix) ;

/* Class Methods */

/**
 * mix_video_get_version:
 * @mix: #MixVideo object.
 * @major: Pointer to an unsigned integer indicating the major version number of this MI-X Video library
 * @minor: Pointer to an unsigned integer indicating the minor version number of this MI-X Video library
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * This function will return the major and minor version numbers of the library.
 */
MIX_RESULT mix_video_get_version(MixVideo * mix, uint * major, uint * minor);



/**
 * mix_video_initialize:
 * @mix: #MixVideo object.
 * @mode: Enum value to indicate encode or decode mode
 * @init_params: MixVideoInitParams object which includes display type and pointer to display, encode or decode mode
 * @drm_init_params: MixDrmParams defined in <emphasis>Moorestown MI-X DRM API</emphasis>.
 *                   This can be null if content is not protected.
 * @returns: In addition to the <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>,
 *           the following error codes may be returned.
 * <itemizedlist>
 * <listitem>MIX_RESULT_ALREADY_INIT, mix_video_initialize() has already been called.</listitem>
 * </itemizedlist>
 *
 * This function will return the major and minor version numbers of the library.
 */
MIX_RESULT mix_video_initialize(MixVideo * mix, MixCodecMode mode,
                                MixVideoInitParams * init_params, MixDrmParams * drm_init_params);

/**
 * mix_video_deinitialize:
 * @mix: #MixVideo object.
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * This function will un-initialize a session with this MI-X instance. During this call, the
 * LibVA session is closed and all resources including surface buffers, #MixBuffers and
 * #MixVideoFrame objects are freed. This function is called by the application once
 * mix_video_initialize() is called, before exiting.
 */
MIX_RESULT mix_video_deinitialize(MixVideo * mix);


/**
 * mix_video_configure:
 * @mix: #MixVideo object.
 * @config_params: Pointer to #MixVideoConfigParams object (either #MixVideoConfigParamsDec or
 *                 #MixVideoConfigParamsEnc for specific media type)
 * @drm_config_params: Pointer to #MixDrmParams defined in <emphasis>Moorestown MI-X DRM API</emphasis>.
 *                     This can be null if content is not protected.
 * @returns: In addition to the <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>,
 *           the following error codes may be returned.
 * <itemizedlist>
 * <listitem>MIX_RESULT_RESOURCES_NOTAVAIL, HW accelerated decoding is not available.</listitem>
 * <listitem>MIX_RESULT_NOTSUPPORTED, A requested parameter is not supported or not available.</listitem>
 * </itemizedlist>
 *
 * This function can be used to configure a stream for the current session.
 *         The caller can use this function to do the following:
 * <itemizedlist>
 * <listitem>Choose frame ordering mode (display order or decode order)</listitem>
 * <listitem>Choose encode or decode mode</listitem>
 * <listitem>Choose whether display frames are enqueued for encode mode</listitem>
 * <listitem>Provide stream parameters</listitem>
 * </itemizedlist>

 * This function can only be called after mix_video_initialize() has been called
 */
MIX_RESULT mix_video_configure(MixVideo * mix,
                               MixVideoConfigParams * config_params,
                               MixDrmParams * drm_config_params);


/**
 * mix_video_get_config:
 * @mix: #MixVideo object.
 * @config_params: Pointer to pointer to #MixVideoConfigParams object defined in
 *                 description of mix_video_configure()
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * This function can be used to get the current configuration of a stream for the current session.
 * A #MixVideoConfigParams object will be returned, which can be used to get each of the
 * parameter current values. The caller will need to release this object when it is no
 * longer needed.
 *
 * This function can only be called once mix_video_configure() has been called.
 *
 * <note> See description of mix_video_configure() for #MixVideoConfigParams object details.
 * For mix_video_get_config(), all input parameter fields become OUT parameters.
 * </note>
 */
MIX_RESULT mix_video_get_config(MixVideo * mix,
                                MixVideoConfigParams ** config_params);

/**
 * mix_video_decode:
 * @mix: #MixVideo object.
 * @bufin: Array of pointers to #MixBuffer objects, described in mix_video_get_mixbuffer() *
 * @bufincnt: Number of #MixBuffer objects
 * @decode_params: #MixVideoDecodeParams object
 * @returns: In addition to the <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>,
 *           the following error codes may be returned.
 * <itemizedlist>
 *     <listitem>
 *           MIX_RESULT_OUTOFSURFACES, No surfaces available for decoding. Nothing will be done.
 *           Caller can try again with the same MixBuffers later when surfaces may have been freed.
 *     </listitem>
 * </itemizedlist>
 *
 * <para>
 * This function is used to initiate HW accelerated decoding of encoded data buffers. This
 * function is used to decode to a surface buffer, which can then be rendered using
 * mix_video_render().
 * Video data input buffers are provided in a scatter/gather list of reference counted
 * #MixBuffers. The input #MixBuffers are retained until a full frame of coded data is
 * accumulated, at which point it will be decoded and the input buffers released. The
 * decoded data will be stored in a surface buffer until it is rendered. The caller must
 * provide the presentation timestamp and any stream discontinuity for the video frame
 * for the encoded data, in the #MixVideoDecodeParams object. These will be preserved
 * and provided for the #MixVideoFrame object that contains the decoded data for this
 * frame data.
 * </para>
 *
 * <para>
 * As only one timestamp is passed in for the buffer, there should be no more than one
 * video frame included in the encoded data buffer provided in a single call to
 * mix_video_decode(). If partial frame data is passed in over multiple calls to
 * mix_video_decode(), the same timestamp should be provided with each call having
 * data associated with the same frame.
 * </para>
 *
 * <para>
 * The application should request a #MixBuffer object using mix_video_get_mixbuffer(),
 * initialize the #MixBuffer with the data pointer to the coded input data, along with the
 * size of the input data buffer, and optionally can provide a token value and a callback
 * function pointer. When the MixBuffer is released by both the application and #MixVideo,
 * the callback will be called and passed the token value and the input data buffer
 * pointer for any buffer management processing that the application needs or wants to
 * perform (such as releasing the actual coded data buffer that was assigned to that
 * #MixBuffer). MixBuffers are allocated in a pool, and the application determines the size
 * of this pool, which is passed to mix_video_configure() in #the MixVideoConfigParams object.
 * </para>
 */
MIX_RESULT mix_video_decode(MixVideo * mix, MixBuffer * bufin[], int bufincnt,
                            MixVideoDecodeParams * decode_params);


/**
 * mix_video_get_frame:
 * @mix: #MixVideo object.
 * @frame: A pointer to a pointer to a #MixVideoFrame object
 * @returns: In addition to the <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>,
 *           the following error codes may be returned.
 * <itemizedlist>
 *    <listitem>
 *        MIX_RESULT_FRAME_NOTAVAIL, No decoded frames are available.
 *    </listitem>
 *    <listitem>
 *        MIX_RESULT_EOS, No more decoded frames are available,
 *        since end of stream has been encountered.
 *     </listitem>
 * </itemizedlist>
 *
 * <para>
 * This function returns a frame object that represents the next frame ID and includes
 * timestamp and discontinuity information. If display frame ordering has been
 * configured, it is the next frame displayed. If decode order frame ordering has been
 * configured, it is the next frame decoded. In both cases the timestamp reflects the
 * presentation timestamp. For encode mode the frame order is always display order.
 * </para>
 *
 * <para>
 * The frame object is a reference counted object that represents the frame. The
 * application can retain this frame object as long as needed to display the frame and
 * redisplay as needed. At presentation time, the application can call mix_video_render()
 * with this frame object to display the frame immediately. When the application no
 * longer needs to display this frame, it should release the object by calling
 * mix_video_release_frame(). The application should not modify the reference count or
 * delete this object directly.
 * </para>
 */
MIX_RESULT mix_video_get_frame(MixVideo * mix, MixVideoFrame ** frame);



/**
 * mix_video_release_frame:
 * @mix: #MixVideo object.
 * @frame: A pointer to a #MixVideoFrame object, described in mix_video_get_frame()
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * This function releases a frame object that was acquired from mix_video_get_frame().
 */
MIX_RESULT mix_video_release_frame(MixVideo * mix, MixVideoFrame * frame);


/**
 * mix_video_render:
 * @mix: #MixVideo object.
 * @render_params: #MixVideoRenderParams object defined below,
 *                 which includes the display window and type,
 *                 src and dest image sizes, deinterlace info, clipping rectangles,
 *                 some post processing parameters, and so forth.
 * @frame: Pointer to a #MixVideoFrame object returned from mix_video_get_frame().
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * This function renders a video frame associated with a MixVideoFrame object to the display.
 * The display is either an X11 Pixmap or an X11 Window using the overlay.
 */
MIX_RESULT mix_video_render(MixVideo * mix,
                            MixVideoRenderParams * render_params, MixVideoFrame *frame);


/**
 * mix_video_encode:
 * @mix: #MixVideo object.
 * @bufin: Array of pointers to #MixBuffer objects, structure defined in mix_video_decode()
 * @bufincnt: Number of #MixBuffer objects
 * @iovout: Array of #MixIOVec structures, pointing to buffers allocated by the application
 * @iovoutcnt: Number of items in iovout array
 * @encode_params: #MixVideoEncodeParams object
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * <para>
 * This function is used to initiate HW accelerated encoding of uncompressed video input
 * buffers. The input buffers may either be uncompressed video in user space buffers, or
 * CI frame indexes from libCI captured frames. In order to use CI frame indexes, the
 * shared buffer mode should be indicated in the #MixVideoConfigParamsEnc object
 * provided to mix_video_configure().
 * </para>
 *
 * <para>
 * Video uncompressed data input buffers are provided in a scatter/gather list of
 * reference counted MixBuffers. The input #MixBuffers are considered a complete frame
 * of data, and are used for encoding before the input buffers are released. LibCI frame
 * indices may also be provided in MixBuffers.
 * </para>
 *
 * <para>
 * The encoded data will be copied to the output buffers provided in the array of
 * #MixIOVec structures, also in a scatter/gather list. These output buffers are allocated
 * by the application. The application can query for the proper size of buffer to allocate
 * for this, using mix_video_get_max_coded_buffer_size(). It is suggested that the
 * application create a pool of these buffers to pass in, for efficiency. The application will
 * also set the buffer_size field in the #MixIOVec structures to the allocated buffer size.
 * When the buffers are filled with encoded data by #MixVideo, the data_size will be set to
 * the encoded data size placed in the buffer. For any buffer not used for encoded data,
 * the data_size will be set to zero.
 * </para>
 *
 * <para>
 * Alternatively, if the application does not allocate the output buffers, the data pointers
 * in the #MixIOVec structures (still provided by the application) can be set to NULL,
 * whereupon #MixVideo will allocate a data buffer for each frame and set the data,
 * buffer_size and data_size pointers in the #MixIOVec structures accordingly.
 * </para>
 *
 * <note>
 * This is not an efficient method to handle these buffers and it is preferred that
 * the application provide pre-allocated buffers.
 * </note>
 *
 * <para>
 * The application should request a #MixBuffer object using mix_video_get_mixbuffer(),
 * initialize the #MixBuffer with the data pointer to the uncompressed input data or a LibCI
 * frame index, along with the size of the input data buffer, and optionally can provide a
 * token value and a callback function pointer. When the #MixBuffer is released by both
 * the application and #MixVideo, the callback will be called and passed the token value
 * and the input data buffer pointer for any buffer management processing that the
 * application needs or wants to perform (such as releasing the actual data buffer that
 * was assigned to that #MixBuffer). #MixBuffers are allocated in a pool, and the application
 * determines the size of this pool, which is passed to mix_video_configure() in the
 * #MixVideoConfigParams object.
 * </para>
 *
 * <para>
 * The application can choose to enable or disable display of the uncompressed video
 * frames using the need_display of the #MixVideoConfigParamsEnc object in
 * mix_video_configure(). If display is enabled, #MixVideoFrames are enqueued by
 * #MixVideo, to be requested by the application with mix_video_get_frame() and used to
 * provide to mix_video_render() for rendering before releasing with
 * mix_video_release_frame(). If display is disabled, no #MixVideoFrames will be
 * enqueued.
 * </para>
 *
 */
MIX_RESULT mix_video_encode(MixVideo * mix, MixBuffer * bufin[], int bufincnt,
                            MixIOVec * iovout[], int iovoutcnt,
                            MixVideoEncodeParams * encode_params);

/**
 * mix_video_flush:
 * @mix: #MixVideo object.
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * This function will flush all encoded and decoded buffers that are currently enqueued or
 * in the process of decoding. After this call, decoding can commence again, but would
 * need to start at the beginning of a sequence (for example, with no dependencies on
 * previously decoded reference frames).
 */
MIX_RESULT mix_video_flush(MixVideo * mix);

/**
 * mix_video_eos:
 * @mix: #MixVideo object.
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * This function will signal end of stream to #MixVideo. This can be used to finalize
 * decoding of the last frame and other end of stream processing. #MixVideo will complete
 * the decoding of all buffers received, and will continue to provide the decoded frame
 * objects by means of the mix_video_get_frame() until all frames have been provided,
 * at which point mix_video_get_frame() will return MIX_RESULT_EOS.
 */
MIX_RESULT mix_video_eos(MixVideo * mix);


/**
 * mix_video_get_state:
 * @mix: #MixVideo object.
 * @state: Current state of MI-X session.
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * This function returns the current state of the MI-X session.
 */
MIX_RESULT mix_video_get_state(MixVideo * mix, MixState * state);

/**
 * mix_video_get_mixbuffer:
 * @mix: #MixVideo object.
 * @buf: A pointer to a pointer to a #MixBuffer object
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * <para>
 * This function returns a frame object that represents the next frame ID and includes
 * timestamp and discontinuity information. If display frame ordering has been
 * configured, it is the next frame displayed. If decode order frame ordering has been
 * configured, it is the next frame decoded. In both cases the timestamp reflects the
 * presentation timestamp.
 * </para>
 *
 * <para>
 * The frame object is a reference counted object that represents the frame. The
 * application can retain this frame object as long as needed to display the frame and
 * redisplay as needed. At presentation time, the application can call mix_video_render()
 * with this frame object to display the frame immediately. When the application no
 * longer needs to display this frame, it should release the object by calling
 * mix_video_release_frame(). The application should not modify the reference count or
 * delete this object directly.
 * </para>
 *
 */
MIX_RESULT mix_video_get_mixbuffer(MixVideo * mix, MixBuffer ** buf);


/**
 * mix_video_release_mixbuffer:
 * @mix: #MixVideo object.
 * @buf: A pointer to a #MixBuffer object, described in mix_video_get_mixbuffer().
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * This function releases a frame object that was acquired from mix_video_get_mixbuffer().
 */
MIX_RESULT mix_video_release_mixbuffer(MixVideo * mix, MixBuffer * buf);


/**
 * mix_video_get_max_coded_buffer_size:
 * @mix: #MixVideo object.
 * @bufsize: Pointer to uint.
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * <para>
 * This function can be used to get the maximum size of encoded data buffer needed for
 * the mix_video_encode() call.
 * </para>
 * <para>
 * This function can only be called once mix_video_configure() has been called.
 * </para>
 */
MIX_RESULT mix_video_get_max_coded_buffer_size(MixVideo * mix, uint *bufsize);


/**
 * mix_video_set_dynamic_enc_config:
 * @mix: #MixVideo object.
 * @params_type: Dynamic encoder configuration type
 * @dynamic_params: Point to dynamic control data structure which includes the new value to be changed to
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * <para>
 * This function can be used to change the encoder parameters at run-time
 * </para>
 * <para>
 * Usually this function is after the encoding session is started.
 * </para>
 */

MIX_RESULT mix_video_set_dynamic_enc_config (MixVideo * mix,
        MixEncParamsType params_type, MixEncDynamicParams * dynamic_params);



/**
 * mix_video_get_new_userptr_for_surface_buffer:
 * @mix: #MixVideo object.
 * @width: Width of new surface to be created
 * @height: Height of new surface to be created
 * @format: Format of new surface to be created
 * @usrptr: User space pointer mapped from the new created VA surface
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * <para>
 * This function can be used to create a new VA surface and map the physical address to user space
 * </para>
 * <para>
 * Usually this function is before the encoding session is started.
 * </para>
 */
MIX_RESULT mix_video_get_new_userptr_for_surface_buffer (MixVideo * mix, uint width, uint height, uint format,
        uint expected_size, uint *outsize, uint * stride, uint8 **usrptr);
#endif /* __MIX_VIDEO_H__ */
