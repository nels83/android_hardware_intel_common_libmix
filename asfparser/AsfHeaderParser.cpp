/* INTEL CONFIDENTIAL
* Copyright (c) 2009 Intel Corporation.  All rights reserved.
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
*/



#define LOG_NDEBUG 0
#define LOG_TAG "AsfHeaderParser"
#include <utils/Log.h>

#include "AsfHeaderParser.h"
#include <string.h>

#include <media/stagefright/Utils.h>

AsfHeaderParser::AsfHeaderParser(void)
    : mAudioInfo(NULL),
      mVideoInfo(NULL),
      mFileInfo(NULL),
      mNumObjectParsed(0),
      mIsProtected(false),
      mPlayreadyHeader(NULL),
      mPlayreadyHeaderLen(0),
      mNumberofHeaderObjects(0) {
    mFileInfo = new AsfFileMediaInfo;
    memset(mFileInfo, 0, sizeof(AsfFileMediaInfo));
}

AsfHeaderParser::~AsfHeaderParser(void) {
    delete mFileInfo;
    if (mPlayreadyHeader) {
        delete mPlayreadyHeader;
        mPlayreadyHeader = NULL;
    }

    // Deleting memory from mExtendedStreamPropertiesObj recursively
    for (vector<AsfExtendedStreamPropertiesObject *>::iterator it = mExtendedStreamPropertiesObj.begin(); it != mExtendedStreamPropertiesObj.end(); ++it) {
        for (int i = 0; i < (*it)->extensionSystems.size(); i++) {
            if ((*it)->extensionSystems[i]->extensionSystemInfo != NULL) {
                delete (*it)->extensionSystems[i]->extensionSystemInfo;
                (*it)->extensionSystems[i]->extensionSystemInfo = NULL;
            }
            delete (*it)->extensionSystems[i];
            (*it)->extensionSystems[i] = NULL;
        }
        (*it)->extensionSystems.clear();
        delete (*it);
        (*it) = NULL;
    }
    mExtendedStreamPropertiesObj.clear();

    resetStreamInfo();
}

AsfAudioStreamInfo* AsfHeaderParser::getAudioInfo() const {
    return mAudioInfo;
}

AsfVideoStreamInfo* AsfHeaderParser::getVideoInfo() const {
    return mVideoInfo;
}

AsfFileMediaInfo* AsfHeaderParser::getFileInfo() const {
    return mFileInfo;
}

uint64_t AsfHeaderParser::getDuration() {
    return mFileInfo->duration - mFileInfo->preroll * ASF_SCALE_MS_TO_100NANOSEC;
}

uint32_t AsfHeaderParser::getDataPacketSize() {
    return mFileInfo->packetSize;
}

uint32_t AsfHeaderParser::getPreroll() {
    // in millisecond unit
    return mFileInfo->preroll;
}

uint64_t AsfHeaderParser::getTimeOffset() {
    // in 100-nanoseconds unit
    if (mAudioInfo) {
        return mAudioInfo->timeOffset;
    }

    if (mVideoInfo) {
        return mVideoInfo->timeOffset;
    }

    return 0;
}

bool AsfHeaderParser::hasVideo() {
    return mVideoInfo != NULL;
}

bool AsfHeaderParser::hasAudio() {
    return mAudioInfo != NULL;
}

bool AsfHeaderParser::isSeekable() {
    return mFileInfo->seekable;
}

