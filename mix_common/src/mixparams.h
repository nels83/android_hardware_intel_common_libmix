/* 
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved. 
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

#ifndef __MIX_PARAMS_H__
#define __MIX_PARAMS_H__

#include <glib-object.h>

#define MIX_PARAMS(obj)          (reinterpret_cast<MixParams*> ((obj)))
#define MIX_PARAMS_CAST(obj)     ((MixParams*)(obj))

/**
 * MIX_PARAMS_REFCOUNT:
 * @obj: a #MixParams
 *
 * Get access to the reference count field of the object.
 */
#define MIX_PARAMS_REFCOUNT(obj)           ((MIX_PARAMS_CAST(obj))->ref_count)

/**
 * MixParams:
 * @instance: type instance
 * @refcount: atomic refcount
 *
 * Base class for a refcounted parameter objects.
 */
class MixParams {

public:
	MixParams();
	virtual ~MixParams();
	MixParams* Ref();
	void Unref();
	gint GetRefCount() { return ref_count;}
  
public:
	/**
	* MixParamsDupFunction:
	* @obj: Params to duplicate
	* @returns: reference to cloned instance. 
	*
	* Virtual function prototype for methods to create duplicate of instance.
	*
	*/
	virtual MixParams * dup () const;

	/**
	* MixParamsCopyFunction:
	* @target: target of the copy
	* @src: source of the copy
	* @returns: boolean indicates if copy is successful.
	*
	* Virtual function prototype for methods to create copies of instance.
	*
	*/
	virtual gboolean copy(MixParams* target) const;

	/**
	* MixParamsFinalizeFunction:
	* @obj: Params to finalize
	*
	* Virtual function prototype for methods to free ressources used by
	* object.
	*/
	virtual void finalize ();

	/**
	* MixParamsEqualsFunction:
	* @first: first object in the comparison
	* @second: second object in the comparison
	*
	* Virtual function prototype for methods to compare 2 objects and check if they are equal.
	*/
	virtual gboolean equal (MixParams *obj) const;

public:
	/*< public >*/
	gint ref_count;

	/*< private >*/
	gpointer _reserved; 

};

/**
 * mix_params_get_type:
 * @returns: type of this object.
 * 
 * Get type.
 */
//GType mix_params_get_type(void);

/**
 * mix_params_new:
 * @returns: return a newly allocated object.
 * 
 * Create new instance of the object.
 */
MixParams* mix_params_new();

/**
 * mix_params_copy:
 * @target: copy to target
 * @src: copy from source
 * @returns: boolean indicating if copy is successful.
 * 
 * Copy data from one instance to the other. This method internally invoked the #MixParams::copy method such that derived object will be copied correctly.
 */
gboolean mix_params_copy(MixParams *target, const MixParams *src);


/** 
 * mix_params_ref:
 * @obj: a #MixParams object.
 * @returns: the object with reference count incremented.
 * 
 * Increment reference count.
 */
MixParams* mix_params_ref(MixParams *obj);


/** 
 * mix_params_unref:
 * @obj: a #MixParams object.
 * 
 * Decrement reference count.
 */
void mix_params_unref  (MixParams *obj);

/**
 * mix_params_replace:
 * @olddata: pointer to a pointer to a object to be replaced
 * @newdata: pointer to new object
 *
 * Modifies a pointer to point to a new object.  The modification
 * is done atomically, and the reference counts are updated correctly.
 * Either @newdata and the value pointed to by @olddata may be NULL.
 */
void mix_params_replace(MixParams **olddata, MixParams *newdata);

/**
 * mix_params_dup:
 * @obj: #MixParams object to duplicate.
 * @returns: A newly allocated duplicate of the object, or NULL if failed.
 * 
 * Duplicate the given #MixParams and allocate a new instance. This method is chained up properly and derive object will be dupped properly.
 */
MixParams *mix_params_dup(const MixParams *obj);

/**
 * mix_params_equal:
 * @first: first object to compare
 * @second: second object to compare
 * @returns: boolean indicates if the 2 object contains same data.
 * 
 * Note that the parameter comparison compares the values that are hold inside the object, not for checking if the 2 pointers are of the same instance.
 */
gboolean mix_params_equal(MixParams *first, MixParams *second);
#endif

