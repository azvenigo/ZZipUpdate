//////////////////////////////////////////////////////////////////////////////////////////////////
// ZZipAPI
// Purpose: This is the main API to be used for manipulating Zip Archives.
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

#include <string>
#include <list>
#include <stdint.h>
#include <boost/filesystem.hpp>
#include "ZipHeaders.h"
#include "ZipJob.h"
#include "zlib.h"
#include "common/ZZFileAPI.h"

using namespace std;
using namespace boost::filesystem;

class ZZipAPI
{
public:
    ZZipAPI();
    ~ZZipAPI();

    enum eOpenType
    {
        kZipOpen = 0,       // For existing Zips
        kZipCreate = 1      // For creating new Zips
    };

    bool			        Init(const wstring& sFilename, eOpenType openType = kZipOpen, int32_t nCompressionLevel = Z_DEFAULT_COMPRESSION);
    bool                    Shutdown();

    // Accessors
    wstring                 GetZipFilename() const { return msZipURL; }
    cZipCD&                 GetZipCD() { return mZipCD;  }

    // Commands for existing Zips
    void                    DumpReport(const wstring& sOutputFilename);
    bool                    DecompressToBuffer(const wstring& sFilename, uint8_t* pOutputBuffer, Progress* pProgress = nullptr);    // output buffer must be large enough to hold entire output
    bool                    DecompressToFile(const wstring& sFilename, const wstring& sOutputFilename, Progress* pProgress = nullptr);
    bool                    DecompressToFolder(const wstring& sPattern, const wstring& sOutputFolder, Progress* pProgress = nullptr);
    bool                    ExtractRawStream(const wstring& sFilename, const wstring& sOutputFilename, Progress* pProgress = nullptr);

    // Commands for creating new Zips
    bool                    AddToZipFile(const wstring& sFilename, const wstring& sBaseFolder, Progress* pProgress = nullptr);  // Only usable if zip file was open with kZipCreate
    bool                    AddToZipFileFromBuffer(uint8_t* nInputBufferSize, uint32_t nBufferSize, const wstring& sFilename, Progress* pProgress = nullptr);       // filename is the relative path within the zipfile 

private:
    bool                    OpenForReading();
    bool                    CreateZipFile();

    eOpenType               mOpenType;              // kZipOpen or kZipCreate
    int32_t                 mnCompressionLevel;     // Valid ranges from -1 (default) to 9.
    wstring                 msZipURL;               // path to the zip archive or URL
    shared_ptr<cZZFile>     mpZZFile;               // Abstraction to local file or HTTP file
    cZipCD                  mZipCD;                 // Zip Central Directory including all headers
    bool                    mbInitted;
};