int AsfHeaderParser::parse(uint8_t *buffer, uint64_t size) {
    int status = ASF_PARSER_SUCCESS;

    // reset parser's status
    mNumObjectParsed = 0;
    resetStreamInfo();
    memset(mFileInfo, 0, sizeof(AsfFileMediaInfo));

    do {
        if (size < sizeof(AsfObject)) {
            return ASF_PARSER_BAD_DATA;
        }

        AsfObject *obj = (AsfObject*)buffer;
        if (obj->objectSize > size) {
            return ASF_PARSER_BAD_VALUE;
        }

        if (obj->objectID == ASF_Header_Object) {
            if (size < sizeof(AsfHeaderObject)) {
                return ASF_PARSER_BAD_DATA;
            }
            AsfHeaderObject *headerObj = (AsfHeaderObject*)buffer;
            mNumberofHeaderObjects = headerObj->numberofHeaderObjects;
            size -= sizeof(AsfHeaderObject);
            buffer += sizeof(AsfHeaderObject);
        } else {
            if(obj->objectID == ASF_File_Properties_Object) {
                status = onFilePropertiesObject(buffer, size);
            } else if(obj->objectID == ASF_Stream_Properties_Object) {
                status = onStreamPropertiesObject(buffer, size);
            } else if(obj->objectID == ASF_Header_Extension_Object) {
                //AsfHeaderExtensionObject *headerExtObj = (AsfHeaderExtensionObject*)buffer;
                if (size < sizeof(AsfHeaderExtensionObject)) {
                    return ASF_PARSER_BAD_DATA;
                }
                status = parseHeaderExtensionObject(
                        buffer + sizeof(AsfHeaderExtensionObject),
                        size - sizeof(AsfHeaderExtensionObject));
            } else if(obj->objectID == ASF_Codec_List_Object) {
            } else if(obj->objectID == ASF_Script_Command_Object) {
            } else if(obj->objectID == ASF_Marker_Object) {
            } else if(obj->objectID == ASF_Bitrate_Mutual_Exclusion_Object) {
            } else if(obj->objectID == ASF_Error_Correction_Object) {
            } else if(obj->objectID == ASF_Content_Description_Object) {
            } else if(obj->objectID == ASF_Extended_Content_Description_Object) {
            } else if(obj->objectID == ASF_Stream_Bitrate_Properties_Object) {
            } else if(obj->objectID == ASF_Content_Branding_Object) {
            } else if(obj->objectID == ASF_Content_Encryption_Object) {
            } else if(obj->objectID == ASF_Extended_Content_Encryption_Object) {
            } else if(obj->objectID == ASF_Digital_Signature_Object) {
            } else if(obj->objectID == ASF_Padding_Object) {
            } else if(obj->objectID == ASF_Protection_System_Identifier_Object) {
                mIsProtected = true;
                LOGV("ASF_Protection_System_Identifier_Object");
                uint64_t playreadyObjSize = obj->objectSize;
                PlayreadyHeaderObj *plrdyHdrObj = (PlayreadyHeaderObj*)buffer;

                uint8_t* playreadyObjBuf = NULL;
                GUID *pldyUuid = (GUID*)plrdyHdrObj->sysId;
                memcpy(mPlayreadyUuid, (uint8_t*)pldyUuid, UUIDSIZE);

                // Rights Management Header - Record Type = 0x0001
                // Traverse till field containing number of records
                playreadyObjBuf = buffer + sizeof(PlayreadyHeaderObj);
                for (int i = 0; i < plrdyHdrObj->countRecords; i++) {
                    uint16_t* recordType = (uint16_t*)playreadyObjBuf;
                    if (*recordType == 0x01)  {// Rights management Header
                        playreadyObjBuf += sizeof(uint16_t);
                        uint16_t* recordLen = (uint16_t*)playreadyObjBuf;
                        mPlayreadyHeaderLen = *recordLen;

                        mPlayreadyHeader = new uint8_t [mPlayreadyHeaderLen];
                        if (mPlayreadyHeader == NULL) {
                            return ASF_PARSER_NO_MEMORY;
                        }
                        playreadyObjBuf += sizeof(uint16_t);
                        memcpy(mPlayreadyHeader, playreadyObjBuf, mPlayreadyHeaderLen);
                        break;
                    }
                }
            }
            if (status != ASF_PARSER_SUCCESS) {
                return status;
            }
            size -= (uint32_t)obj->objectSize;
            buffer += obj->objectSize;
            mNumObjectParsed++;
            if (mNumObjectParsed == mNumberofHeaderObjects) {
                return ASF_PARSER_SUCCESS;
            }
        }
    }
    while (status == ASF_PARSER_SUCCESS);

    return status;
}

