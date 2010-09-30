/* 
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __MIX_BUFFER_H__
#define __MIX_BUFFER_H__

#include <mixparams.h>
#include "mixvideodef.h"

G_BEGIN_DECLS

/**
 * MIX_TYPE_BUFFER:
 *
 * Get type of class.
 */
#define MIX_TYPE_BUFFER (mix_buffer_get_type ())

/**
 * MIX_BUFFER:
 * @obj: object to be type-casted.
 */
#define MIX_BUFFER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MIX_TYPE_BUFFER, MixBuffer))

/**
 * MIX_IS_BUFFER:
 * @obj: an object.
 *
 * Checks if the given object is an instance of #MixParams
 */
#define MIX_IS_BUFFER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIX_TYPE_BUFFER))

/**
 * MIX_BUFFER_CLASS:
 * @klass: class to be type-casted.
 */
#define MIX_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MIX_TYPE_BUFFER, MixBufferClass))

/**
 * MIX_IS_BUFFER_CLASS:
 * @klass: a class.
 *
 * Checks if the given class is #MixParamsClass
 */
#define MIX_IS_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MIX_TYPE_BUFFER))

/**
 * MIX_BUFFER_GET_CLASS:
 * @obj: a #MixParams object.
 *
 * Get the class instance of the object.
 */
#define MIX_BUFFER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), MIX_TYPE_BUFFER, MixBufferClass))

typedef void (*MixBufferCallback)(gulong token, guchar *data);

typedef struct _MixBuffer MixBuffer;
typedef struct _MixBufferClass MixBufferClass;

/**
 * MixBuffer:
 *
 * MI-X Buffer Parameter object
 */
struct _MixBuffer {
	/*< public > */
	MixParams parent;

	/*< public > */
	
	/* Pointer to coded data buffer */
	guchar *data;
	
	/* Size of coded data buffer */
	guint size;
	
	/* Token that will be passed to 
	 * the callback function. Can be 
	 * used by the application for 
	 * any information to be associated 
	 * with this coded data buffer, 
	 * such as a pointer to a structure 
	 * belonging to the application. */
	gulong token;
	
	/* callback function pointer */
	MixBufferCallback callback;

	/* < private > */
	/* reserved */
	gpointer reserved;
};

/**
 * MixBufferClass:
 *
 * MI-X VideoConfig object class
 */
struct _MixBufferClass {
	/*< public > */
	MixParamsClass parent_class;

	/* class members */
};

/**
 * mix_buffer_get_type:
 * @returns: type
 *
 * Get the type of object.
 */
GType mix_buffer_get_type(void);

/**
 * mix_buffer_new:
 * @returns: A newly allocated instance of #MixBuffer
 *
 * Use this method to create new instance of #MixBuffer
 */
MixBuffer *mix_buffer_new(void);
/**
 * mix_buffer_ref:
 * @mix: object to add reference
 * @returns: the #MixBuffer instance where reference count has been increased.
 *
 * Add reference count.
 */
MixBuffer *mix_buffer_ref(MixBuffer * mix);

/**
 * mix_buffer_unref:
 * @obj: object to unref.
 *
 * Decrement reference count of the object.
 */
void mix_buffer_unref(MixBuffer * mix);

/* Class Methods */

/**
 * mix_buffer_set_data:
 * @obj: #MixBuffer object
 * @data: data buffer  
 * @size: data buffer size  
 * @token:  token 
 * @callback: callback function pointer  
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set data buffer, size, token and callback function 
 */
MIX_RESULT mix_buffer_set_data(MixBuffer * obj, guchar *data, guint size,
		gulong token, MixBufferCallback callback);

G_END_DECLS

#endif /* __MIX_BUFFER_H__ */
