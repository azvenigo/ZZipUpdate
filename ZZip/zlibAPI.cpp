// MIT License
// Copyright 2019 Alex Zvenigorodsky
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <stdlib.h>
#include <stdio.h>
#include "zlib.h"
#include "zutil.h"
#include <iostream>

#ifndef ZLIB_INTERNAL
#define ZLIB_INTERNAL
#endif

#include "inftrees.h"
#include "inffixed.h"
#include "inflate.h"
#include "zlibAPI.h"
#include "common/crc32Fast.h"


ZDecompressor::ZDecompressor()
{
    mInitialized = false;
    mpZStream = NULL;
    mpOutputBuffer = NULL;
    mnOutputBufferSpace = 0;
    mnOutputAvailable = 0;
    mTotalInputBytesProcessed = 0;
    mTotalOutputBytes = 0;
    mStatus = Z_OK;
    mbFinalPass = false;
}

ZDecompressor::~ZDecompressor()
{
    Shutdown();
}

const int32_t kDefaultDecompressBuffer  = 256 * 1024;   // 256KB Decompression Buffer
const int32_t kDefaultCompressBuffer    = 1024 * 1024;  // 1MB Compression Buffer

int32_t ZDecompressor::Init()
{
    if (!mInitialized)
    {
        mpZStream = (z_stream*)(malloc(sizeof(z_stream)));

        mpZStream->zalloc = nullptr;
        mpZStream->zfree = nullptr;;
        mpZStream->opaque = nullptr;
        mpZStream->next_in = nullptr;
        mpZStream->avail_in = 0;

        mInitialized = true;
    }

    mnOutputAvailable = 0;
    mTotalInputBytesProcessed = 0;
    mTotalOutputBytes = 0;
    mbFinalPass = false;

    mStatus = inflateInit2(mpZStream, -MAX_WBITS);
    if (mStatus != Z_OK)
        return mStatus;

    uint8_t* pNew = (uint8_t*)ZALLOC(mpZStream, 1, mnOutputBufferSpace + kDefaultDecompressBuffer);
    memcpy((void*)pNew, mpOutputBuffer, mnOutputBufferSpace);
    ZFREE(mpZStream, mpOutputBuffer);
    mpOutputBuffer = pNew;
    mnOutputBufferSpace += kDefaultDecompressBuffer;

    return mStatus;
}

int32_t ZDecompressor::Shutdown()
{
    if (mpOutputBuffer)
    {
        ZFREE(mpZStream, mpOutputBuffer);
        mpOutputBuffer = NULL;
    }
    mnOutputBufferSpace = 0;
    mnOutputAvailable = 0;
    mTotalInputBytesProcessed = 0;
    mTotalOutputBytes = 0;

    inflateEnd(mpZStream);
    free(mpZStream);
    mpZStream = NULL;

    mInitialized = false;
    mbFinalPass = false;

    return Z_OK;
}


int32_t ZDecompressor::InitStream(uint8_t* pInputBuf, int32_t nLength)
{
    if (mStatus != Z_OK || pInputBuf == NULL || nLength < 0 || !mInitialized)
        return Z_ERRNO;

    mbFinalPass = false;
    mpZStream->next_in = (uint8_t*)pInputBuf;
    mpZStream->avail_in = nLength;

    return Z_OK;
}

int32_t ZDecompressor::Decompress()
{
    if (!mInitialized)
    {
        mStatus = Z_ERRNO;
        return Z_ERRNO;
    }

    mnOutputAvailable = 0;

    if (mpZStream->avail_in > 0 || mbFinalPass)
    {
        mpZStream->next_out = (uint8_t*)(mpOutputBuffer);
        mpZStream->avail_out = (uInt)mnOutputBufferSpace;

        uint8_t* pNextOutBeforeInflate = mpZStream->next_out;
        uint8_t* pNextInBeforeInflate = mpZStream->next_in;

        mStatus = inflate(mpZStream, Z_SYNC_FLUSH);

        uint8_t* pNextOutAfterInflate = mpZStream->next_out;
        uint8_t* pNextInAfterInflate = mpZStream->next_in;

        int32_t bytesProcessed = (int32_t)(pNextInAfterInflate - pNextInBeforeInflate);
        int32_t bytesDecompressed = (int32_t)(pNextOutAfterInflate - pNextOutBeforeInflate);

        // Tracking
        mnOutputAvailable = pNextOutAfterInflate - pNextOutBeforeInflate;
        mTotalInputBytesProcessed += bytesProcessed;
        mTotalOutputBytes += bytesDecompressed;

        // Make sure we grab all the data if we've exhausted our input and output buffers (there may be more)
        if (mStatus == Z_OK && mpZStream->avail_in == 0 && mpZStream->avail_out == 0)
        {
            mbFinalPass = true;
        }

        if (mStatus == Z_STREAM_END)
        {
            inflateEnd(mpZStream);
            return Z_STREAM_END;
        }

        if (mStatus < 0)
        {
            if (mStatus != Z_BUF_ERROR)
                inflateEnd(mpZStream);
            else
                mStatus = Z_OK; // Explicitly allow Z_BUF_ERROR because we either have more data coming, or we will catch this error via other means
        }
    }

    return mStatus;
}

