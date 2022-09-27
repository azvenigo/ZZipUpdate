// MIT 7icense
// Copyright 2019 Alex Zvenigorodsky
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "ZZFileAPI.h"
#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <mutex>
#include <vector>
#include <assert.h>
#include "StringHelpers.h"

using namespace std;

const string kHTTPTag("http://");
const string kHTTPSTag("https://");

extern bool gbSkipCertCheck;

enum
{
    kZZfileError_None           = 0,
    kZZFileError_Exception      = -5001,
    kZZFileError_General        = -5002,
    kZZFileError_Unsupported    = -5003,
};


// Factory
bool cZZFile::Open(const string& sURL, bool bWrite, shared_ptr<cZZFile>& pFile, const string& sName, const string& sPassword, bool bVerbose)
{
    // 2022/9/17 - temp setting verbose
    //bVerbose = true;


    cZZFile* pNewFile = nullptr;
    if (sURL.substr(0, 4) == "http")
    {
        pNewFile = new cHTTPFile();
    }
    else
    {
        pNewFile = new cZZFileLocal();
    }

    pFile.reset(pNewFile);
    return pNewFile->OpenInternal(sURL, bWrite, sName, sPassword, bVerbose);   // call protected virtualized Open
}

bool cZZFile::Open(const wstring& sURL, bool bWrite, shared_ptr<cZZFile>& pFile, const wstring& sName, const wstring& sPassword, bool bVerbose)
{
//    return Open(string(sURL.begin(), sURL.end()), bWrite, pFile, bVerbose);
    return Open(StringHelpers::wstring_to_string(sURL), bWrite, pFile, StringHelpers::wstring_to_string(sName), StringHelpers::wstring_to_string(sPassword), bVerbose);
}


cZZFile::cZZFile() : mnFileSize(0), mnLastError(kZZfileError_None)
{
}

cZZFileLocal::cZZFileLocal() : cZZFile()
{
}

cZZFileLocal::~cZZFileLocal()
{
    cZZFileLocal::Close();
}

bool cZZFileLocal::OpenInternal(string sURL, bool bWrite, string sName, string sPassword, bool bVerbose)
{
    mnLastError = kZZfileError_None;
    mbVerbose = bVerbose;

    if (bWrite)
        mFileStream.open(sURL, ios_base::in | ios_base::out | ios_base::binary | ios_base::trunc);
    else
        mFileStream.open(sURL, ios_base::in | ios_base::binary);

    if (mFileStream.fail())
    {
        mnLastError = errno;
        //cerr << "Failed to open file:" << sURL.c_str() << "! Reason: " << errno << "\n";
        return false;
    }

    mFileStream.seekg(0, ios::end);     // seek to end
    mnFileSize = mFileStream.tellg();

    mFileStream.seekg(0, ios::beg);

    return true;
}

bool cZZFileLocal::Close()
{
    mFileStream.close();
    mnLastError = kZZfileError_None;

    return true;
}

bool cZZFileLocal::Read(int64_t nOffset, uint32_t nBytes, uint8_t* pDestination, uint32_t& nBytesRead)
{
    std::unique_lock<mutex> lock(mMutex);

    mnLastError = kZZfileError_None;

    if (nOffset != ZZFILE_NO_SEEK)
    {
        mFileStream.seekg(nOffset, ios::beg);
        if (mFileStream.fail())
        {
            mnLastError = errno;
            cerr << "Failed to seek to " << nOffset << "reason:" << mnLastError << "\n";
            return false;
        }
    }

    mFileStream.read((char*)pDestination, nBytes);
    uint32_t nRead = (uint32_t)mFileStream.gcount();
    if (mFileStream.fail() && nRead == 0)
    {
        mnLastError = errno;
        cerr << "Failed to read " << nBytes << " bytes reason:" << mnLastError << "\n";
        return false;
    }

    nBytesRead = nRead;
    return true;
}

