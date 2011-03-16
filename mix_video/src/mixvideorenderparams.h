/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __MIX_VIDEORENDERPARAMS_H__
#define __MIX_VIDEORENDERPARAMS_H__

#include <mixparams.h>
#include "mixvideodef.h"
#include "mixdisplay.h"
#include "mixvideoframe.h"
#include <va/va.h>

/**
 * MIX_VIDEORENDERPARAMS:
 * @obj: object to be type-casted.
 */
#define MIX_VIDEORENDERPARAMS(obj) (reinterpret_cast<MixVideoRenderParams*>(obj))

/**
 * MIX_IS_VIDEORENDERPARAMS:
 * @obj: an object.
 *
 * Checks if the given object is an instance of #MixParams
 */
#define MIX_IS_VIDEORENDERPARAMS(obj) ((NULL != MIX_VIDEORENDERPARAMS(obj)) ? TRUE : FALSE)


/**
 * MixVideoRenderParams:
 *
 * MI-X VideoRender Parameter object
 */
class MixVideoRenderParams : public MixParams {
public:
    MixVideoRenderParams();
    virtual ~MixVideoRenderParams();
    virtual bool copy(MixParams *target) const;
    virtual bool equal(MixParams*) const;
    virtual MixParams* dup() const;

    MIX_RESULT set_clipping_rects(MixRect* clipping_rects,
                                  uint number_of_clipping_rects);
    MIX_RESULT get_clipping_rects(MixRect ** clipping_rects,
                                  uint* number_of_clipping_rects);
    MIX_RESULT get_va_cliprects(VARectangle ** va_cliprects,
                                uint* number_of_cliprects);

public:
    /*< public > */
    /* Pointer to a MixDisplay object
     * such as MixDisplayX11 */
    MixDisplay *display;

    /* MixRect object to define offset,
     * height and width of source image */
    MixRect src_rect;

    /* MixRect object to define offset,
     * height and width of the display
     * destination */
    MixRect dst_rect;

    /* Array of clipping rectangles
     * to be applied */
    MixRect *clipping_rects;

    /* Number of clipping rectangles
     * in clipping_rects */
    uint number_of_clipping_rects;

    /* Post processing parameters */
    uint post_proc;

    /* Reserved */
    void* reserved;

    /* Reserved for future use */
    void* reserved1;

    /* Reserved for future use */
    void* reserved2;

    /* Reserved for future use */
    void* reserved3;

    /* Reserved for future use */
    void* reserved4;

private:
    VARectangle *mVa_cliprects;

};


/**
 * mix_videorenderparams_new:
 * @returns: A newly allocated instance of #MixVideoRenderParams
 *
 * Use this method to create new instance of #MixVideoRenderParams
 */

MixVideoRenderParams *mix_videorenderparams_new(void);
/**
 * mix_videorenderparams_ref:
 * @mix: object to add reference
 * @returns: the MixVideoRenderParams instance where reference count has been increased.
 *
 * Add reference count.
 */

MixVideoRenderParams *mix_videorenderparams_ref(MixVideoRenderParams * mix);

/**
 * mix_videorenderparams_unref:
 * @obj: object to unref.
 *
 * Decrement reference count of the object.
 */
#define mix_videorenderparams_unref(obj) mix_params_unref(MIX_PARAMS(obj))

/* Class Methods */

/**
 * mix_videorenderparams_set_display:
 * @obj: #MixVideoRenderParams object
 * @display: #MixDisplay object
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set MixDisplay Object
 */
MIX_RESULT mix_videorenderparams_set_display(
    MixVideoRenderParams * obj, MixDisplay * display);

/**
 * mix_videorenderparams_get_display:
 * @obj: #MixVideoRenderParams object
 * @display: pointer to #MixDisplay object
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get MixDisplay Object
 */
MIX_RESULT mix_videorenderparams_get_display(
    MixVideoRenderParams * obj, MixDisplay ** display);

/**
 * mix_videorenderparams_set_src_rect:
 * @obj: #MixVideoRenderParams object
 * @src_rect: MixRect object to define offset, height and width of source image
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set source rectangle
 */
MIX_RESULT mix_videorenderparams_set_src_rect(
    MixVideoRenderParams * obj, MixRect src_rect);

/**
 * mix_videorenderparams_get_src_rect:
 * @obj: #MixVideoRenderParams object
 * @src_rect: Source rectangle to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get source rectangle
 */
MIX_RESULT mix_videorenderparams_get_src_rect(
    MixVideoRenderParams * obj, MixRect * src_rect);

/**
 * mix_videorenderparams_set_dest_rect:
 * @obj: #MixVideoRenderParams object
 * @dst_rect: MixRect object to define offset, height and width of the display destination
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set destination rectangle
 */
MIX_RESULT mix_videorenderparams_set_dest_rect(
    MixVideoRenderParams * obj, MixRect dst_rect);

/**
 * mix_videorenderparams_set_dest_rect:
 * @obj: #MixVideoRenderParams object
 * @dst_rect: MixRect object to define offset, height and width of the display destination
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get destination rectangle
 */
MIX_RESULT mix_videorenderparams_get_dest_rect(
    MixVideoRenderParams * obj, MixRect * dst_rect);

/**
 * mix_videorenderparams_set_clipping_rects:
 * @obj: #MixVideoRenderParams object
 * @clipping_rects: Array of clipping rectangles to be applied
 * @number_of_clipping_rects: Number of clipping rectangles in clipping_rects
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set clipping rectangles
 */
MIX_RESULT mix_videorenderparams_set_clipping_rects(
    MixVideoRenderParams * obj, MixRect* clipping_rects, uint number_of_clipping_rects);

/**
 * mix_videorenderparams_get_clipping_rects:
 * @obj: #MixVideoRenderParams object
 * @clipping_rects: Array of clipping rectangles returned
 * @number_of_clipping_rects: Number of clipping rectangles in clipping_rects returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get clipping rectangles
 *
 * <note>
 * DO NOT free clipping_rects!
 * </note>
 */
MIX_RESULT mix_videorenderparams_get_clipping_rects(
    MixVideoRenderParams * obj, MixRect ** clipping_rects, uint* number_of_clipping_rects);

/* TODO: Add getters and setters for other properties */



#endif /* __MIX_VIDEORENDERPARAMS_H__ */
