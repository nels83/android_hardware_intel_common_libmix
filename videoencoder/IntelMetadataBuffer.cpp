/*
 * Copyright (c) 2007 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "IntelMetadataBuffer.h"
#include <string.h>
#include <stdio.h>

IntelMetadataBuffer::IntelMetadataBuffer()
{
    mType = MetadataBufferTypeCameraSource;
    mValue = 0;
    mInfo = NULL;
    mExtraValues = NULL;
    mExtraValues_Count = 0;
    mBytes = NULL;
    mSize = 0;
}

IntelMetadataBuffer::IntelMetadataBuffer(MetadataBufferType type, int32_t value)
{
    mType = type;
    mValue = value;
    mInfo = NULL;
    mExtraValues = NULL;
    mExtraValues_Count = 0;
    mBytes = NULL;
    mSize = 0;
}
 
IntelMetadataBuffer::~IntelMetadataBuffer()
{
    if (mInfo)
        delete mInfo;

    if (mExtraValues)
        delete[] mExtraValues;

    if (mBytes)
        delete[] mBytes;
}


IntelMetadataBuffer::IntelMetadataBuffer(const IntelMetadataBuffer& imb)
     :mType(imb.mType), mValue(imb.mValue), mInfo(NULL), mExtraValues(NULL),
      mExtraValues_Count(imb.mExtraValues_Count), mBytes(NULL), mSize(imb.mSize)
{
    if (imb.mInfo)
        mInfo = new ValueInfo(*imb.mInfo);

    if (imb.mExtraValues)
    {
        mExtraValues = new int32_t[mExtraValues_Count];
        memcpy(mExtraValues, imb.mExtraValues, 4 * mExtraValues_Count);
    }

    if (imb.mBytes)
    {
        mBytes = new uint8_t[mSize];
        memcpy(mBytes, imb.mBytes, mSize);
    }
}

const IntelMetadataBuffer& IntelMetadataBuffer::operator=(const IntelMetadataBuffer& imb)
{
    mType = imb.mType;
    mValue = imb.mValue;
    mInfo = NULL;
    mExtraValues = NULL;
    mExtraValues_Count = imb.mExtraValues_Count;
    mBytes = NULL;
    mSize = imb.mSize;

    if (imb.mInfo)
        mInfo = new ValueInfo(*imb.mInfo);

    if (imb.mExtraValues)
    {
        mExtraValues = new int32_t[mExtraValues_Count];
        memcpy(mExtraValues, imb.mExtraValues, 4 * mExtraValues_Count);
    }

    if (imb.mBytes)
    {
        mBytes = new uint8_t[mSize];
        memcpy(mBytes, imb.mBytes, mSize);
    }

    return *this;
}

IMB_Result IntelMetadataBuffer::GetType(MetadataBufferType& type)
{
    type = mType;

    return IMB_SUCCESS;
}

IMB_Result IntelMetadataBuffer::SetType(MetadataBufferType type)
{
    if (type < MetadataBufferTypeLast)
        mType = type;
    else
        return IMB_INVAL_PARAM;

    return IMB_SUCCESS;
}

IMB_Result IntelMetadataBuffer::GetValue(int32_t& value)
{
    value = mValue;

    return IMB_SUCCESS;
}

IMB_Result IntelMetadataBuffer::SetValue(int32_t value)
{
    mValue = value;

    return IMB_SUCCESS;
}

IMB_Result IntelMetadataBuffer::GetValueInfo(ValueInfo* &info)
{
    info = mInfo;

    return IMB_SUCCESS;
}

IMB_Result IntelMetadataBuffer::SetValueInfo(ValueInfo* info)
{
    if (info)
    {
        if (mInfo == NULL)
            mInfo = new ValueInfo;

        memcpy(mInfo, info, sizeof(ValueInfo));
    }
    else
        return IMB_INVAL_PARAM;    

    return IMB_SUCCESS;
}

IMB_Result IntelMetadataBuffer::GetExtraValues(int32_t* &values, uint32_t& num)
{
    values = mExtraValues;
    num = mExtraValues_Count;
 
    return IMB_SUCCESS;
}

IMB_Result IntelMetadataBuffer::SetExtraValues(int32_t* values, uint32_t num)
{
    if (values && num > 0)
    {
        if (mExtraValues && mExtraValues_Count != num)
        {
            delete[] mExtraValues;
            mExtraValues = NULL;
        }

        if (mExtraValues == NULL)
            mExtraValues = new int32_t[num];
        
        memcpy(mExtraValues, values, sizeof(int32_t) * num);
        mExtraValues_Count = num;
    }
    else
        return IMB_INVAL_PARAM;

    return IMB_SUCCESS;
}

IMB_Result IntelMetadataBuffer::UnSerialize(uint8_t* data, uint32_t size)
{
    if (!data || size == 0)
        return IMB_INVAL_PARAM;

    MetadataBufferType type;
    int32_t value;
    uint32_t extrasize = size - 8;
    ValueInfo* info = NULL;
    int32_t* ExtraValues = NULL;
    uint32_t ExtraValues_Count = 0;

    memcpy(&type, data, 4);
    data += 4;
    memcpy(&value, data, 4);
    data += 4;
    
    switch (type)
    {
        case MetadataBufferTypeCameraSource:
        case MetadataBufferTypeEncoder:
        case MetadataBufferTypeUser:
        {
            if (extrasize >0 && extrasize < sizeof(ValueInfo))
                return IMB_INVAL_BUFFER;    
            
            if (extrasize > sizeof(ValueInfo)) //has extravalues 
            {
                if ( (extrasize - sizeof(ValueInfo)) % 4 != 0 )
                    return IMB_INVAL_BUFFER;
                ExtraValues_Count = (extrasize - sizeof(ValueInfo)) / 4;
            }
                
            if (extrasize > 0) 
            {
                info = new ValueInfo;
                memcpy(info, data, sizeof(ValueInfo));
                data += sizeof(ValueInfo);
            }

            if (ExtraValues_Count > 0)
            {
                ExtraValues = new int32_t[ExtraValues_Count];
                memcpy(ExtraValues, data, ExtraValues_Count * 4);
            }

            break;
        }
        case MetadataBufferTypeGrallocSource:
            if (extrasize > 0)
                return IMB_INVAL_BUFFER;

            break;
        default:
            return IMB_INVAL_BUFFER;
    }

    //store data
    mType = type;
    mValue = value;
    if (mInfo)
        delete mInfo;
    mInfo = info;
    if (mExtraValues)
        delete[] mExtraValues;
    mExtraValues = ExtraValues;
    mExtraValues_Count = ExtraValues_Count;

    return IMB_SUCCESS;
}

IMB_Result IntelMetadataBuffer::SetBytes(uint8_t* data, uint32_t size)
{
    return UnSerialize(data, size);
}

IMB_Result IntelMetadataBuffer::Serialize(uint8_t* &data, uint32_t& size)
{
    if (mBytes == NULL)
    {
        if (mType == MetadataBufferTypeGrallocSource && mInfo)
            return IMB_INVAL_PARAM;

        //assemble bytes according members
        mSize = 8;
        if (mInfo)
        {
            mSize += sizeof(ValueInfo);
            if (mExtraValues)
                mSize += 4 * mExtraValues_Count;
        }

        mBytes = new uint8_t[mSize];
        uint8_t *ptr = mBytes;
        memcpy(ptr, &mType, 4);
        ptr += 4;
        memcpy(ptr, &mValue, 4);
        ptr += 4;
        
        if (mInfo)
        {
            memcpy(ptr, mInfo, sizeof(ValueInfo));
            ptr += sizeof(ValueInfo);

            if (mExtraValues)
                memcpy(ptr, mExtraValues, mExtraValues_Count * 4);
        }
    }

    data = mBytes;
    size = mSize;
    
    return IMB_SUCCESS;
}

IMB_Result IntelMetadataBuffer::GetBytes(uint8_t* &data, uint32_t& size)
{
    return Serialize(data, size);
}

uint32_t IntelMetadataBuffer::GetMaxBufferSize()
{
    return 256;
}