bool ZDecompressor::HasMoreOutput()
{
    if (!mpZStream)
        return false;

    // If there is more input data but no more output buffer
    if (mpZStream->avail_in > 0)
        return true;

    // if there is no more data coming in, the
    return mStatus == Z_OK && mnOutputAvailable > 0;
}

bool ZDecompressor::NeedsMoreInput()
{
    if (!mpZStream)
        return false;

    return mStatus == Z_OK;
}

ZCompressor::ZCompressor()
{
    mbInitted = false;
    mStatus = Z_OK;
    mpZStream = NULL;
    mpOutputBuffer = NULL;
    mnOutputBufferSpace = 0;
    mnOutputAvailable = 0;

    mTotalInputBytesProcessed = 0;
    mTotalOutputBytes = 0;
}

ZCompressor::~ZCompressor()
{
    Shutdown();
}

int32_t ZCompressor::Init(int nCompressionLevel)
{
    if (!mbInitted)
    {
        mpZStream = (z_stream*)(malloc(sizeof(z_stream)));

        mpZStream->zalloc = nullptr;
        mpZStream->zfree = nullptr; 
        mpZStream->opaque = nullptr;
        mpZStream->next_in = nullptr;
        mpZStream->avail_in = 0;

        mStatus = deflateInit2(mpZStream, nCompressionLevel, Z_DEFLATED, -MAX_WBITS, 9, Z_DEFAULT_STRATEGY);

        if (mStatus == Z_OK)
        {
            mbInitted = true;
            mnOutputAvailable = 0;
            mTotalInputBytesProcessed = 0;
            mTotalOutputBytes = 0;      
        }

        uint8_t* pNew = (uint8_t*)ZALLOC(mpZStream, 1, mnOutputBufferSpace + kDefaultCompressBuffer);
        memcpy((void*)pNew, mpOutputBuffer, mnOutputBufferSpace);
        ZFREE(mpZStream, mpOutputBuffer);
        mpOutputBuffer = pNew;
        mnOutputBufferSpace += kDefaultCompressBuffer;
    }

    return mStatus;
}

int32_t ZCompressor::Shutdown()
{
    if (mpOutputBuffer)
    {
        ZFREE(mpZStream, mpOutputBuffer);
        mpOutputBuffer = NULL;
    }
    mnOutputBufferSpace = 0;
    mnOutputAvailable = 0;
    mTotalInputBytesProcessed = 0;
    mTotalOutputBytes = 0;

    deflateEnd(mpZStream);
    free(mpZStream);
    mpZStream = NULL;

    mbInitted = false;

    return Z_OK;
}

int32_t ZCompressor::InitStream(uint8_t* sourceStream, int32_t sourceLength)
{
    if (mStatus != Z_OK || sourceStream == NULL || sourceLength < 0)
        return Z_ERRNO;

    if (!mbInitted)
    {
        int32_t result  = Init();
        if (result != Z_OK)
            return result;
    }

    mpZStream->next_in = (uint8_t*) sourceStream;
    mpZStream->avail_in = sourceLength;

    return Z_OK;
}

int32_t ZCompressor::Compress(bool bFinalBlock)
{
    if (!mbInitted)
    {
        mStatus = Z_ERRNO;
        return Z_ERRNO;
    }

    mnOutputAvailable = 0;

    if (mpZStream->avail_in > 0)
    {
        mpZStream->next_out = (uint8_t*)(mpOutputBuffer);
        mpZStream->avail_out = (uInt)mnOutputBufferSpace;

        uint8_t* pNextOutBeforeDeflate = mpZStream->next_out;
        uint8_t* pNextInBeforeDeflate = mpZStream->next_in;

        if (bFinalBlock)
            mStatus = deflate(mpZStream, Z_FINISH);
        else
            mStatus = deflate(mpZStream, Z_SYNC_FLUSH);

        uint8_t* pNextOutAfterDeflate = mpZStream->next_out;
        uint8_t* pNextInAfterDeflate = mpZStream->next_in;

        int32_t bytesProcessed = (int32_t)(pNextInAfterDeflate - pNextInBeforeDeflate);
        int32_t bytesCompressed = (int32_t)(pNextOutAfterDeflate - pNextOutBeforeDeflate);

        // Tracking
        mnOutputAvailable = bytesCompressed;
        mTotalInputBytesProcessed += bytesProcessed;
        mTotalOutputBytes += bytesCompressed;
    }

    return mStatus;
}


bool ZCompressor::HasMoreOutput()
{
    if (!mpZStream)
        return false;

    // If there is more input data but no more output buffer
    if (mpZStream->avail_in > 0)
        return true;

    // if there is no more data coming in
    return mStatus == Z_OK && mnOutputAvailable > 0;
}

bool ZCompressor::NeedsMoreInput()
{
    if (!mpZStream)
        return false;

    return mStatus == Z_OK;
}
