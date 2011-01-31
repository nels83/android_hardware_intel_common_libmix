/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

/**
 * SECTION:mixvideo
 * @short_description: Object to support a single stream decoding or encoding using hardware accelerated decoder/encoder.
 * @include: mixvideo.h
 *
 * #MixVideo objects are created by the MMF/App and utilized for main MI-X API functionality for video.
 *
 * The MixVideo object handles any of the video formats internally.
 * The App/MMF will pass a MixVideoConfigParamsDecH264/MixVideoConfigParamsDecVC1/
 * MixVideoConfigParamsEncH264/etc object to MixVideo in the mix_video_configure()
 * call. MixVideoInitParams, MixVideoDecodeParams, MixVideoEncodeParams, and
 * MixVideoRenderParams objects will be passed in the mix_video_initialize(),
 * mix_video_decode(), mix_video_encode() and mix_video_render() calls respectively.
 *
 * The application can take the following steps to decode video:
 * <itemizedlist>
 * <listitem>Create a mix_video object using mix_video_new()</listitem>
 * <listitem>Initialize the object using mix_video_initialize()</listitem>
 * <listitem>Configure the stream using mix_video_configure()</listitem>
 * <listitem>Decode frames using mix_video_decode()</listitem>
 * <listitem>Retrieve the decoded frames using mix_video_get_frame(). The decoded frames can be retrieved in decode order or display order.</listitem>
 * <listitem>At the presentation time, using the timestamp provided with the decoded frame, render the frame to an X11 Window using mix_video_render(). The frame can be retained for redrawing until the next frame is retrieved.</listitem>
 * <listitem>When the frame is no longer needed for redrawing, release the frame using mix_video_release_frame().</listitem>
 * </itemizedlist>
 *
 * For encoding, the application can take the following steps to encode video:
 * <itemizedlist>
 * <listitem>Create a mix_video object using mix_video_new()</listitem>
 * <listitem>Initialize the object using mix_video_initialize()</listitem>
 * <listitem>Configure the stream using mix_video_configure()</listitem>
 * <listitem>Encode frames using mix_video_encode()</listitem>
 * <listitem>Use the encoded data buffers as desired; for example, forward to a muxing component for saving to a file.</listitem>
 * <listitem>Retrieve the uncompressed frames for display using mix_video_get_frame().</listitem>
 * <listitem>At the presentation time, using the timestamp provided with the decoded frame, render the frame to an X11 Window using mix_video_render(). For encode, the frame should not be retained for redrawing after the initial rendering, due to resource limitations.</listitem>
 * <listitem>Release the frame using mix_video_release_frame().</listitem>
 * </itemizedlist>
 *
 */

#include <string.h>
#include <va/va.h>             /* libVA */

#ifndef ANDROID
#include <X11/Xlib.h>
#include <va/va_x11.h>
#else
#define Display unsigned int
//#include "mix_vagetdisplay.h"

#ifdef __cplusplus
extern "C" {
#endif

VADisplay vaGetDisplay (
    void *android_dpy
);

#ifdef __cplusplus
}
#endif


#endif

#include "mixvideolog.h"

#ifndef ANDROID
#include "mixdisplayx11.h"
#else
#include "mixdisplayandroid.h"
#endif
#include "mixvideoframe.h"

#include "mixframemanager.h"
#include "mixvideorenderparams.h"
#include "mixvideorenderparams_internal.h"

#include "mixvideoformat.h"
#include "mixvideoformat_vc1.h"
#include "mixvideoformat_h264.h"
#include "mixvideoformat_mp42.h"

#include "mixvideoconfigparamsdec_vc1.h"
#include "mixvideoconfigparamsdec_h264.h"
#include "mixvideoconfigparamsdec_mp42.h"

#if MIXVIDEO_ENCODE_ENABLE
#include "mixvideoformatenc.h"
#include "mixvideoformatenc_h264.h"
#include "mixvideoformatenc_mpeg4.h"
#include "mixvideoformatenc_preview.h"
#include "mixvideoformatenc_h263.h"

#include "mixvideoconfigparamsenc_h264.h"
#include "mixvideoconfigparamsenc_mpeg4.h"
#include "mixvideoconfigparamsenc_preview.h"
#include "mixvideoconfigparamsenc_h263.h"
#endif

#include "mixvideo.h"
#include "mixvideo_private.h"

#ifdef ANDROID
#define mix_strcmp strcmp
#else
#define mix_strcmp g_strcmp0
#endif

#define USE_OPAQUE_POINTER

#ifdef USE_OPAQUE_POINTER
#define MIX_VIDEO_PRIVATE(mix) (MixVideoPrivate *)(mix->context)
#else
#define MIX_VIDEO_PRIVATE(mix) MIX_VIDEO_GET_PRIVATE(mix)
#endif

#define CHECK_INIT(mix, priv) \
	if (!mix) { \
		return MIX_RESULT_NULL_PTR; \
	} \
	priv = MIX_VIDEO_PRIVATE(mix); \
	if (!priv->initialized) { \
		LOG_E( "Not initialized\n"); \
		return MIX_RESULT_NOT_INIT; \
	}

#define CHECK_INIT_CONFIG(mix, priv) \
	CHECK_INIT(mix, priv); \
	if (!priv->configured) { \
		LOG_E( "Not configured\n"); \
		return MIX_RESULT_NOT_CONFIGURED; \
	}

/*
 * default implementation of virtual methods
 */

MIX_RESULT mix_video_get_version_default(MixVideo * mix, guint * major,
		guint * minor);

MIX_RESULT mix_video_initialize_default(MixVideo * mix, MixCodecMode mode,
		MixVideoInitParams * init_params, MixDrmParams * drm_init_params);

MIX_RESULT mix_video_deinitialize_default(MixVideo * mix);

MIX_RESULT mix_video_configure_default(MixVideo * mix,
		MixVideoConfigParams * config_params, MixDrmParams * drm_config_params);

MIX_RESULT mix_video_get_config_default(MixVideo * mix,
		MixVideoConfigParams ** config_params);

MIX_RESULT mix_video_decode_default(MixVideo * mix, MixBuffer * bufin[],
		gint bufincnt, MixVideoDecodeParams * decode_params);

MIX_RESULT mix_video_get_frame_default(MixVideo * mix, MixVideoFrame ** frame);

MIX_RESULT mix_video_release_frame_default(MixVideo * mix,
		MixVideoFrame * frame);

MIX_RESULT mix_video_render_default(MixVideo * mix,
		MixVideoRenderParams * render_params, MixVideoFrame *frame);
#if MIXVIDEO_ENCODE_ENABLE
MIX_RESULT mix_video_encode_default(MixVideo * mix, MixBuffer * bufin[],
		gint bufincnt, MixIOVec * iovout[], gint iovoutcnt,
		MixVideoEncodeParams * encode_params);
#endif
MIX_RESULT mix_video_flush_default(MixVideo * mix);

MIX_RESULT mix_video_eos_default(MixVideo * mix);

MIX_RESULT mix_video_get_state_default(MixVideo * mix, MixState * state);

MIX_RESULT mix_video_get_mixbuffer_default(MixVideo * mix, MixBuffer ** buf);

MIX_RESULT mix_video_release_mixbuffer_default(MixVideo * mix, MixBuffer * buf);
#if MIXVIDEO_ENCODE_ENABLE
MIX_RESULT mix_video_get_max_coded_buffer_size_default (MixVideo * mix, guint *max_size);
#endif
MIX_RESULT mix_video_set_dynamic_enc_config_default (MixVideo * mix,
	MixEncParamsType params_type, MixEncDynamicParams * dynamic_params);

static void mix_video_finalize(MixVideo * obj);
MIX_RESULT mix_video_configure_decode(MixVideo * mix,
		MixVideoConfigParamsDec * config_params_dec,
		MixDrmParams * drm_config_params);

#if MIXVIDEO_ENCODE_ENABLE
MIX_RESULT mix_video_configure_encode(MixVideo * mix,
		MixVideoConfigParamsEnc * config_params_enc,
		MixDrmParams * drm_config_params);
#endif
static void mix_video_init(MixVideo * self);

MixVideo::MixVideo() {
	//context = malloc(sizeof(MixVideoPrivate));
	context = &mPriv;
	get_version_func = mix_video_get_version_default;
	initialize_func = mix_video_initialize_default;
	deinitialize_func = mix_video_deinitialize_default;
	configure_func = mix_video_configure_default;
	get_config_func = mix_video_get_config_default;
	decode_func = mix_video_decode_default;
	get_frame_func = mix_video_get_frame_default;
	release_frame_func = mix_video_release_frame_default;
	render_func = mix_video_render_default;
#if MIXVIDEO_ENCODE_ENABLE
	encode_func = mix_video_encode_default;
#endif
	flush_func = mix_video_flush_default;
	eos_func = mix_video_eos_default;
	get_state_func = mix_video_get_state_default;
	get_mix_buffer_func = mix_video_get_mixbuffer_default;
	release_mix_buffer_func = mix_video_release_mixbuffer_default;
#if MIXVIDEO_ENCODE_ENABLE
	get_max_coded_buffer_size_func = mix_video_get_max_coded_buffer_size_default;
	set_dynamic_enc_config_func = mix_video_set_dynamic_enc_config_default;
#endif
	mix_video_init(this);

	ref_count = 1;

}