bool cZZFileLocal::Write(int64_t nOffset, uint32_t nBytes, uint8_t* pSource, uint32_t& nBytesWritten)
{
    std::unique_lock<mutex> lock(mMutex);

    mnLastError = kZZfileError_None;

    if (nOffset == ZZFILE_SEEK_END)
    {
        mFileStream.seekg(0, ios::end);
    }

    uint64_t nOffsetBeforeWrite = mFileStream.tellg();

    if (nOffset >= 0)
    {
        mFileStream.seekg(nOffset, ios::beg);
        if (mFileStream.fail())
        {
            mnLastError = errno;
            cerr << "Failed to seek to " << nOffset << "reason:" << mnLastError << "\n";
            return false;
        }

        nOffsetBeforeWrite = nOffset;
    }

    mFileStream.write((char*)pSource, nBytes);
    if (mFileStream.fail())
    {
        mnLastError = errno;
        cerr << "Failed to write:" << nBytes << " bytes! Reason: " << mnLastError << "\n";
        return false;
    }

    nBytesWritten = nBytes;

    // If we're writing past the end of the current file size we need to know what the new file size is
    if (nOffsetBeforeWrite + nBytes > mnFileSize)
    {
        mnFileSize = nOffsetBeforeWrite + nBytes;
    }

    return true;
}


struct HTTPFileResponse
{
    HTTPFileResponse() : pDest(nullptr), nBytesWritten(0) {}

    uint8_t* pDest;
    size_t nBytesWritten;
};



cHTTPFile::cHTTPFile() : cZZFile()
{
    mpCurlShare = nullptr;
}

cHTTPFile::~cHTTPFile()
{
    cHTTPFile::Close();
}


static atomic<int64_t> gnLocks = 0;
static atomic<int64_t> gnUnlocks = 0;


void cHTTPFile::lock_cb(CURL* handle, curl_lock_data data, curl_lock_access access, void* userp)
{
    cHTTPFile* pFile = (cHTTPFile*)userp;
    gnLocks++;
    pFile->mCurlMutex.lock();

}

void cHTTPFile::unlock_cb(CURL* handle, curl_lock_data data, void* userp)
{
    cHTTPFile* pFile = (cHTTPFile*)userp;

    gnUnlocks++;

//    cout << "locks:" << gnLocks << "unlocks:" << gnUnlocks << "\n";
    pFile->mCurlMutex.unlock();
}


