/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __MIX_VIDEOFORMAT_H__
#define __MIX_VIDEOFORMAT_H__

#include <va/va.h>
#include <glib-object.h>

extern "C" {
#include "vbp_loader.h"
};

#include "mixvideodef.h"
#include <mixdrmparams.h>
#include "mixvideoconfigparamsdec.h"
#include "mixvideodecodeparams.h"
#include "mixvideoframe.h"
#include "mixframemanager.h"
#include "mixsurfacepool.h"
#include "mixbuffer.h"
#include "mixbufferpool.h"
#include "mixvideoformatqueue.h"
#include "mixvideothread.h"

// Redefine the Handle defined in vbp_loader.h
#define VBPhandle Handle

class MixVideoFormat;

#define MIX_VIDEOFORMAT(obj) (dynamic_cast<MixVideoFormat*>(obj))
/* vmethods typedef */

typedef MIX_RESULT (*MixVideoFmtGetCapsFunc)(MixVideoFormat *mix, GString *msg);
typedef MIX_RESULT (*MixVideoFmtInitializeFunc)(MixVideoFormat *mix,
		MixVideoConfigParamsDec * config_params,
                MixFrameManager * frame_mgr,
		MixBufferPool * input_buf_pool,
		MixSurfacePool ** surface_pool,
		VADisplay va_display);
typedef MIX_RESULT (*MixVideoFmtDecodeFunc)(MixVideoFormat *mix, 
		MixBuffer * bufin[], gint bufincnt, 
		MixVideoDecodeParams * decode_params);
typedef MIX_RESULT (*MixVideoFmtFlushFunc)(MixVideoFormat *mix);
typedef MIX_RESULT (*MixVideoFmtEndOfStreamFunc)(MixVideoFormat *mix);
typedef MIX_RESULT (*MixVideoFmtDeinitializeFunc)(MixVideoFormat *mix);

class MixVideoFormat {
	/*< public > */
public:
	MixVideoFormat();
	virtual ~MixVideoFormat();

	
	virtual MIX_RESULT GetCaps(GString *msg);
	virtual MIX_RESULT Initialize(
		MixVideoConfigParamsDec * config_params,
		MixFrameManager * frame_mgr,
		MixBufferPool * input_buf_pool,
		MixSurfacePool ** surface_pool,
		VADisplay va_display);
	virtual MIX_RESULT Decode(
		MixBuffer * bufin[], gint bufincnt, 
		MixVideoDecodeParams * decode_params);
	virtual MIX_RESULT Flush();
	virtual MIX_RESULT EndOfStream();
	virtual MIX_RESULT Deinitialize();


	void Lock() {
		mLock.lock();
	}

	void Unlock() {
		mLock.unlock();
	}

	MixVideoFormat* Ref() {
		++ref_count;
		return this;
	}
	MixVideoFormat* Unref() {
		if (0 == (--ref_count)) {
			delete this;
			return NULL;
		} else {
			return this;
		}
	}

public:
	/*< private > */
	MixVideoMutex mLock;
	gboolean initialized;
	MixFrameManager *framemgr;
	MixSurfacePool *surfacepool;
	VADisplay va_display;
	VAContextID va_context;
	VAConfigID va_config;
	VASurfaceID *va_surfaces;
	guint va_num_surfaces;
	VBPhandle parser_handle;
	GString *mime_type;
	guint frame_rate_num;
	guint frame_rate_denom;
	guint picture_width;
	guint picture_height;
	gboolean parse_in_progress;
	gboolean discontinuity_frame_in_progress;
	guint64 current_timestamp;
	MixBufferPool *inputbufpool;
	GQueue *inputbufqueue;    
	gboolean va_initialized;
	gboolean end_picture_pending;
	MixVideoFrame* video_frame;    
	guint extra_surfaces;
	MixVideoConfigParamsDec * config_params;
	guint ref_count ;
};


/**
 * mix_videoformat_new:
 * @returns: A newly allocated instance of #MixVideoFormat
 *
 * Use this method to create new instance of #MixVideoFormat
 */
MixVideoFormat *mix_videoformat_new(void);

/**
 * mix_videoformat_ref:
 * @mix: object to add reference
 * @returns: the MixVideoFormat instance where reference count has been increased.
 *
 * Add reference count.
 */
MixVideoFormat *mix_videoformat_ref(MixVideoFormat * mix);

/**
 * mix_videoformat_unref:
 * @obj: object to unref.
 *
 * Decrement reference count of the object.
 */
MixVideoFormat* mix_videoformat_unref(MixVideoFormat* mix);

/* Class Methods */

MIX_RESULT mix_videofmt_getcaps(MixVideoFormat *mix, GString *msg);

MIX_RESULT mix_videofmt_initialize(
	MixVideoFormat *mix,
	MixVideoConfigParamsDec * config_params,
	MixFrameManager * frame_mgr,
	MixBufferPool * input_buf_pool,
	MixSurfacePool ** surface_pool,
	VADisplay va_display);

MIX_RESULT mix_videofmt_decode(
	MixVideoFormat *mix, MixBuffer * bufin[],
	gint bufincnt, MixVideoDecodeParams * decode_params);

MIX_RESULT mix_videofmt_flush(MixVideoFormat *mix);

MIX_RESULT mix_videofmt_eos(MixVideoFormat *mix);

MIX_RESULT mix_videofmt_deinitialize(MixVideoFormat *mix);

#endif /* __MIX_VIDEOFORMAT_H__ */