MixVideo::~MixVideo(){
	mix_video_finalize(this);
}

static void mix_video_init(MixVideo * self) {

	MixVideoPrivate *priv = MIX_VIDEO_GET_PRIVATE(self);

#ifdef USE_OPAQUE_POINTER
	self->context = priv;
#else
	self->context = NULL;
#endif

	/* private structure initialization */
	mix_video_private_initialize(priv);
}

MixVideo *mix_video_new(void) {

	MixVideo *ret = new MixVideo;

	return ret;
}

void mix_video_finalize(MixVideo * mix) {

	/* clean up here. */

	mix_video_deinitialize(mix);
}

MixVideo *
mix_video_ref(MixVideo * mix) {
	if (NULL != mix)
		mix->ref_count ++;
	return mix;
}

MixVideo *
mix_video_unref(MixVideo * mix) {
	if(NULL != mix) {
		mix->ref_count --;
		if (mix->ref_count == 0) {
			delete mix;
			return NULL;
		}
	}
	return mix;
}

/* private methods */
#define MIXUNREF(obj, unref) if(obj) { unref(obj); obj = NULL; }

void mix_video_private_initialize(MixVideoPrivate* priv) {
	priv->initialized = FALSE;
	priv->configured = FALSE;

	/* libVA */
	priv->va_display = NULL;
	priv->va_major_version = -1;
	priv->va_major_version = -1;

	/* mix objects */
	priv->frame_manager = NULL;
	priv->video_format = NULL;
#if MIXVIDEO_ENCODE_ENABLE
	priv->video_format_enc = NULL; //for encoding
#endif
	priv->surface_pool = NULL;
	priv->buffer_pool = NULL;

	priv->codec_mode = MIX_CODEC_MODE_DECODE;
	priv->init_params = NULL;
	priv->drm_params = NULL;
	priv->config_params = NULL;
}

void mix_video_private_cleanup(MixVideoPrivate* priv) {

	VAStatus va_status;

	if (!priv) {
		return;
	}
#if MIXVIDEO_ENCODE_ENABLE
	if (priv->video_format_enc) {
		mix_videofmtenc_deinitialize(priv->video_format_enc);
	}
#endif
	MIXUNREF(priv->frame_manager, mix_framemanager_unref)
	MIXUNREF(priv->video_format, mix_videoformat_unref)
#if MIXVIDEO_ENCODE_ENABLE
	MIXUNREF(priv->video_format_enc, mix_videoformatenc_unref)
#endif
	//for encoding
	MIXUNREF(priv->buffer_pool, mix_bufferpool_unref)
	MIXUNREF(priv->surface_pool, mix_surfacepool_unref)
/*	MIXUNREF(priv->init_params, mix_videoinitparams_unref) */
	MIXUNREF(priv->drm_params, mix_drmparams_unref)
	MIXUNREF(priv->config_params, mix_videoconfigparams_unref)

	/* terminate libVA */
	if (priv->va_display) {
		va_status = vaTerminate(priv->va_display);
		LOG_V( "vaTerminate\n");
		if (va_status != VA_STATUS_SUCCESS) {
			LOG_W( "Failed vaTerminate\n");
		} else {
			priv->va_display = NULL;
		}
	}

	MIXUNREF(priv->init_params, mix_videoinitparams_unref)

	priv->va_major_version = -1;
	priv->va_major_version = -1;
	priv->codec_mode = MIX_CODEC_MODE_DECODE;
	priv->initialized = FALSE;
	priv->configured = FALSE;
}

/* The following methods are defined in MI-X API */