bool cHTTPFile::OpenInternal(string sURL, bool bWrite, string sName, string sPassword, bool bVerbose)       // todo maybe someday use real URI class
{
    mnLastError = kZZfileError_None;
    mbVerbose = bVerbose;
    if (bWrite)
    {
        mnLastError = kZZFileError_Unsupported;
        std::cerr << "cHTTPFile does not support writing.....yet........maybe ever." << std::endl;
        return false;
    }

    msURL = sURL;
    msName = sName;
    msPassword = sPassword;


    curl_global_init(CURL_GLOBAL_DEFAULT);

    mpCurlShare = curl_share_init();

    CURLSHcode sharecode = curl_share_setopt(mpCurlShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
    if (sharecode != 0)
    {
        std::cerr << "CURL Fail: curl_share_setopt(pShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE)\n";
        return false;
    }
    sharecode = curl_share_setopt(mpCurlShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
    if (sharecode != 0)
    {
        std::cerr << "CURL Fail: curl_share_setopt(pShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION )\n";
        return false;
    }
    sharecode = curl_share_setopt(mpCurlShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT);
    if (sharecode != 0)
    {
        std::cerr << "CURL Fail: curl_share_setopt(pShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT  )\n";
        return false;
    }
    
    sharecode = curl_share_setopt(mpCurlShare, CURLSHOPT_LOCKFUNC, lock_cb);
    if (sharecode != 0)
    {
        std::cerr << "CURL Fail: curl_share_setopt(pShare, CURLSHOPT_SHARE, CURLSHOPT_LOCKFUNC  )\n";
        return false;
    }

    sharecode = curl_share_setopt(mpCurlShare, CURLSHOPT_UNLOCKFUNC, unlock_cb);
    if (sharecode != 0)
    {
        std::cerr << "CURL Fail: curl_share_setopt(pShare, CURLSHOPT_SHARE, CURLSHOPT_UNLOCKFUNC  )\n";
        return false;
    }

    sharecode = curl_share_setopt(mpCurlShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT);
    if (sharecode != 0)
    {
        std::cerr << "CURL Fail: curl_share_setopt(pShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT  )\n";
        return false;
    }
    sharecode = curl_share_setopt(mpCurlShare, CURLSHOPT_USERDATA, this);
    if (sharecode != 0)
    {
        std::cerr << "CURL Fail: curl_share_setopt(pShare, CURLSHOPT_USERDATA, this  )\n";
        return false;
    }



    CURL* pCurl = curl_easy_init();
    if (!pCurl)
    {
        std::cerr << "Failed to create curl instance!\n";
        return false;
    }

    CURLcode res;
    curl_header* pHeader;

    curl_easy_setopt(pCurl, CURLOPT_URL, msURL.c_str());
    curl_easy_setopt(pCurl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(pCurl, CURLOPT_SHARE, mpCurlShare);
    curl_easy_setopt(pCurl, CURLOPT_VERBOSE, (int) mbVerbose);
//    curl_easy_setopt(pCurl, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(pCurl, CURLOPT_NOBODY, 1);

    if (gbSkipCertCheck)
    {
        curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYPEER, 0);
    }
    else
    {
        curl_easy_setopt(pCurl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
        //    curl_easy_setopt(pCurl, CURLOPT_SSLCERTTYPE, "PEM");
        //    curl_easy_setopt(pCurl, CURLOPT_CAINFO, "F:/dev/git/openssl-1.1.1k/certs/cacert-2022-07-19.pem");
    }


    res = curl_easy_perform(pCurl);
    if (res != CURLE_OK)
    {
        std::cerr << "curl failed for url:" << msURL << " response: " << curl_easy_strerror(res) << "\n";
        return false;
    }

    CURLHcode hRes = curl_easy_header(pCurl, "Content-Length", 0, CURLH_HEADER, -1, &pHeader);
    if (hRes != CURLE_OK)
    {
        std::cerr << "curl to Content-Length for url:" << msURL << " response: " << hRes << "\n";
        return false;
    }

    mnFileSize = (uint64_t)strtoull(pHeader->value, nullptr, 10);
    


    cout << "Opened HTTP server_host:\"" << msHost << "\"\n server_path:\"" << msPath << "\"\n";

    return true;
}

// Tracking stats
atomic<int64_t> gnTotalHTTPBytesRequested = 0;
atomic<int64_t> gnTotalRequestsIssued = 0;

bool cHTTPFile::Close()
{
    if (mbVerbose)
    {
        cout << "Total HTTP Requests:" << gnTotalRequestsIssued << "\n";
        cout << "Total HTTP bytes requested:" << gnTotalHTTPBytesRequested << "\n";
    }

    curl_share_cleanup(mpCurlShare);
    mpCurlShare = nullptr;
    curl_global_cleanup();

    return true;
}

size_t cHTTPFile::write_data(char* buffer, size_t size, size_t nitems, void* userp)
{
    HTTPFileResponse* pResponse = (HTTPFileResponse*)userp;
    memcpy(pResponse->pDest+pResponse->nBytesWritten, buffer, size * nitems);
    pResponse->nBytesWritten += size * nitems;

    return size * nitems;
}




bool cHTTPFile::Read(int64_t nOffset, uint32_t nBytes, uint8_t* pDestination, uint32_t& nBytesRead)
{
    mnLastError = kZZfileError_None;

    bool bUseCache = false;
    int64_t nOffsetToRequest = nOffset;
    uint32_t nBytesToRequest = nBytes;
    uint8_t* pBufferWrite = pDestination;
    shared_ptr<HTTPCacheLine> cacheLine;

    // 2022/9/17 - disabling http cache for now
    if (nBytesToRequest < kHTTPCacheLineSize && (uint64_t)nOffset + (uint64_t)kHTTPCacheLineSize < mnFileSize)
        bUseCache = true;



    if (bUseCache)
    {
        bool bNew = mCache.CheckOrReserve(nOffset, nBytes, cacheLine);           // If a cache line that would contain this data hasn't already been requested this will reserve one and return it
        if (!bNew)
        {
            cacheLine->Get(nOffset, nBytes, pDestination);      // this may block if the line is pending
            nBytesRead = nBytes;

            //cout << "HTTP Data read from cache...Requested:" << nBytes << "b at offset:" << nOffset << "\n";
            return true;
        }

        int64_t nUnfullfilledBytes = cacheLine->mUnfullfilledInterval.second - cacheLine->mUnfullfilledInterval.first;
        assert(!cacheLine->mbCommitted);

        nOffsetToRequest = cacheLine->mUnfullfilledInterval.first;
        nBytesToRequest = (uint32_t)nUnfullfilledBytes;
        int64_t nOffsetIntoCacheLine = nOffsetToRequest - cacheLine->mnBaseOffset;

        pBufferWrite = &cacheLine->mData[nOffsetIntoCacheLine];
    }

    if (nBytesToRequest > 0)
    {
        CURL* pCurl = curl_easy_init();


        HTTPFileResponse response;
        response.pDest = pBufferWrite;

        curl_easy_setopt(pCurl, CURLOPT_URL, msURL.c_str());
//        curl_easy_setopt(pCurl, CURLOPT_PROXY, "http://localhost:8888");

        curl_easy_setopt(pCurl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(pCurl, CURLOPT_VERBOSE, (int)mbVerbose);
        curl_easy_setopt(pCurl, CURLOPT_BUFFERSIZE, 512*1024);


        curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(pCurl, CURLOPT_SHARE, mpCurlShare);
        curl_easy_setopt(pCurl, CURLOPT_NOBODY, 0);
        curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, (void*) &response);

        if (gbSkipCertCheck)
        {
            curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYPEER, 0);
        }
        else
        {
            curl_easy_setopt(pCurl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
            //    curl_easy_setopt(pCurl, CURLOPT_SSLCERTTYPE, "PEM");
            //    curl_easy_setopt(pCurl, CURLOPT_CAINFO, "F:/dev/git/openssl-1.1.1k/certs/cacert-2022-07-19.pem");
        }


        stringstream ss;
        ss << nOffsetToRequest << "-" << nOffsetToRequest + nBytesToRequest - 1;
        curl_easy_setopt(pCurl, CURLOPT_RANGE, ss.str().c_str());


        uint64_t nStartTime = GetUSSinceEpoch();
        CURLcode res = curl_easy_perform(pCurl);
        uint64_t nEndTime = GetUSSinceEpoch();

        if (res != CURLE_OK)
        {
            std::cerr << "curl GET failed. Range:" << ss.str() << " url:" << msURL << " response: " << curl_easy_strerror(res) << "\n";
            return false;
        }

        /*
        size_t nKiB = nBytesToRequest / 1024;
        double fMS = (nEndTime - nStartTime) / 1000.0;

        double fRate = nKiB / (fMS/1000.0);
        cout << "CURL:" << nKiB << "KiB in " << fMS << "ms. Rate:" << fRate << "KiB/s\n";
        */


        gnTotalHTTPBytesRequested += nBytesToRequest;
        gnTotalRequestsIssued++;

        curl_easy_cleanup(pCurl);
    }

    // if the request can be cached
    if (bUseCache)
    {
        //cout << "committed " << nBytesToRequest << "b to cache offset:" << cacheLine->mnBaseOffset << ". Returning:" << nBytes << "\n";
        // cache the retrieved results
        memcpy(pDestination, cacheLine->mData, nBytes);
        cacheLine->Commit(nBytesToRequest);     // this commits the data to the cache line and frees any waiting requests on it
    }

    return true;
}

bool cHTTPFile::Write(int64_t, uint32_t, uint8_t*, uint32_t&)
{
    std::cerr << "cHTTPFile does not support writing.....yet........maybe ever." << std::endl;
    return false;
}

