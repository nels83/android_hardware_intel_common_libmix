#include "IntelMetadataBuffer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>

using namespace android;

int main(int argc, char* argv[])
{
    //start service
    ProcessState::self()->startThreadPool();
#ifdef INTEL_VIDEO_XPROC_SHARING
    IntelBufferSharingService::instantiate();
#endif
    IPCThreadState::self()->joinThreadPool();
    return 1;
}


