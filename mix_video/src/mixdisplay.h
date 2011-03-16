/*
INTEL CONFIDENTIAL
Copyright 2009 Intel Corporation All Rights Reserved.
The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

#ifndef __MIX_DISPLAY_H__
#define __MIX_DISPLAY_H__

#include <mixtypes.h>

#define MIX_DISPLAY(obj)	(reinterpret_cast<MixDisplay*>(obj))

/**
* MixDisplay:
* @instance: type instance
* @refcount: atomic refcount
*
* Base class for a refcounted parameter objects.
*/
class MixDisplay {
public:
    virtual ~MixDisplay();

    virtual MixDisplay* Dup() const;
    virtual bool Copy(MixDisplay* target) const;
    virtual void Finalize();
    virtual bool Equal(const MixDisplay* obj) const;

    MixDisplay * Ref();
    void Unref ();

    friend MixDisplay *mix_display_new (void);

protected:
    MixDisplay();
public:
    /*< public > */
    int refcount;
    /*< private > */
    void* _reserved;
};


/**
* mix_display_new:
* @returns: return a newly allocated object.
*
* Create new instance of the object.
*/
MixDisplay *mix_display_new (void);



/**
* mix_display_copy:
* @target: copy to target
* @src: copy from source
* @returns: boolean indicating if copy is successful.
*
* Copy data from one instance to the other. This method internally invoked the #MixDisplay::copy method such that derived object will be copied correctly.
*/
bool mix_display_copy (MixDisplay * target, const MixDisplay * src);

/**
* mix_display_ref:
* @obj: a #MixDisplay object.
* @returns: the object with reference count incremented.
*
* Increment reference count.
*/
MixDisplay *mix_display_ref (MixDisplay * obj);

/**
* mix_display_unref:
* @obj: a #MixDisplay object.
*
* Decrement reference count.
*/
void mix_display_unref (MixDisplay * obj);

/**
* mix_display_replace:
* @olddata: old data
* @newdata: new data
*
* Replace a pointer of the object with the new one.
*/
void mix_display_replace (MixDisplay ** olddata, MixDisplay * newdata);

/**
* mix_display_dup:
* @obj: #MixDisplay object to duplicate.
* @returns: A newly allocated duplicate of the object, or NULL if failed.
*
* Duplicate the given #MixDisplay and allocate a new instance. This method is chained up properly and derive object will be dupped properly.
*/
MixDisplay *mix_display_dup (const MixDisplay * obj);

/**
* mix_display_equal:
* @first: first object to compare
* @second: second object to compare
* @returns: boolean indicates if the 2 object contains same data.
*
* Note that the parameter comparison compares the values that are hold inside the object, not for checking if the 2 pointers are of the same instance.
*/
bool mix_display_equal (MixDisplay * first, MixDisplay * second);


#endif
