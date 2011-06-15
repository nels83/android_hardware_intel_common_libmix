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




#include "AsfHeaderParser.h"
#include <string.h>


AsfHeaderParser::AsfHeaderParser(void)
    : mAudioInfo(NULL),
      mVideoInfo(NULL),
      mFileInfo(NULL),
      mNumObjectParsed(0),
      mNumberofHeaderObjects(0) {
    mFileInfo = new AsfFileMediaInfo;
    memset(mFileInfo, 0, sizeof(AsfFileMediaInfo));
}

AsfHeaderParser::~AsfHeaderParser(void) {
    delete mFileInfo;

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

int AsfHeaderParser::parse(uint8_t *buffer, uint32_t size) {
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
            } else {
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
    videoInfo->fourCC = bmp->compressionID;

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
    audioInfo->codecID = format->codecIDFormatTag;
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
    return ASF_PARSER_SUCCESS;
}

int AsfHeaderParser::parseHeaderExtensionObject(uint8_t* buffer, uint32_t size) {
    // No empty space, padding, leading, or trailing bytes are allowed in the extention data
    int status;
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