int AsfHeaderParser::getPlayreadyUuid(uint8_t *playreadyUuid) {

    if (playreadyUuid == NULL || (!mIsProtected))
        return ASF_PARSER_FAILED;
    memcpy(playreadyUuid, mPlayreadyUuid, UUIDSIZE);
    return ASF_PARSER_SUCCESS;
}

int AsfHeaderParser::getPlayreadyHeaderXml(uint8_t *playreadyHeader, uint32_t *playreadyHeaderLen) {

    if (playreadyHeader == NULL) {
        *playreadyHeaderLen = mPlayreadyHeaderLen;
        return ASF_PARSER_NULL_POINTER;
    }
    memcpy(playreadyHeader, mPlayreadyHeader, mPlayreadyHeaderLen);
    *playreadyHeaderLen = mPlayreadyHeaderLen;

    return ASF_PARSER_SUCCESS;
}

int AsfHeaderParser::onFilePropertiesObject(uint8_t *buffer, uint32_t size) {
    if (size < sizeof(AsfFilePropertiesObject))  {
        return ASF_PARSER_BAD_DATA;
    }

    AsfFilePropertiesObject *obj = (AsfFilePropertiesObject*)buffer;
    mFileInfo->dataPacketsCount = obj->dataPacketsCount;
    mFileInfo->duration = obj->playDuration;
    mFileInfo->fileSize = obj->fileSize;
    mFileInfo->packetSize = obj->maximumDataPacketSize;
    if (mFileInfo->packetSize != obj->minimumDataPacketSize) {
        return ASF_PARSER_BAD_VALUE;
    }
    mFileInfo->preroll = obj->preroll;
    mFileInfo->seekable = obj->flags.bits.seekableFlag;
    if (obj->flags.bits.broadcastFlag) {
        // turn off seeking
        mFileInfo->seekable = false;
    }
    return ASF_PARSER_SUCCESS;
}

int AsfHeaderParser::onStreamPropertiesObject(uint8_t *buffer, uint32_t size) {
    int status;
    if (size < sizeof(AsfStreamPropertiesObject)) {
        return ASF_PARSER_BAD_DATA;
    }

    AsfStreamPropertiesObject *obj = (AsfStreamPropertiesObject*)buffer;
    if (obj->typeSpecificDataLength + obj->errorCorrectionDataLength >
        size - sizeof(AsfStreamPropertiesObject)) {
        return ASF_PARSER_BAD_VALUE;
    }

    uint8_t *typeSpecificData = buffer + sizeof(AsfStreamPropertiesObject);
    if (obj->streamType == ASF_Video_Media) {
        status = onVideoSpecificData(obj, typeSpecificData);
    } else if (obj->streamType == ASF_Audio_Media) {
        status = onAudioSpecificData(obj, typeSpecificData);
    } else {
        // ignore other media specific data
        status = ASF_PARSER_SUCCESS;
    }
    return status;
}

