/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

/**
 * SECTION:mixvideoinitparams
 * @short_description: MI-X Video Initialization Parameters
 *
 * The MixVideoInitParams object will be created by the MMF/App 
 * and provided in the mix_video_initialize() function. 
 * The get and set methods for the properties will be available for 
 * the caller to set and get information used at initialization time.
 */

#include "mixvideothread.h"

MixVideoMutex::MixVideoMutex() {
	pthread_mutex_init(&mMutex, NULL);
}
MixVideoMutex::MixVideoMutex(const char* name) {
	pthread_mutex_init(&mMutex, NULL);
}
MixVideoMutex::MixVideoMutex(int type, const char* name) {
	if (type == SHARED) {
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
		pthread_mutex_init(&mMutex, &attr);
		pthread_mutexattr_destroy(&attr);
	} else {
		pthread_mutex_init(&mMutex, NULL);
	}
}
MixVideoMutex::~MixVideoMutex() {
	pthread_mutex_destroy(&mMutex);
}
int MixVideoMutex::lock() {
	return -pthread_mutex_lock(&mMutex);
}
void MixVideoMutex::unlock() {
	pthread_mutex_unlock(&mMutex);
}
int MixVideoMutex::tryLock() {
	return -pthread_mutex_trylock(&mMutex);
}

