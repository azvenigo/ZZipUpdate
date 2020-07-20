//////////////////////////////////////////////////////////////////////////////////////////////////
// ZZFileAPI
// Purpose: An abstraction around local or HTTP files. 
//          ZZFile can open local files for reading or writing
//          cHTTPFile and cHTTPSFile can open remote files served by a web server for reading only.
//
// Usage:  Use the factory function cZZFile::Open to instantiate the appropriate subclass type
// Example: 
//                  shared_ptr<cZZFile> pInFile;
//                  bool bSuccess = cZZFile::Open( filename, ZZFILE_READ, pInFile);
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
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <fstream>
#include <atomic>
#include <mutex>
#include "HTTPCache.h"

using namespace std;

// Interface class
class cZZFile
{
public:

    const static int64_t ZZFILE_NO_SEEK = -1;
    const static int64_t ZZFILE_SEEK_END = -2;
    const static bool    ZZFILE_READ = false;
    const static bool    ZZFILE_WRITE = true;


    // Factory Construction
    // returns either a cZZFileLocal, cHTTPFile* or a cHTTPSFile* depending on the url needs
    static bool         Open(const string& sURL, bool bWrite, shared_ptr<cZZFile>& pFile, bool bVerbose = false);
    static bool         Open(const wstring& sURL, bool bWrite, shared_ptr<cZZFile>& pFile, bool bVerbose = false);	    // wstring version for convenience

    virtual             ~cZZFile() {};

    virtual bool	    Close() = 0;
    virtual bool	    Read(int64_t nOffset, uint32_t nBytes, uint8_t* pDestination, uint32_t& nBytesRead) = 0;
    virtual bool        Write(int64_t nOffset, uint32_t nBytes, uint8_t* pSource, uint32_t& nBytesWritten) = 0;

    virtual uint64_t    GetFileSize() { return mnFileSize; }
    virtual int64_t     GetLastError() { return mnLastError; }

protected:
    cZZFile();          // private constructor.... use cZZFile::Open factory function for construction

    virtual bool	    OpenInternal(string sURL, bool bWrite, bool bVerbose) = 0;
    string			    msPath;
    bool                mbVerbose;
    uint64_t		    mnFileSize;
    int64_t             mnLastError;
};


//////////////////////////////////////////////////////////////////////////////////////////
class cZZFileLocal : public cZZFile
{
    friend class cZZFile;
public:
    ~cZZFileLocal();

	virtual bool    Close();
	virtual bool    Read(int64_t nOffset, uint32_t nBytes, uint8_t* pDestination, uint32_t& nBytesRead);
    virtual bool    Write(int64_t nOffset, uint32_t nBytes, uint8_t* pSource, uint32_t& nBytesWritten);

protected:
    cZZFileLocal(); // private constructor.... use cZZFile::Open factory function for construction

    virtual bool    OpenInternal(string sURL, bool bWrite, bool bVerbose);

protected:
	fstream         mFileStream;
	mutex           mMutex;
};




//////////////////////////////////////////////////////////////////////////////////////////
class cHTTPResponseHeaders
{
public:

    cHTTPResponseHeaders();

    void Reset();

    bool Parse(std::istream& response_stream, bool bVerbose = false);

    bool GetString(const string& sNameIn, string& sValueOut);
    bool GetInt(const string& sNameIn, int64_t& nValueOut);
    bool Contains(const string& sNameIn);

    string      msHTTPVersion;
    uint32_t    mnHTTPStatusCode;
    string      msStatus;

private:
    bool ParseHeaders(list<std::string>& headersList);

    map<string, string> mNameToValueMap;

};

//////////////////////////////////////////////////////////////////////////////////////////
class cHTTPFile : public cZZFile
{
    friend class cZZFile;
public:
    ~cHTTPFile();

    virtual bool    Close();
    virtual bool    Read(int64_t nOffset, uint32_t nBytes, uint8_t* pDestination, uint32_t& nBytesRead);
    virtual bool    Write(int64_t, uint32_t, uint8_t*, uint32_t&);    // not permitted

protected:
    cHTTPFile();    // private constructor.... use cZZFile::Open factory function for construction

    virtual bool	OpenInternal(string sURL, bool bWrite, bool bVerbose);
    bool            ReadInternal(int64_t nOffset, uint32_t nBytes, uint8_t* pDestination, uint32_t& nBytesRead);
    bool            ReadInternalSSL(int64_t nOffset, uint32_t nBytes, uint8_t* pDestination, uint32_t& nBytesRead);

    string                              msURL;
    string                              msHost;

    boost::asio::io_service             mIOService;
    cHTTPResponseHeaders                mConnectionResponseHeaders;

    // HTTPS related
    bool                                mbHTTPSConnection;
    atomic <boost::asio::ssl::context*> mpSSLContext;

    HTTPCache                           mCache;

};
