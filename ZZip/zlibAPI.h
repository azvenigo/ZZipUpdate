//////////////////////////////////////////////////////////////////////////////////////////////////
// zlibAPI
// Purpose: This is a thin wrapper around zlib compressor/decompressor
//
// MIT License
// Copyright 2019 Alex Zvenigorodsky
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include <stdint.h>
#include <zlib.h>

class ZDecompressor
{
public:
    ZDecompressor();
    ~ZDecompressor();

    int32_t	    Init();
    int32_t     Shutdown();

    int32_t     InitStream(uint8_t* pInputBuf, int32_t nLength);
    int32_t     Decompress();

    bool        HasMoreOutput();    // true if there is more output pending that didn't fit into the output buffer
    bool        NeedsMoreInput();   // true if the decompressor hasn't reached the end of the stream (Z_STREAM_END)

    uint8_t*    GetDecompressedBuffer() { return mpOutputBuffer; }
    uint64_t    GetDecompressedBytes() { return mnOutputAvailable; }

    uint64_t    GetTotalInputBytesProcessed() { return mTotalInputBytesProcessed; }
    uint64_t	GetTotalOutputBytes() { return mTotalOutputBytes; }

private:
    bool		mInitialized;
    int32_t     mStatus;

    z_stream*	mpZStream;
    uint8_t*	mpOutputBuffer;                 
    uint32_t    mnOutputBufferSpace;
    uint64_t    mnOutputAvailable;
    uint64_t    mTotalInputBytesProcessed;      // all the compressed data passed in
    uint64_t    mTotalOutputBytes;				// all of the decompressed data returned
    bool        mbFinalPass;                    // Input and output buffers may be exhausted but there may be more to inflate so another pass may be needed
};

class ZCompressor
{
public:
    ZCompressor();
    ~ZCompressor();

    int32_t	    Init(int nCompressionLevel = Z_DEFAULT_COMPRESSION);
    int32_t     Shutdown();

    int32_t     InitStream(uint8_t* pInputBuf, int32_t nLength);
    int32_t     Compress(bool bFinalBlock = false);     // bFinalBlock flushes stream if no more data incoming.  

    bool        HasMoreOutput();    // true if there is more output pending that didn't fit into the output buffer
    bool        NeedsMoreInput();   // true if the decompressor hasn't reached the end of the stream (Z_STREAM_END)

    uint8_t*    GetCompressedBuffer() { return mpOutputBuffer; }
    uint64_t    GetCompressedBytes() { return mnOutputAvailable; }

    uint64_t    GetTotalInputBytesProcessed() { return mTotalInputBytesProcessed; }
    uint64_t	GetTotalOutputBytes() { return mTotalOutputBytes; }


private:
    bool		mbInitted;
    int32_t     mStatus;

    z_stream*	mpZStream;
    uint8_t*	mpOutputBuffer;
    uint32_t    mnOutputBufferSpace;
    uint64_t    mnOutputAvailable;
    uint64_t    mTotalInputBytesProcessed;
    uint64_t    mTotalOutputBytes;
};