int AsfHeaderParser::onVideoSpecificData(AsfStreamPropertiesObject *obj, uint8_t *data) {
    // size of codec specific data is obj->typeSpecificDataLength
    uint32_t headerLen = sizeof(AsfVideoInfoHeader) + sizeof(AsfBitmapInfoHeader);
    if (obj->typeSpecificDataLength < headerLen) {
        return ASF_PARSER_BAD_DATA;
    }
    AsfVideoInfoHeader *info = (AsfVideoInfoHeader*)data;
    AsfBitmapInfoHeader *bmp = (AsfBitmapInfoHeader*)(data + sizeof(AsfVideoInfoHeader));

    if (info->formatDataSize < sizeof(AsfBitmapInfoHeader)) {
        return ASF_PARSER_BAD_VALUE;
    }

    if (bmp->formatDataSize - sizeof(AsfBitmapInfoHeader) >
        obj->typeSpecificDataLength - headerLen) {

        // codec specific data is invalid
        return ASF_PARSER_BAD_VALUE;
    }

    AsfVideoStreamInfo *videoInfo = new AsfVideoStreamInfo;
    if (videoInfo == NULL) {
        return ASF_PARSER_NO_MEMORY;
    }
    videoInfo->streamNumber = obj->flags.bits.streamNumber;
    videoInfo->encryptedContentFlag = obj->flags.bits.encryptedContentFlag;
    videoInfo->timeOffset = obj->timeOffset;
    videoInfo->width = info->encodedImageWidth;
    videoInfo->height = info->encodedImageHeight;

    if (bmp->compressionID == FOURCC('Y', 'D', 'R', 'P')) {
        // That means PYV content
        uint32_t* ptrActCompId = (uint32_t*)((data + sizeof(AsfVideoInfoHeader) + bmp->formatDataSize - sizeof(uint32_t)));
        bmp->actualCompressionID = *ptrActCompId;
        videoInfo->fourCC = bmp->actualCompressionID;
        LOGV("onVideoSpecificData() with bmp->actualCompressionID = %x", bmp->actualCompressionID);
    } else {
        videoInfo->fourCC = bmp->compressionID;
    }

    // TODO: get aspect ratio from video meta data
    videoInfo->aspectX = 1;
    videoInfo->aspectY = 1;

    videoInfo->codecDataSize = bmp->formatDataSize - sizeof(AsfBitmapInfoHeader);
    if (videoInfo->codecDataSize) {
        videoInfo->codecData = new uint8_t [videoInfo->codecDataSize];
        if (videoInfo->codecData == NULL) {
            delete videoInfo;
            return ASF_PARSER_NO_MEMORY;
        }
        memcpy(videoInfo->codecData,
        data + headerLen,
        videoInfo->codecDataSize);
    } else {
        videoInfo->codecData = NULL;
    }

    videoInfo->next = NULL;
    if (mVideoInfo == NULL) {
        mVideoInfo = videoInfo;
    } else {
        AsfVideoStreamInfo *last = mVideoInfo;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = videoInfo;
    }

    return ASF_PARSER_SUCCESS;
}

int AsfHeaderParser::onAudioSpecificData(AsfStreamPropertiesObject *obj, uint8_t *data) {
    if (obj->typeSpecificDataLength < sizeof(AsfWaveFormatEx)) {
        return ASF_PARSER_BAD_DATA;
    }

    AsfWaveFormatEx *format = (AsfWaveFormatEx*)data;
    if (format->codecSpecificDataSize >
        obj->typeSpecificDataLength - sizeof(AsfWaveFormatEx)) {
        return ASF_PARSER_BAD_VALUE;
    }

    AsfAudioStreamInfo *audioInfo = new AsfAudioStreamInfo;
    if (audioInfo == NULL) {
        return ASF_PARSER_NO_MEMORY;
    }
    audioInfo->streamNumber = obj->flags.bits.streamNumber;
    audioInfo->encryptedContentFlag = obj->flags.bits.encryptedContentFlag;
    audioInfo->timeOffset = obj->timeOffset;
    LOGV("onAudioSpecificData => format->codecIDFormatTag = %x",format->codecIDFormatTag);

    if (format->codecIDFormatTag == 0x5052) {
        uint32_t* ptrActCodecId = (uint32_t*)((data + sizeof(AsfWaveFormatEx) + format->codecSpecificDataSize - sizeof(format->codecIDFormatTag)));
        format->codecIDFormatTag = *ptrActCodecId;
        audioInfo->codecID = format->codecIDFormatTag;
    } else {
        audioInfo->codecID = format->codecIDFormatTag;
    }

    audioInfo->numChannels = format->numberOfChannels;
    audioInfo->sampleRate= format->samplesPerSecond;
    audioInfo->avgByteRate = format->averageNumberOfBytesPerSecond;
    audioInfo->blockAlignment = format->blockAlignment;
    audioInfo->bitsPerSample = format->bitsPerSample;
    audioInfo->codecDataSize = format->codecSpecificDataSize;
    if (audioInfo->codecDataSize) {
        audioInfo->codecData = new uint8_t [audioInfo->codecDataSize];
        if (audioInfo->codecData == NULL) {
            delete audioInfo;
            return ASF_PARSER_NO_MEMORY;
        }
        memcpy(audioInfo->codecData,
            data + sizeof(AsfWaveFormatEx),
            audioInfo->codecDataSize);
    } else {
        audioInfo->codecData = NULL;
    }

    audioInfo->next = NULL;

    if (mAudioInfo == NULL) {
        mAudioInfo = audioInfo;
    } else {
        AsfAudioStreamInfo *last = mAudioInfo;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = audioInfo;
    }

    return ASF_PARSER_SUCCESS;
}