MIX_RESULT mix_video_get_version_default(MixVideo * mix, guint * major,
		guint * minor) {
	if (!mix || !major || !minor) {
		return MIX_RESULT_NULL_PTR;
	}

	*major = MIXVIDEO_CURRENT - MIXVIDEO_AGE;
	*minor = MIXVIDEO_AGE;

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_video_initialize_default(MixVideo * mix, MixCodecMode mode,
		MixVideoInitParams * init_params, MixDrmParams * drm_init_params) {

	MIX_RESULT ret = MIX_RESULT_FAIL;
	MixVideoPrivate *priv = NULL;
	MixDisplay *mix_display = NULL;

	LOG_V( "Begin\n");

	if (!mix || !init_params) {
		LOG_E( "!mix || !init_params\n");
		return MIX_RESULT_NULL_PTR;
	}

	if (mode >= MIX_CODEC_MODE_LAST) {
		LOG_E("mode >= MIX_CODEC_MODE_LAST\n");
		return MIX_RESULT_INVALID_PARAM;
	}

#if 0  //we have encoding support
	/* TODO: We need to support encoding in the future */
	if (mode == MIX_CODEC_MODE_ENCODE) {
		LOG_E("mode == MIX_CODEC_MODE_ENCODE\n");
		return MIX_RESULT_NOTIMPL;
	}
#endif

	if (!MIX_IS_VIDEOINITPARAMS(init_params)) {
		LOG_E("!MIX_IS_VIDEOINITPARAMS(init_params\n");
		return MIX_RESULT_INVALID_PARAM;
	}

	priv = MIX_VIDEO_PRIVATE(mix);

	if (priv->initialized) {
		LOG_W( "priv->initialized\n");
		return MIX_RESULT_ALREADY_INIT;
	}

	/* clone mode */
	priv->codec_mode = mode;

	/* ref init_params */
	priv->init_params = (MixVideoInitParams *) mix_params_ref(MIX_PARAMS(
			init_params));
	if (!priv->init_params) {
		ret = MIX_RESULT_NO_MEMORY;
		LOG_E( "!priv->init_params\n");
		goto cleanup;
	}

	/* NOTE: we don't do anything with drm_init_params */

	/* libVA initialization */

	{
		VAStatus va_status;
		Display *display = NULL;
		ret = mix_videoinitparams_get_display(priv->init_params, &mix_display);
		if (ret != MIX_RESULT_SUCCESS) {
			LOG_E("Failed to get display 1\n");
			goto cleanup;
		}
#ifndef ANDROID
		if (MIX_IS_DISPLAYX11(mix_display)) {
			MixDisplayX11 *mix_displayx11 = MIX_DISPLAYX11(mix_display);
			ret = mix_displayx11_get_display(mix_displayx11, &display);
			if (ret != MIX_RESULT_SUCCESS) {
				LOG_E("Failed to get display 2\n");
				goto cleanup;

			}
		} else {
			/* TODO: add support to other MixDisplay type. For now, just return error!*/
			LOG_E("It is not display x11\n");
			ret = MIX_RESULT_FAIL;
			goto cleanup;
		}
#else
               if (MIX_IS_DISPLAYANDROID(mix_display)) {
                        MixDisplayAndroid *mix_displayandroid = MIX_DISPLAYANDROID(mix_display);
                        ret = mix_displayandroid_get_display(mix_displayandroid, (void**)&display);
                        if (ret != MIX_RESULT_SUCCESS) {
                                LOG_E("Failed to get display 2\n");
                                goto cleanup;

                        }
                } else {
                        /* TODO: add support to other MixDisplay type. For now, just return error!*/
                        LOG_E("It is not display android\n");
                        ret = MIX_RESULT_FAIL;
                        goto cleanup;
                }
#endif
		/* Now, we can initialize libVA */

		LOG_V("Try to get vaDisplay : display = %x\n", display);
		priv->va_display = vaGetDisplay(display);

		/* Oops! Fail to get VADisplay */
		if (!priv->va_display) {
			ret = MIX_RESULT_FAIL;
			LOG_E("Fail to get VADisplay\n");
			goto cleanup;
		}

		/* Initialize libVA */
		va_status = vaInitialize(priv->va_display, &priv->va_major_version,
				&priv->va_minor_version);

		/* Oops! Fail to initialize libVA */
		if (va_status != VA_STATUS_SUCCESS) {
			ret = MIX_RESULT_FAIL;
			LOG_E("Fail to initialize libVA\n");
			goto cleanup;
		}

		/* TODO: check the version numbers of libVA */

		priv->initialized = TRUE;
		ret = MIX_RESULT_SUCCESS;
	}

	cleanup:

	if (ret != MIX_RESULT_SUCCESS) {
		mix_video_private_cleanup(priv);
	}

	MIXUNREF(mix_display, mix_display_unref);

	LOG_V( "End\n");

	return ret;
}


MIX_RESULT mix_video_deinitialize_default(MixVideo * mix) {

	MixVideoPrivate *priv = NULL;

	LOG_V( "Begin\n");

	CHECK_INIT(mix, priv);

	mix_video_private_cleanup(priv);

	LOG_V( "End\n");
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_video_configure_decode(MixVideo * mix,
		MixVideoConfigParamsDec * config_params_dec, MixDrmParams * drm_config_params) {

	MIX_RESULT ret = MIX_RESULT_FAIL;
	MixVideoPrivate *priv = NULL;
	MixVideoConfigParamsDec *priv_config_params_dec = NULL;

	gchar *mime_type = NULL;
	guint fps_n, fps_d;
	guint bufpoolsize = 0;

	MixFrameOrderMode frame_order_mode = MIX_FRAMEORDER_MODE_DISPLAYORDER;
	MixDisplayOrderMode display_order_mode = MIX_DISPLAY_ORDER_UNKNOWN;

	LOG_V( "Begin\n");

	CHECK_INIT(mix, priv);

	if (!config_params_dec) {
		LOG_E( "!config_params_dec\n");
		return MIX_RESULT_NULL_PTR;
	}

	if (!MIX_IS_VIDEOCONFIGPARAMSDEC(config_params_dec)) {
		LOG_E("Not a MixVideoConfigParamsDec\n");
		return MIX_RESULT_INVALID_PARAM;
	}

	/* ---------------------- begin lock --------------------- */
	priv->objlock.lock();

	/*
	 * MixVideo has already been configured, it should be
	 * re-configured.
	 *
	 * TODO: Allow MixVideo re-configuration
	 */
	if (priv->configured) {
		ret = MIX_RESULT_SUCCESS;
		LOG_W( "Already configured\n");
		goto cleanup;
	}

	/* Make a copy of config_params */
	priv->config_params = (MixVideoConfigParams *) mix_params_dup(MIX_PARAMS(
			config_params_dec));
	if (!priv->config_params) {
		ret = MIX_RESULT_NO_MEMORY;
		LOG_E("Fail to duplicate config_params\n");
		goto cleanup;
	}

	priv_config_params_dec = (MixVideoConfigParamsDec *)priv->config_params;

	/* Get fps, frame order mode and mime type from config_params */
	ret = mix_videoconfigparamsdec_get_mime_type(priv_config_params_dec, &mime_type);
	if (ret != MIX_RESULT_SUCCESS) {
		LOG_E("Failed to get mime type\n");
		goto cleanup;
	}

	LOG_I( "mime : %s\n", mime_type);

#ifdef MIX_LOG_ENABLE
	if (mix_strcmp(mime_type, "video/x-wmv") == 0) {

		LOG_I( "mime : video/x-wmv\n");
		if (MIX_IS_VIDEOCONFIGPARAMSDEC_VC1(priv_config_params_dec)) {
			LOG_I( "VC1 config_param\n");
		} else {
			LOG_E("Not VC1 config_param\n");
		}
	}
#endif

	ret = mix_videoconfigparamsdec_get_frame_order_mode(priv_config_params_dec,
			&frame_order_mode);
	if (ret != MIX_RESULT_SUCCESS) {
		LOG_E("Failed to frame order mode\n");
		goto cleanup;
	}

	ret = mix_videoconfigparamsdec_get_frame_rate(priv_config_params_dec, &fps_n,
			&fps_d);
	if (ret != MIX_RESULT_SUCCESS) {
		LOG_E("Failed to get frame rate\n");
		goto cleanup;
	}

	if (!fps_n) {
		ret = MIX_RESULT_FAIL;
		LOG_E( "fps_n is 0\n");
		goto cleanup;
	}

	ret = mix_videoconfigparamsdec_get_buffer_pool_size(priv_config_params_dec,
			&bufpoolsize);
	if (ret != MIX_RESULT_SUCCESS) {
		LOG_E("Failed to get buffer pool size\n");
		goto cleanup;
	}

	/* create frame manager */
	priv->frame_manager = mix_framemanager_new();
	if (!priv->frame_manager) {
		ret = MIX_RESULT_NO_MEMORY;
		LOG_E("Failed to create frame manager\n");
		goto cleanup;
	}

	if (frame_order_mode == MIX_FRAMEORDER_MODE_DECODEORDER)
	{
        display_order_mode = MIX_DISPLAY_ORDER_FIFO;
    }
	else if (mix_strcmp(mime_type, "video/x-wmv")  == 0 ||
            mix_strcmp(mime_type, "video/mpeg")   == 0 ||
            mix_strcmp(mime_type, "video/x-divx") == 0 ||
            mix_strcmp(mime_type, "video/x-h263") == 0 ||
            mix_strcmp(mime_type, "video/x-xvid") == 0 )
    {
        display_order_mode = MIX_DISPLAY_ORDER_PICTYPE;
	}
	else
	{
        //display_order_mode = MIX_DISPLAY_ORDER_TIMESTAMP;
        display_order_mode = MIX_DISPLAY_ORDER_PICNUMBER;
	}

	/* initialize frame manager */
    ret = mix_framemanager_initialize(priv->frame_manager,
            display_order_mode, fps_n, fps_d);

	if (ret != MIX_RESULT_SUCCESS) {
		LOG_E("Failed to initialize frame manager\n");
		goto cleanup;
	}

	/* create buffer pool */
	priv->buffer_pool = mix_bufferpool_new();
	if (!priv->buffer_pool) {
		ret = MIX_RESULT_NO_MEMORY;
		LOG_E("Failed to create buffer pool\n");
		goto cleanup;
	}

	ret = mix_bufferpool_initialize(priv->buffer_pool, bufpoolsize);
	if (ret != MIX_RESULT_SUCCESS) {
		LOG_E("Failed to initialize buffer pool\n");
		goto cleanup;
	}

	/* Finally, we can create MixVideoFormat */
	/* What type of MixVideoFormat we need create? */

	if (mix_strcmp(mime_type, "video/x-wmv") == 0
			&& MIX_IS_VIDEOCONFIGPARAMSDEC_VC1(priv_config_params_dec)) {

		MixVideoFormat_VC1 *video_format = mix_videoformat_vc1_new();
		if (!video_format) {
			ret = MIX_RESULT_NO_MEMORY;
			LOG_E("Failed to create VC-1 video format\n");
			goto cleanup;
		}

		/* TODO: work specific to VC-1 */

		priv->video_format = MIX_VIDEOFORMAT(video_format);

	} else if (mix_strcmp(mime_type, "video/x-h264") == 0
			&& MIX_IS_VIDEOCONFIGPARAMSDEC_H264(priv_config_params_dec)) {

		MixVideoFormat_H264 *video_format = mix_videoformat_h264_new();
		if (!video_format) {
			ret = MIX_RESULT_NO_MEMORY;
			LOG_E("Failed to create H.264 video format\n");
			goto cleanup;
		}

		/* TODO: work specific to H.264 */

		priv->video_format = MIX_VIDEOFORMAT(video_format);

	} else if (mix_strcmp(mime_type, "video/mpeg")   == 0 ||
                   mix_strcmp(mime_type, "video/x-divx") == 0 ||
                   mix_strcmp(mime_type, "video/x-h263") == 0 ||
                   mix_strcmp(mime_type, "video/x-xvid") == 0 ||
                   mix_strcmp(mime_type, "video/x-dx50") == 0) {

		guint version = 0;

		/* Is this mpeg4:2 ? */
		if (mix_strcmp(mime_type, "video/mpeg") == 0 ||
                    mix_strcmp(mime_type, "video/x-h263") == 0 ) {

			/*
			 *  we don't support mpeg other than mpeg verion 4
			 */
			if (!MIX_IS_VIDEOCONFIGPARAMSDEC_MP42(priv_config_params_dec)) {
				ret = MIX_RESULT_NOT_SUPPORTED;
				goto cleanup;
			}

			/* what is the mpeg version ? */
			ret = mix_videoconfigparamsdec_mp42_get_mpegversion(
					MIX_VIDEOCONFIGPARAMSDEC_MP42(priv_config_params_dec), &version);
			if (ret != MIX_RESULT_SUCCESS) {
				LOG_E("Failed to get mpeg version\n");
				goto cleanup;
			}

			/* if it is not MPEG4 */
			if (version != 4) {
				ret = MIX_RESULT_NOT_SUPPORTED;
				goto cleanup;
			}

		} else {

			/* config_param shall be MixVideoConfigParamsDecMP42 */
			if (!MIX_IS_VIDEOCONFIGPARAMSDEC_MP42(priv_config_params_dec)) {
                                LOG_E("MIX_IS_VIDEOCONFIGPARAMSDEC_MP42 failed.\n");
				ret = MIX_RESULT_NOT_SUPPORTED;
				goto cleanup;
			}

			/* what is the divx version ? */
			ret = mix_videoconfigparamsdec_mp42_get_divxversion(
					MIX_VIDEOCONFIGPARAMSDEC_MP42(priv_config_params_dec), &version);
			if (ret != MIX_RESULT_SUCCESS) {
				LOG_E("Failed to get divx version\n");
				goto cleanup;
			}

			/* if it is not divx 4 or 5 */
			if (version != 4 && version != 5) {
                                LOG_E("Invalid divx version.\n");
				ret = MIX_RESULT_NOT_SUPPORTED;
				goto cleanup;
			}
		}

		MixVideoFormat_MP42 *video_format = mix_videoformat_mp42_new();
		if (!video_format) {
			ret = MIX_RESULT_NO_MEMORY;
			LOG_E("Failed to create MPEG-4:2 video format\n");
			goto cleanup;
		}

		/* TODO: work specific to MPEG-4:2 */
		priv->video_format = MIX_VIDEOFORMAT(video_format);

	} else {

		/* Oops! A format we don't know */

		ret = MIX_RESULT_FAIL;
		LOG_E("Unknown format, we can't handle it\n");
		goto cleanup;
	}

	/* initialize MixVideoFormat */
	ret = mix_videofmt_initialize(priv->video_format, priv_config_params_dec,
			priv->frame_manager, priv->buffer_pool, &priv->surface_pool,
			priv->va_display);

	if (ret != MIX_RESULT_SUCCESS) {
		LOG_E("Failed initialize video format\n");
		goto cleanup;
	}

	mix_surfacepool_ref(priv->surface_pool);

	/* decide MixVideoFormat from mime_type*/

	priv->configured = TRUE;
	ret = MIX_RESULT_SUCCESS;

	cleanup:

	if (ret != MIX_RESULT_SUCCESS) {
		MIXUNREF(priv->config_params, mix_videoconfigparams_unref);
		MIXUNREF(priv->frame_manager, mix_framemanager_unref);
		MIXUNREF(priv->buffer_pool, mix_bufferpool_unref);
		MIXUNREF(priv->video_format, mix_videoformat_unref);
	}

	if (mime_type) {
		g_free(mime_type);
	}

	priv->objlock.unlock();
	/* ---------------------- end lock --------------------- */

	LOG_V( "End\n");

	return ret;
}
#if MIXVIDEO_ENCODE_ENABLE
MIX_RESULT mix_video_configure_encode(MixVideo * mix,
		MixVideoConfigParamsEnc * config_params_enc,
		MixDrmParams * drm_config_params) {

	MIX_RESULT ret = MIX_RESULT_FAIL;
	MixVideoPrivate *priv = NULL;
	MixVideoConfigParamsEnc *priv_config_params_enc = NULL;


	gchar *mime_type = NULL;
	MixEncodeTargetFormat encode_format = MIX_ENCODE_TARGET_FORMAT_H264;
	guint bufpoolsize = 0;

	LOG_V( "Begin\n");

	CHECK_INIT(mix, priv);

	if (!config_params_enc) {
		LOG_E("!config_params_enc\n");
		return MIX_RESULT_NULL_PTR;
	}
	if (!MIX_IS_VIDEOCONFIGPARAMSENC(config_params_enc)) {
		LOG_E("Not a MixVideoConfigParams\n");
		return MIX_RESULT_INVALID_PARAM;
	}

	/* ---------------------- begin lock --------------------- */
	priv->objlock.lock();

	/*
	 * MixVideo has already been configured, it should be
	 * re-configured.
	 *
	 * TODO: Allow MixVideo re-configuration
	 */
	if (priv->configured) {
		ret = MIX_RESULT_SUCCESS;
		LOG_E( "Already configured\n");
		goto cleanup;
	}

	/* Make a copy of config_params */
	priv->config_params = (MixVideoConfigParams *) mix_params_dup(
			MIX_PARAMS(config_params_enc));
	if (!priv->config_params) {
		ret = MIX_RESULT_NO_MEMORY;
		LOG_E("Fail to duplicate config_params\n");
		goto cleanup;
	}

	priv_config_params_enc = (MixVideoConfigParamsEnc *)priv->config_params;

	/* Get fps, frame order mode and mime type from config_params */
	ret = mix_videoconfigparamsenc_get_mime_type(priv_config_params_enc,
			&mime_type);
	if (ret != MIX_RESULT_SUCCESS) {
		LOG_E("Failed to get mime type\n");
		goto cleanup;
	}

	LOG_I( "mime : %s\n", mime_type);

	ret = mix_videoconfigparamsenc_get_encode_format(priv_config_params_enc,
			&encode_format);
	if (ret != MIX_RESULT_SUCCESS) {
		LOG_E("Failed to get target format\n");
		goto cleanup;
	}

	LOG_I( "encode_format : %d\n",
			encode_format);

	ret = mix_videoconfigparamsenc_get_buffer_pool_size(
			priv_config_params_enc, &bufpoolsize);
	if (ret != MIX_RESULT_SUCCESS) {
		LOG_E("Failed to get buffer pool size\n");
		goto cleanup;
	}

	/* create frame manager */
	priv->frame_manager = mix_framemanager_new();
	if (!priv->frame_manager) {
		ret = MIX_RESULT_NO_MEMORY;
		LOG_E("Failed to create frame manager\n");
		goto cleanup;
	}

	/* initialize frame manager */
	/* frame rate can be any value for encoding. */
	ret = mix_framemanager_initialize(priv->frame_manager, MIX_DISPLAY_ORDER_FIFO,
			1, 1);

	if (ret != MIX_RESULT_SUCCESS) {
		LOG_E("Failed to initialize frame manager\n");
		goto cleanup;
	}

	/* create buffer pool */
	priv->buffer_pool = mix_bufferpool_new();
	if (!priv->buffer_pool) {
		ret = MIX_RESULT_NO_MEMORY;
		LOG_E("Failed to create buffer pool\n");
		goto cleanup;
	}

	ret = mix_bufferpool_initialize(priv->buffer_pool, bufpoolsize);
	if (ret != MIX_RESULT_SUCCESS) {
		LOG_E("Failed to initialize buffer pool\n");
		goto cleanup;
	}

	/* Finally, we can create MixVideoFormatEnc */
	/* What type of MixVideoFormatEnc we need create? */

	if (encode_format == MIX_ENCODE_TARGET_FORMAT_H264
			&& MIX_IS_VIDEOCONFIGPARAMSENC_H264(priv_config_params_enc)) {

		MixVideoFormatEnc_H264 *video_format_enc =
				mix_videoformatenc_h264_new();
		if (!video_format_enc) {
			ret = MIX_RESULT_NO_MEMORY;
			LOG_E("mix_video_configure_encode: Failed to create h264 video enc format\n");
			goto cleanup;
		}

		/* work specific to h264 encode */

		priv->video_format_enc = MIX_VIDEOFORMATENC(video_format_enc);

	}
    else if (encode_format == MIX_ENCODE_TARGET_FORMAT_MPEG4
            && MIX_IS_VIDEOCONFIGPARAMSENC_MPEG4(priv_config_params_enc)) {

        MixVideoFormatEnc_MPEG4 *video_format_enc = mix_videoformatenc_mpeg4_new();
        if (!video_format_enc) {
            ret = MIX_RESULT_NO_MEMORY;
            LOG_E("mix_video_configure_encode: Failed to create mpeg-4:2 video format\n");
            goto cleanup;
        }

		/* work specific to mpeg4 */

		priv->video_format_enc = MIX_VIDEOFORMATENC(video_format_enc);

	}

        else if (encode_format == MIX_ENCODE_TARGET_FORMAT_H263
            && MIX_IS_VIDEOCONFIGPARAMSENC_H263(priv_config_params_enc)) {

        MixVideoFormatEnc_H263 *video_format_enc = mix_videoformatenc_h263_new();
        if (!video_format_enc) {
            ret = MIX_RESULT_NO_MEMORY;
            LOG_E("mix_video_configure_encode: Failed to create h.263 video format\n");
            goto cleanup;
        }

		/* work specific to h.263 */

		priv->video_format_enc = MIX_VIDEOFORMATENC(video_format_enc);

	}

        else if (encode_format == MIX_ENCODE_TARGET_FORMAT_PREVIEW
            && MIX_IS_VIDEOCONFIGPARAMSENC_PREVIEW(priv_config_params_enc)) {

        MixVideoFormatEnc_Preview *video_format_enc = mix_videoformatenc_preview_new();
        if (!video_format_enc) {
            ret = MIX_RESULT_NO_MEMORY;
            LOG_E( "mix_video_configure_encode: Failed to create preview video format\n");
            goto cleanup;
        }

		priv->video_format_enc = MIX_VIDEOFORMATENC(video_format_enc);

	}
	else {

		/*unsupported format */
		ret = MIX_RESULT_NOT_SUPPORTED;
		LOG_E("Unknown format, we can't handle it\n");
		goto cleanup;
	}

	/* initialize MixVideoEncFormat */
	ret = mix_videofmtenc_initialize(priv->video_format_enc,
            priv_config_params_enc, priv->frame_manager, NULL, &priv->surface_pool,
            priv->va_display);

	if (ret != MIX_RESULT_SUCCESS) {
		LOG_E("Failed initialize video format\n");
		goto cleanup;
	}

	mix_surfacepool_ref(priv->surface_pool);

	priv->configured = TRUE;
	ret = MIX_RESULT_SUCCESS;

	cleanup:

	if (ret != MIX_RESULT_SUCCESS) {
		MIXUNREF(priv->frame_manager, mix_framemanager_unref);
		MIXUNREF(priv->config_params, mix_videoconfigparams_unref);
		MIXUNREF(priv->buffer_pool, mix_bufferpool_unref);
		MIXUNREF(priv->video_format_enc, mix_videoformatenc_unref);
	}

	if (mime_type) {
		g_free(mime_type);
	}

	priv->objlock.unlock();
	/* ---------------------- end lock --------------------- */

	LOG_V( "End\n");

	return ret;
}
#endif
MIX_RESULT mix_video_configure_default(MixVideo * mix,
		MixVideoConfigParams * config_params,
		MixDrmParams * drm_config_params) {

	MIX_RESULT ret = MIX_RESULT_FAIL;
	MixVideoPrivate *priv = NULL;

	LOG_V( "Begin\n");

	CHECK_INIT(mix, priv);
	if(!config_params) {
		LOG_E("!config_params\n");
		return MIX_RESULT_NULL_PTR;
	}

	/*Decoder mode or Encoder mode*/
	if (priv->codec_mode == MIX_CODEC_MODE_DECODE && MIX_IS_VIDEOCONFIGPARAMSDEC(config_params)) {
		ret = mix_video_configure_decode(mix, (MixVideoConfigParamsDec*)config_params, NULL);
	} 
#if MIXVIDEO_ENCODE_ENABLE
	else if (priv->codec_mode == MIX_CODEC_MODE_ENCODE && MIX_IS_VIDEOCONFIGPARAMSENC(config_params)) {
		ret = mix_video_configure_encode(mix, (MixVideoConfigParamsEnc*)config_params, NULL);
	}
#endif
	else {
		LOG_E("Codec mode not supported\n");
	}

	LOG_V( "end\n");

	return ret;
}

MIX_RESULT mix_video_get_config_default(MixVideo * mix,
		MixVideoConfigParams ** config_params) {

	MIX_RESULT ret = MIX_RESULT_SUCCESS;
	MixVideoPrivate *priv = NULL;

	CHECK_INIT_CONFIG(mix, priv);

	if (!config_params) {
		LOG_E( "!config_params\n");
		return MIX_RESULT_NULL_PTR;
	}

	/* ---------------------- begin lock --------------------- */
	priv->objlock.lock();

	*config_params = MIX_VIDEOCONFIGPARAMS(mix_params_dup(MIX_PARAMS(priv->config_params)));
	if(!*config_params) {
		ret = MIX_RESULT_NO_MEMORY;
		LOG_E("Failed to duplicate MixVideoConfigParams\n");
		goto cleanup;
	}

	cleanup:

	/* ---------------------- end lock --------------------- */
	priv->objlock.unlock();

	LOG_V( "End\n");

	return ret;

}

MIX_RESULT mix_video_decode_default(MixVideo * mix, MixBuffer * bufin[],
		gint bufincnt, MixVideoDecodeParams * decode_params) {

	MIX_RESULT ret = MIX_RESULT_FAIL;
	MixVideoPrivate *priv = NULL;

	LOG_V( "Begin\n");

	CHECK_INIT_CONFIG(mix, priv);
	if(!bufin || !bufincnt || !decode_params) {
		LOG_E( "!bufin || !bufincnt || !decode_params\n");
		return MIX_RESULT_NULL_PTR;
	}

    // reset new sequence flag
    decode_params->new_sequence = FALSE;

	//First check that we have surfaces available for decode
	ret = mix_surfacepool_check_available(priv->surface_pool);

	if (ret == MIX_RESULT_POOLEMPTY) {
		LOG_I( "Out of surface\n");
		return MIX_RESULT_OUTOFSURFACES;
	}

	priv->objlock.lock();

	ret = mix_videofmt_decode(priv->video_format, bufin, bufincnt, decode_params);

	priv->objlock.unlock();

	LOG_V( "End\n");

	return ret;
}

MIX_RESULT mix_video_get_frame_default(MixVideo * mix, MixVideoFrame ** frame) {

	LOG_V( "Begin\n");

	MIX_RESULT ret = MIX_RESULT_FAIL;
	MixVideoPrivate *priv = NULL;

	CHECK_INIT_CONFIG(mix, priv);

	if (!frame) {
		LOG_E( "!frame\n");
		return MIX_RESULT_NULL_PTR;
	}

	/* ---------------------- begin lock --------------------- */
	priv->objlock.lock();

	LOG_V("Calling frame manager dequeue\n");

	ret = mix_framemanager_dequeue(priv->frame_manager, frame);

	/* ---------------------- end lock --------------------- */
	priv->objlock.unlock();

	LOG_V( "End\n");

	return ret;
}

MIX_RESULT mix_video_release_frame_default(MixVideo * mix,
		MixVideoFrame * frame) {

	LOG_V( "Begin\n");

	MIX_RESULT ret = MIX_RESULT_FAIL;
	MixVideoPrivate *priv = NULL;

	CHECK_INIT_CONFIG(mix, priv);

	if (!frame) {
		LOG_E( "!frame\n");
		return MIX_RESULT_NULL_PTR;
	}

	/*
	 * We don't need lock here. MixVideoFrame has lock to
	 * protect itself.
	 */
#if 0
	/* ---------------------- begin lock --------------------- */
	priv->objlock.lock();
#endif

	LOG_I("Releasing reference frame %x\n", (guint) frame);
	mix_videoframe_unref(frame);

	ret = MIX_RESULT_SUCCESS;

#if 0
	/* ---------------------- end lock --------------------- */
	priv->objlock.unlock();
#endif

	LOG_V( "End\n");

	return ret;

}

#ifdef ANDROID

MIX_RESULT mix_video_render_default(MixVideo * mix,
                MixVideoRenderParams * render_params, MixVideoFrame *frame) {

	return MIX_RESULT_NOTIMPL;
}

#else
MIX_RESULT mix_video_render_default(MixVideo * mix,
		MixVideoRenderParams * render_params, MixVideoFrame *frame) {

	LOG_V( "Begin\n");

	MIX_RESULT ret = MIX_RESULT_FAIL;
	MixVideoPrivate *priv = NULL;

	MixDisplay *mix_display = NULL;
	MixDisplayX11 *mix_display_x11 = NULL;

	Display *display = NULL;

	Drawable drawable = 0;
	MixRect src_rect, dst_rect;

	VARectangle *va_cliprects = NULL;
	guint number_of_cliprects = 0;

	/* VASurfaceID va_surface_id; */
	gulong va_surface_id;
	VAStatus va_status;

	gboolean sync_flag = FALSE;

	CHECK_INIT_CONFIG(mix, priv);

	if (!render_params || !frame) {
		LOG_E( "!render_params || !frame\n");
		return MIX_RESULT_NULL_PTR;
	}

	/* Is this render param valid? */
	if (!MIX_IS_VIDEORENDERPARAMS(render_params)) {
		LOG_E("Not MixVideoRenderParams\n");
		return MIX_RESULT_INVALID_PARAM;
	}

	/*
	 * We don't need lock here. priv->va_display may be the only variable
	 * seems need to be protected. But, priv->va_display is initialized
	 * when mixvideo object is initialized, and it keeps
	 * the same value thoughout the life of mixvideo.
	 */
#if 0
	/* ---------------------- begin lock --------------------- */
	priv->objlock.lock();
#endif

	/* get MixDisplay prop from render param */
	ret = mix_videorenderparams_get_display(render_params, &mix_display);
	if (ret != MIX_RESULT_SUCCESS) {
		LOG_E("Failed to get mix_display\n");
		goto cleanup;
	}

	/* Is this MixDisplayX11 ? */
	/* TODO: we shall also support MixDisplay other than MixDisplayX11 */
	if (!MIX_IS_DISPLAYX11(mix_display)) {
		ret = MIX_RESULT_INVALID_PARAM;
		LOG_E( "Not MixDisplayX11\n");
		goto cleanup;
	}

	/* cast MixDisplay to MixDisplayX11 */
	mix_display_x11 = MIX_DISPLAYX11(mix_display);

	/* Get Drawable */
	ret = mix_displayx11_get_drawable(mix_display_x11, &drawable);
	if (ret != MIX_RESULT_SUCCESS) {
		LOG_E( "Failed to get drawable\n");
		goto cleanup;
	}

	/* Get Display */
	ret = mix_displayx11_get_display(mix_display_x11, &display);
	if (ret != MIX_RESULT_SUCCESS) {
		LOG_E( "Failed to get display\n");
		goto cleanup;
	}

	/* get src_rect */
	ret = mix_videorenderparams_get_src_rect(render_params, &src_rect);
	if (ret != MIX_RESULT_SUCCESS) {
		LOG_E("Failed to get SOURCE src_rect\n");
		goto cleanup;
	}

	/* get dst_rect */
	ret = mix_videorenderparams_get_dest_rect(render_params, &dst_rect);
	if (ret != MIX_RESULT_SUCCESS) {
		LOG_E( "Failed to get dst_rect\n");
		goto cleanup;
	}

	/* get va_cliprects */
	ret = mix_videorenderparams_get_cliprects_internal(render_params,
			&va_cliprects, &number_of_cliprects);
	if (ret != MIX_RESULT_SUCCESS) {
		LOG_E("Failed to get va_cliprects\n");
		goto cleanup;
	}

	/* get surface id from frame */
	ret = mix_videoframe_get_frame_id(frame, &va_surface_id);
	if (ret != MIX_RESULT_SUCCESS) {
		LOG_E("Failed to get va_surface_id\n");
		goto cleanup;
	}
	guint64 timestamp = 0;
	mix_videoframe_get_timestamp(frame, &timestamp);
	LOG_V( "Displaying surface ID %d, timestamp %"G_GINT64_FORMAT"\n", (int)va_surface_id, timestamp);

	guint32 frame_structure = 0;
	mix_videoframe_get_frame_structure(frame, &frame_structure);

	ret = mix_videoframe_get_sync_flag(frame, &sync_flag);
	if (ret != MIX_RESULT_SUCCESS) {
		LOG_E("Failed to get sync_flag\n");
		goto cleanup;
	}

	if (!sync_flag) {
		ret = mix_videoframe_set_sync_flag(frame, TRUE);
		if (ret != MIX_RESULT_SUCCESS) {
			LOG_E("Failed to set sync_flag\n");
			goto cleanup;
		}

		va_status = vaSyncSurface(priv->va_display, va_surface_id);
		if (va_status != VA_STATUS_SUCCESS) {
			ret = MIX_RESULT_FAIL;
			LOG_E("Failed vaSyncSurface() : va_status = 0x%x\n", va_status);
			goto cleanup;
		}
	}


	/* TODO: the last param of vaPutSurface is de-interlacing flags,
	 what is value shall be*/
	va_status = vaPutSurface(priv->va_display, (VASurfaceID) va_surface_id,
			drawable, src_rect.x, src_rect.y, src_rect.width, src_rect.height,
			dst_rect.x, dst_rect.y, dst_rect.width, dst_rect.height,
			va_cliprects, number_of_cliprects, frame_structure);

	if (va_status != VA_STATUS_SUCCESS) {
		ret = MIX_RESULT_FAIL;
		LOG_E("Failed vaPutSurface() : va_status = 0x%x\n", va_status);
		goto cleanup;
	}

	ret = MIX_RESULT_SUCCESS;

	cleanup:

	MIXUNREF(mix_display, mix_display_unref)
	/*	MIXUNREF(render_params, mix_videorenderparams_unref)*/

#if 0
	/* ---------------------- end lock --------------------- */
	priv->objlock.unlock();
#endif

	LOG_V( "End\n");

	return ret;

}
#endif /* ANDROID */

#if MIXVIDEO_ENCODE_ENABLE
MIX_RESULT mix_video_encode_default(MixVideo * mix, MixBuffer * bufin[],
		gint bufincnt, MixIOVec * iovout[], gint iovoutcnt,
		MixVideoEncodeParams * encode_params) {

	MIX_RESULT ret = MIX_RESULT_FAIL;
	MixVideoPrivate *priv = NULL;

	LOG_V( "Begin\n");

	CHECK_INIT_CONFIG(mix, priv);
	if(!bufin || !bufincnt) { //we won't check encode_params here, it's just a placeholder
		LOG_E( "!bufin || !bufincnt\n");
		return MIX_RESULT_NULL_PTR;
	}

	//First check that we have surfaces available for decode
	ret = mix_surfacepool_check_available(priv->surface_pool);

	if (ret == MIX_RESULT_POOLEMPTY) {
		LOG_I( "Out of surface\n");
		return MIX_RESULT_OUTOFSURFACES;
	}


	priv->objlock.lock();

	ret = mix_videofmtenc_encode(priv->video_format_enc, bufin, bufincnt,
			iovout, iovoutcnt, encode_params);

	priv->objlock.unlock();

	LOG_V( "End\n");
	return ret;
}
#endif
MIX_RESULT mix_video_flush_default(MixVideo * mix) {

	MIX_RESULT ret = MIX_RESULT_FAIL;
	MixVideoPrivate *priv = NULL;

	LOG_V( "Begin\n");

	CHECK_INIT_CONFIG(mix, priv);

	/* ---------------------- begin lock --------------------- */
	priv->objlock.lock();

	if (priv->codec_mode == MIX_CODEC_MODE_DECODE && priv->video_format != NULL) {
		ret = mix_videofmt_flush(priv->video_format);

		ret = mix_framemanager_flush(priv->frame_manager);
	} 
#if MIXVIDEO_ENCODE_ENABLE
	else if (priv->codec_mode == MIX_CODEC_MODE_ENCODE
			&& priv->video_format_enc != NULL) {
		/*No framemanager for encoder now*/
		ret = mix_videofmtenc_flush(priv->video_format_enc);
	} 
#endif
	else {
		priv->objlock.unlock();
		LOG_E("Invalid video_format/video_format_enc Pointer\n");
		return MIX_RESULT_NULL_PTR;
	}

	/* ---------------------- end lock --------------------- */
	priv->objlock.unlock();

	LOG_V( "End\n");

	return ret;

}

MIX_RESULT mix_video_eos_default(MixVideo * mix) {

	MIX_RESULT ret = MIX_RESULT_FAIL;
	MixVideoPrivate *priv = NULL;

	LOG_V( "Begin\n");

	CHECK_INIT_CONFIG(mix, priv);

	/* ---------------------- begin lock --------------------- */
	priv->objlock.lock();

	if (priv->codec_mode == MIX_CODEC_MODE_DECODE && priv->video_format != NULL) {
		ret = mix_videofmt_eos(priv->video_format);

		/* We should not call mix_framemanager_eos() here.
		 * MixVideoFormat* is responsible to call this function.
		 * Commnet the function call here!
		 */
		/* frame manager will set EOS flag to be TRUE */
		/* ret = mix_framemanager_eos(priv->frame_manager); */
	}
#if MIXVIDEO_ENCODE_ENABLE
	else if (priv->codec_mode == MIX_CODEC_MODE_ENCODE
			&& priv->video_format_enc != NULL) {
		/*No framemanager now*/
		ret = mix_videofmtenc_eos(priv->video_format_enc);
	}
#endif
	else {
		priv->objlock.unlock();
		LOG_E("Invalid video_format/video_format_enc Pointer\n");
		return MIX_RESULT_NULL_PTR;
	}

	/* ---------------------- end lock --------------------- */
	priv->objlock.unlock();

	LOG_V( "End\n");

	return ret;
}

MIX_RESULT mix_video_get_state_default(MixVideo * mix, MixState * state) {

	MixVideoPrivate *priv = NULL;

	LOG_V( "Begin\n");

	CHECK_INIT_CONFIG(mix, priv);

	if (!state) {
		LOG_E( "!state\n");
		return MIX_RESULT_NULL_PTR;
	}

	*state = MIX_STATE_CONFIGURED;

	LOG_V( "End\n");

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_video_get_mixbuffer_default(MixVideo * mix, MixBuffer ** buf) {

	MIX_RESULT ret = MIX_RESULT_FAIL;
	MixVideoPrivate *priv = NULL;

	LOG_V( "Begin\n");

	CHECK_INIT_CONFIG(mix, priv);

	if (!buf) {
		LOG_E( "!buf\n");
		return MIX_RESULT_INVALID_PARAM;
	}

	/* ---------------------- begin lock --------------------- */
	priv->objlock.lock();

	ret = mix_bufferpool_get(priv->buffer_pool, buf);

	/* ---------------------- end lock --------------------- */
	priv->objlock.unlock();

	LOG_V( "End ret = 0x%x\n", ret);

	return ret;

}

MIX_RESULT mix_video_release_mixbuffer_default(MixVideo * mix, MixBuffer * buf) {

	MIX_RESULT ret = MIX_RESULT_FAIL;
	MixVideoPrivate *priv = NULL;

	LOG_V( "Begin\n");

	CHECK_INIT_CONFIG(mix, priv);

	if (!buf) {
		LOG_E( "!buf\n");
		return MIX_RESULT_INVALID_PARAM;
	}

	/* ---------------------- begin lock --------------------- */
	priv->objlock.lock();

	mix_buffer_unref(buf);

	/* ---------------------- end lock --------------------- */
	priv->objlock.unlock();

	LOG_V( "End\n");
	return ret;

}

#if MIXVIDEO_ENCODE_ENABLE
MIX_RESULT mix_video_get_max_coded_buffer_size_default (MixVideo * mix, guint *max_size)
{
      MIX_RESULT ret = MIX_RESULT_FAIL;
	MixVideoPrivate *priv = NULL;

	LOG_V( "Begin\n");

	if (!mix || !max_size) /* TODO: add other parameter NULL checking */
	{
		LOG_E( "!mix || !bufsize\n");
		return MIX_RESULT_NULL_PTR;
	}

	CHECK_INIT_CONFIG(mix, priv);

	priv->objlock.lock();

	ret = mix_videofmtenc_get_max_coded_buffer_size(priv->video_format_enc, max_size);

	priv->objlock.unlock();

	LOG_V( "End\n");
	return ret;
}


MIX_RESULT mix_video_set_dynamic_enc_config_default (MixVideo * mix,
	MixEncParamsType params_type, MixEncDynamicParams * dynamic_params)
{
	MIX_RESULT ret = MIX_RESULT_FAIL;
	MixVideoPrivate *priv = NULL;

	LOG_V( "Begin\n");

	CHECK_INIT_CONFIG(mix, priv);

	if (dynamic_params == NULL) {
		LOG_E(
			"dynamic_params == NULL\n");
		return MIX_RESULT_FAIL;
	}

	MixVideoConfigParamsEnc *priv_config_params_enc = NULL;
	if (priv->config_params) {
		/*
		 * FIXME: It would be better to use ref/unref
		 */
		priv_config_params_enc = (MixVideoConfigParamsEnc *)priv->config_params;
		//priv_config_params_enc = mix_videoconfigparamsenc_ref (priv->config_params);
	}
	else {
		LOG_E(
			"priv->config_params is invalid\n");
		return MIX_RESULT_FAIL;
	}

	priv->objlock.lock();

	switch (params_type) {
		case MIX_ENC_PARAMS_BITRATE:
		{
			ret = mix_videoconfigparamsenc_set_bit_rate (priv_config_params_enc, dynamic_params->bitrate);
			if (ret != MIX_RESULT_SUCCESS) {
				LOG_E("Failed mix_videoconfigparamsenc_set_bit_rate\n");
				goto cleanup;
			}
		}
			break;

		case MIX_ENC_PARAMS_INIT_QP:
		{
			ret = mix_videoconfigparamsenc_set_init_qp (priv_config_params_enc, dynamic_params->init_QP);
			if (ret != MIX_RESULT_SUCCESS) {
				LOG_E("Failed mix_videoconfigparamsenc_set_init_qp\n");
				goto cleanup;
			}
		}
			break;

		case MIX_ENC_PARAMS_MIN_QP:
		{
			ret = mix_videoconfigparamsenc_set_min_qp (priv_config_params_enc, dynamic_params->min_QP);
			if (ret != MIX_RESULT_SUCCESS) {
				LOG_E("Failed mix_videoconfigparamsenc_set_min_qp\n");
				goto cleanup;
			}
		}
			break;

		case MIX_ENC_PARAMS_WINDOW_SIZE:
		{
			ret = mix_videoconfigparamsenc_set_window_size (priv_config_params_enc, dynamic_params->window_size);
			if (ret != MIX_RESULT_SUCCESS) {
				LOG_E("Failed mix_videoconfigparamsenc_set_window_size\n");
				goto cleanup;
			}
		}
			break;

		case MIX_ENC_PARAMS_TARGET_PERCENTAGE:
		{
			ret = mix_videoconfigparamsenc_set_target_percentage (priv_config_params_enc, dynamic_params->target_percentage);
			if (ret != MIX_RESULT_SUCCESS) {
				LOG_E("Failed mix_videoconfigparamsenc_set_target_percentage\n");
				goto cleanup;
			}
		}
			break;


		case MIX_ENC_PARAMS_MTU_SLICE_SIZE:
		{
			ret = mix_videoconfigparamsenc_set_max_slice_size(priv_config_params_enc, dynamic_params->max_slice_size);
			if (ret != MIX_RESULT_SUCCESS) {
				LOG_E("Failed mix_videoconfigparamsenc_set_max_slice_size\n");
				goto cleanup;
			}
		}
			break;

		case MIX_ENC_PARAMS_SLICE_NUM:
		{
			/*
			*/
			MixVideoConfigParamsEncH264 * config_params_enc_h264 =
				MIX_VIDEOCONFIGPARAMSENC_H264 (priv->config_params);

			ret = mix_videoconfigparamsenc_h264_set_slice_num (config_params_enc_h264, dynamic_params->slice_num);
			if (ret != MIX_RESULT_SUCCESS) {
				LOG_E("Failed mix_videoconfigparamsenc_h264_set_slice_num\n");
				goto cleanup;
			}
		}
			break;

		case MIX_ENC_PARAMS_I_SLICE_NUM:
		{
			/*
			*/
			MixVideoConfigParamsEncH264 * config_params_enc_h264 =
				MIX_VIDEOCONFIGPARAMSENC_H264 (priv->config_params);

			ret = mix_videoconfigparamsenc_h264_set_I_slice_num (config_params_enc_h264, dynamic_params->I_slice_num);
			if (ret != MIX_RESULT_SUCCESS) {
				LOG_E("Failed mix_videoconfigparamsenc_h264_set_I_slice_num\n");
				goto cleanup;
			}
		}
			break;

		case MIX_ENC_PARAMS_P_SLICE_NUM:
		{
			/*
			*/
			MixVideoConfigParamsEncH264 * config_params_enc_h264 =
				MIX_VIDEOCONFIGPARAMSENC_H264 (priv->config_params);

			ret = mix_videoconfigparamsenc_h264_set_P_slice_num (config_params_enc_h264, dynamic_params->P_slice_num);
			if (ret != MIX_RESULT_SUCCESS) {
				LOG_E("Failed mix_videoconfigparamsenc_h264_set_P_slice_num\n");
				goto cleanup;
			}
		}
			break;

		case MIX_ENC_PARAMS_IDR_INTERVAL:
		{
			MixVideoConfigParamsEncH264 * config_params_enc_h264 =
				MIX_VIDEOCONFIGPARAMSENC_H264 (priv->config_params);

			ret = mix_videoconfigparamsenc_h264_set_IDR_interval(config_params_enc_h264, dynamic_params->idr_interval);
			if (ret != MIX_RESULT_SUCCESS) {
				LOG_E("Failed mix_videoconfigparamsenc_h264_set_IDR_interval\n");
				goto cleanup;
			}
		}
			break;

		case MIX_ENC_PARAMS_RC_MODE:
		case MIX_ENC_PARAMS_RESOLUTION:
		{
			/*
			 * Step 1: Release videofmtenc Object
			 */
			if (priv->video_format_enc) {
				mix_videofmtenc_deinitialize(priv->video_format_enc);
			}

			MIXUNREF(priv->video_format_enc, mix_videoformatenc_unref)

			//priv->alloc_surface_cnt = 0; //Surfaces are also released, we need to set alloc_surface_cnt to 0

			/*
			* Please note there maybe issue here for usrptr shared buffer mode
			*/

			/*
			 * Step 2: Change configuration parameters (frame size)
			 */

			if (params_type == MIX_ENC_PARAMS_RESOLUTION) {
				ret = mix_videoconfigparamsenc_set_picture_res (priv_config_params_enc, dynamic_params->width, dynamic_params->height);
				if (ret != MIX_RESULT_SUCCESS) {
					LOG_E("Failed mix_videoconfigparamsenc_set_picture_res\n");
					goto cleanup;
				}
			}
			else if (params_type == MIX_ENC_PARAMS_RC_MODE) {
				ret = mix_videoconfigparamsenc_set_rate_control(priv_config_params_enc, dynamic_params->rc_mode);
				if (ret != MIX_RESULT_SUCCESS) {
					LOG_E("Failed mix_videoconfigparamsenc_set_rate_control\n");
					goto cleanup;
				}
			}


			/*
			 * Step 3: Renew mixvideofmtenc object
			 */

			MixEncodeTargetFormat encode_format = MIX_ENCODE_TARGET_FORMAT_H264;

			ret = mix_videoconfigparamsenc_get_encode_format(priv_config_params_enc,
				&encode_format);
			if (ret != MIX_RESULT_SUCCESS) {
				LOG_E("Failed to get target format\n");
				goto cleanup;
			}

			if (encode_format == MIX_ENCODE_TARGET_FORMAT_H264
				&& MIX_IS_VIDEOCONFIGPARAMSENC_H264(priv_config_params_enc)) {

				MixVideoFormatEnc_H264 *video_format_enc =
					mix_videoformatenc_h264_new();

				if (!video_format_enc) {
					ret = MIX_RESULT_NO_MEMORY;
					LOG_E("mix_video_configure_encode: Failed to create h264 video enc format\n");
					goto cleanup;
				}

				/* work specific to h264 encode */

				priv->video_format_enc = MIX_VIDEOFORMATENC(video_format_enc);

			}
			else if (encode_format == MIX_ENCODE_TARGET_FORMAT_MPEG4
				&& MIX_IS_VIDEOCONFIGPARAMSENC_MPEG4(priv_config_params_enc)) {

				MixVideoFormatEnc_MPEG4 *video_format_enc = mix_videoformatenc_mpeg4_new();
				if (!video_format_enc) {
					ret = MIX_RESULT_NO_MEMORY;
					LOG_E("mix_video_configure_encode: Failed to create mpeg-4:2 video format\n");
					goto cleanup;
				}

				/* work specific to mpeg4 */

				priv->video_format_enc = MIX_VIDEOFORMATENC(video_format_enc);

			}

        		else if (encode_format == MIX_ENCODE_TARGET_FORMAT_H263
				&& MIX_IS_VIDEOCONFIGPARAMSENC_H263(priv_config_params_enc)) {

				MixVideoFormatEnc_H263 *video_format_enc = mix_videoformatenc_h263_new();
				if (!video_format_enc) {
					ret = MIX_RESULT_NO_MEMORY;
					LOG_E("mix_video_configure_encode: Failed to create h.263 video format\n");
					goto cleanup;
				}

				/* work specific to h.263 */

				priv->video_format_enc = MIX_VIDEOFORMATENC(video_format_enc);

			}

			else if (encode_format == MIX_ENCODE_TARGET_FORMAT_PREVIEW
				&& MIX_IS_VIDEOCONFIGPARAMSENC_PREVIEW(priv_config_params_enc)) {

        			MixVideoFormatEnc_Preview *video_format_enc = mix_videoformatenc_preview_new();
        			if (!video_format_enc) {
			            	ret = MIX_RESULT_NO_MEMORY;
			            	LOG_E( "mix_video_configure_encode: Failed to create preview video format\n");
					goto cleanup;
		       	 }

				priv->video_format_enc = MIX_VIDEOFORMATENC(video_format_enc);

			}
			else {

				/*unsupported format */
				ret = MIX_RESULT_NOT_SUPPORTED;
				LOG_E("Unknown format, we can't handle it\n");
				goto cleanup;
			}


			/*
			 * Step 4: Re-initialize and start a new encode session, of course with new resolution value
			 */

			/*
			  * Initialize MixVideoEncFormat
			  */

			/*
			* If we are using usrptr shared buffer mode, alloc_surfaces/usrptr/alloc_surface_cnt
			* will be re-requested by v4l2camsrc, how to differetiate old surface pools and new one
			* is a problem.
			*/

			/*
			* priv->alloc_surface_cnt already been reset to 0 after calling mix_videofmtenc_initialize
			* For dynamic frame size change, upstream element need to re-call buffer allocation method
			* and priv->alloc_surface_cnt will get a new value.
			*/
			//priv->alloc_surface_cnt = 5;
			ret = mix_videofmtenc_initialize(priv->video_format_enc,
            			priv_config_params_enc, priv->frame_manager, NULL, &priv->surface_pool,
            			priv->va_display/*, priv->alloc_surfaces, priv->usrptr, priv->alloc_surface_cnt*/);

			if (ret != MIX_RESULT_SUCCESS) {
				LOG_E("Failed initialize video format\n");
				goto cleanup;
			}

			mix_surfacepool_ref(priv->surface_pool);


		}
			break;
		case MIX_ENC_PARAMS_GOP_SIZE:
		{
			ret = mix_videoconfigparamsenc_set_intra_period (priv_config_params_enc, dynamic_params->intra_period);
			if (ret != MIX_RESULT_SUCCESS) {
				LOG_E("Failed mix_videoconfigparamsenc_set_intra_period\n");
				goto cleanup;
			}

		}
			break;
		case MIX_ENC_PARAMS_FRAME_RATE:
		{
			ret = mix_videoconfigparamsenc_set_frame_rate (priv_config_params_enc, dynamic_params->frame_rate_num, dynamic_params->frame_rate_denom);
			if (ret != MIX_RESULT_SUCCESS) {
				LOG_E("Failed mix_videoconfigparamsenc_set_frame_rate\n");
				goto cleanup;
			}
		}
			break;
		case MIX_ENC_PARAMS_FORCE_KEY_FRAME:
		{
			/*
			 * nothing to be done now.
			 */
		}
			break;

		case MIX_ENC_PARAMS_REFRESH_TYPE:
		{
			ret = mix_videoconfigparamsenc_set_refresh_type(priv_config_params_enc, dynamic_params->refresh_type);
			if (ret != MIX_RESULT_SUCCESS) {
				LOG_E("Failed mix_videoconfigparamsenc_set_refresh_type\n");
				goto cleanup;
			}
		}
			break;

		case MIX_ENC_PARAMS_AIR:
		{
			ret = mix_videoconfigparamsenc_set_AIR_params(priv_config_params_enc, dynamic_params->air_params);
			if (ret != MIX_RESULT_SUCCESS) {
				LOG_E("Failed mix_videoconfigparamsenc_set_AIR_params\n");
				goto cleanup;
			}
		}
			break;

		case MIX_ENC_PARAMS_CIR_FRAME_CNT:
		{
			ret = mix_videoconfigparamsenc_set_CIR_frame_cnt (priv_config_params_enc, dynamic_params->CIR_frame_cnt);
			if (ret != MIX_RESULT_SUCCESS) {
				LOG_E("Failed mix_videoconfigparamsenc_set_CIR_frame_cnt\n");
				goto cleanup;
			}

		}
			break;

		default:
			break;
	}

	ret = mix_videofmtenc_set_dynamic_enc_config (priv->video_format_enc, priv_config_params_enc, params_type);

cleanup:

	priv->objlock.unlock();

	LOG_V( "End ret = 0x%x\n", ret);

	return ret;
}
#endif
/*
 * API functions
 */

#define CHECK_AND_GET_MIX_CLASS(mix, klass) \
	if (!mix) { \
		return MIX_RESULT_NULL_PTR; \
	} \
	if (!MIX_IS_VIDEO(mix)) { \
		LOG_E( "Not MixVideo\n"); \
		return MIX_RESULT_INVALID_PARAM; \
	} 


MIX_RESULT mix_video_get_version(MixVideo * mix, guint * major, guint * minor) {

	return mix->get_version_func(mix, major, minor);

}

MIX_RESULT mix_video_initialize(MixVideo * mix, MixCodecMode mode,
		MixVideoInitParams * init_params, MixDrmParams * drm_init_params) {

	return mix->initialize_func(mix, mode, init_params, drm_init_params);
}

MIX_RESULT mix_video_deinitialize(MixVideo * mix) {

	return mix->deinitialize_func(mix);
}

MIX_RESULT mix_video_configure(MixVideo * mix,
		MixVideoConfigParams * config_params,
		MixDrmParams * drm_config_params) {

	return mix->configure_func(mix, config_params, drm_config_params);
}

MIX_RESULT mix_video_get_config(MixVideo * mix,
		MixVideoConfigParams ** config_params_dec) {

	return mix->get_config_func(mix, config_params_dec);

}

MIX_RESULT mix_video_decode(MixVideo * mix, MixBuffer * bufin[], gint bufincnt,
		MixVideoDecodeParams * decode_params) {

	return mix->decode_func(mix, bufin, bufincnt,
				decode_params);

}

MIX_RESULT mix_video_get_frame(MixVideo * mix, MixVideoFrame ** frame) {

	return mix->get_frame_func(mix, frame);

}

MIX_RESULT mix_video_release_frame(MixVideo * mix, MixVideoFrame * frame) {

	return mix->release_frame_func(mix, frame);
}

MIX_RESULT mix_video_render(MixVideo * mix,
		MixVideoRenderParams * render_params, MixVideoFrame *frame) {

	return mix->render_func(mix, render_params, frame);
}

#if MIXVIDEO_ENCODE_ENABLE
MIX_RESULT mix_video_encode(MixVideo * mix, MixBuffer * bufin[], gint bufincnt,
		MixIOVec * iovout[], gint iovoutcnt,
		MixVideoEncodeParams * encode_params) {

	return mix->encode_func(mix, bufin, bufincnt, iovout, iovoutcnt,
				encode_params);
}
#else
MIX_RESULT mix_video_encode(MixVideo * mix, MixBuffer * bufin[], gint bufincnt,
		MixIOVec * iovout[], gint iovoutcnt,
		MixParams * encode_params) {
	return MIX_RESULT_NOT_SUPPORTED;
}
#endif

MIX_RESULT mix_video_flush(MixVideo * mix) {

	return mix->flush_func(mix);
}

MIX_RESULT mix_video_eos(MixVideo * mix) {

	return mix->eos_func(mix);
}

MIX_RESULT mix_video_get_state(MixVideo * mix, MixState * state) {
	return mix->get_state_func(mix, state);
}

MIX_RESULT mix_video_get_mixbuffer(MixVideo * mix, MixBuffer ** buf) {

	return mix->get_mix_buffer_func(mix, buf);
}

MIX_RESULT mix_video_release_mixbuffer(MixVideo * mix, MixBuffer * buf) {

	return mix->release_mix_buffer_func(mix, buf);
}

MIX_RESULT mix_video_get_max_coded_buffer_size(MixVideo * mix, guint *bufsize) {
#if MIXVIDEO_ENCODE_ENABLE
	return mix->get_max_coded_buffer_size_func(mix, bufsize);
#else
	return MIX_RESULT_NOT_SUPPORTED;
#endif
}

MIX_RESULT mix_video_set_dynamic_enc_config (MixVideo * mix,
	MixEncParamsType params_type, MixEncDynamicParams * dynamic_params) {
#if MIXVIDEO_ENCODE_ENABLE
	return mix->set_dynamic_enc_config_func(mix, params_type, dynamic_params);
#else
	return MIX_RESULT_NOT_SUPPORTED;
#endif
}