int AsfHeaderParser::onExtendedStreamPropertiesObject(uint8_t *buffer, uint32_t size) {
    int status = ASF_PARSER_SUCCESS;

    if (size < sizeof(AsfObject)) {
        return ASF_PARSER_BAD_DATA;
    }

    AsfExtendedStreamPropertiesObject *extStrObj = new AsfExtendedStreamPropertiesObject;
    if (extStrObj == NULL) {
        return ASF_PARSER_NO_MEMORY;
    }

    AsfExtendedStreamPropertiesObject *obj = (AsfExtendedStreamPropertiesObject *)buffer;
    if (obj->objectSize > size) {
        ALOGE("Invalid ASF Extended Stream Prop Object size");
        delete extStrObj;
        return ASF_PARSER_BAD_VALUE;
    }

    extStrObj->objectID = obj->objectID;
    extStrObj->objectSize = obj->objectSize;
    extStrObj->startTime = obj->startTime;
    extStrObj->endTime = obj->endTime;
    extStrObj->dataBitrate = obj->dataBitrate;
    extStrObj->bufferSize = obj->bufferSize;
    extStrObj->initialBufferFullness = obj->initialBufferFullness;
    extStrObj->alternateDataBitrate = obj->alternateDataBitrate;
    extStrObj->alternateBufferSize = obj->alternateBufferSize;
    extStrObj->alternateInitialBufferFullness = obj->alternateInitialBufferFullness;
    extStrObj->maximumObjectSize = obj->maximumObjectSize;
    extStrObj->flags = obj->flags;
    extStrObj->streamNumber = obj->streamNumber;
    extStrObj->streamLanguageIDIndex = obj->streamLanguageIDIndex;
    extStrObj->averageTimePerFrame = obj->averageTimePerFrame;
    extStrObj->streamNameCount = obj->streamNameCount;
    extStrObj->payloadExtensionSystemCount = obj->payloadExtensionSystemCount;

    ALOGD("stream number = 0x%08X", obj->streamNumber);
    ALOGD("payloadExtensionSystemCount = 0x%08X", obj->payloadExtensionSystemCount);

    // Get pointer to buffer where first extension system object starts
    buffer = (uint8_t *)&(obj->extensionSystems);
    int extSysSize = 0;
    for (int i = 0; i < obj->payloadExtensionSystemCount; i++ ) {
        extSysSize = 0;
        PayloadExtensionSystem *extensionObj = new  PayloadExtensionSystem;
        PayloadExtensionSystem *extObjData = (PayloadExtensionSystem *)buffer;
        // populate the extension object from the buffer
        extensionObj->extensionSystemId = extObjData->extensionSystemId;
        extensionObj->extensionDataSize = extObjData->extensionDataSize;
        extensionObj->extensionSystemInfoLength = extObjData->extensionSystemInfoLength;

        // Allocate space to store extensionSystemInfo
        if (extensionObj->extensionSystemInfoLength > 0) {
            // TODO: make sure this memory is freed when not reuired.
            extensionObj->extensionSystemInfo = new uint8_t [extObjData->extensionSystemInfoLength];
            if (extensionObj->extensionSystemInfo == NULL) {
               delete extensionObj;
               delete extStrObj;
               return ASF_PARSER_NO_MEMORY;
            }
            memcpy(extensionObj->extensionSystemInfo, extObjData->extensionSystemInfo, extObjData->extensionSystemInfoLength);
        } else {
            // no extension system info
            extensionObj->extensionSystemInfo = NULL;
        }

        // calculate the length of current extension system.
        // if there are multiple extension systems then increment buffer by  extSysSize
        // to point to next extension object
        extSysSize += sizeof(GUID) + sizeof(uint16_t) + sizeof(uint32_t) + extensionObj->extensionSystemInfoLength;
        buffer += extSysSize;

        // add the extension object to the extended stream object
        extStrObj->extensionSystems.push_back(extensionObj);
    }

    mExtendedStreamPropertiesObj.push_back(extStrObj);
    return ASF_PARSER_SUCCESS;
}

int AsfHeaderParser::parseHeaderExtensionObject(uint8_t* buffer, uint32_t size) {
    // No empty space, padding, leading, or trailing bytes are allowed in the extention data

    int status = ASF_PARSER_SUCCESS;
    do {
        if (size < sizeof(AsfObject)) {
            return ASF_PARSER_BAD_DATA;
        }

        AsfObject *obj = (AsfObject *)buffer;
        if (obj->objectSize > size) {
            return ASF_PARSER_BAD_VALUE;
        }

        if(obj->objectID == ASF_Extended_Stream_Properties_Object) {
            status = onExtendedStreamPropertiesObject(buffer, size);
        } else if(obj->objectID == ASF_Advanced_Mutual_Exclusion_Object) {
        } else if(obj->objectID == ASF_Group_Mutual_Exclusion_Object) {
        } else if(obj->objectID == ASF_Stream_Prioritization_Object) {
        } else if(obj->objectID == ASF_Bandwidth_Sharing_Object) {
        } else if(obj->objectID == ASF_Language_List_Object) {
        } else if(obj->objectID == ASF_Metadata_Object) {
        } else if(obj->objectID == ASF_Metadata_Library_Object) {
        } else if(obj->objectID == ASF_Index_Parameters_Object) {
        } else if(obj->objectID == ASF_Media_Object_Index_Parameters_Object) {
        } else if(obj->objectID == ASF_Timecode_Index_Parameters_Object) {
        } else if(obj->objectID == ASF_Compatibility_Object) {
        } else if(obj->objectID == ASF_Advanced_Content_Encryption_Object) {
        } else {
        }

        if (status != ASF_PARSER_SUCCESS) {
            break;
        }

        size -= (uint32_t)obj->objectSize;
        buffer += obj->objectSize;

        if (size == 0) {
            break;
        }
    }
    while (status == ASF_PARSER_SUCCESS);

    return status;
}

int AsfHeaderParser::getPayloadExtensionSystems(uint8_t streamNumber, vector<PayloadExtensionSystem *> **extSystems ) {
    for (unsigned int i = 0; i < mExtendedStreamPropertiesObj.size(); i++) {
        if (streamNumber == mExtendedStreamPropertiesObj[i]->streamNumber) {
            *extSystems = &(mExtendedStreamPropertiesObj[i]->extensionSystems);
            return ASF_PARSER_SUCCESS;
        }
    }
    return ASF_PARSER_FAILED;
}

void AsfHeaderParser::resetStreamInfo() {
    while (mAudioInfo) {
         AsfAudioStreamInfo *next = mAudioInfo->next;
         delete [] mAudioInfo->codecData;
         delete mAudioInfo;
         mAudioInfo = next;
     }

     while (mVideoInfo) {
         AsfVideoStreamInfo *next = mVideoInfo->next;
         delete [] mVideoInfo->codecData;
         delete mVideoInfo;
         mVideoInfo = next;
     }
}

