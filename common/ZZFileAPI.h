//////////////////////////////////////////////////////////////////////////////////////////////////
// ZZFileAPI
// Purpose: An abstraction around local or HTTP files. 
//          ZZFile can open local files for reading or writing
//          cHTTPFile and cHTTPSFile can open remote files served by a web server for reading only.
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

using namespace std;

class cZZFile
{
public:

    const static int64_t ZZFILE_NO_SEEK  = -1;
    const static int64_t ZZFILE_SEEK_END = -2;

	cZZFile();
	~cZZFile();

	virtual bool	Open(string sURL, bool bWrite = false); 
	virtual bool	Close();
	virtual bool	Read(int64_t nOffset, uint32_t nBytes, uint8_t* pDestination, uint32_t& nBytesRead);
    virtual bool    Write(int64_t nOffset, uint32_t nBytes, uint8_t* pSource, uint32_t& nBytesWritten);

	uint64_t		GetFileSize() { return mnFileSize; }
    int64_t         GetLastError() { return mnLastError; }

	static cZZFile* FileFactory(const string& sURL = "");	// returns either a cHTTPFile* or a cHTTPSFile* depending on the url needs

protected:
	fstream			mFileStream;
	string			msPath;
	uint64_t		mnFileSize;
    int64_t         mnLastError;

	mutex			mMutex;
};


class cHTTPFile : public cZZFile
{
public:
    cHTTPFile();
    ~cHTTPFile();

    virtual bool Open(string sURL, bool bWrite);    // Writing not permitted
    virtual bool Close();
    virtual bool Read(int64_t nOffset, uint32_t nBytes, uint8_t* pDestination, uint32_t& nBytesRead);
    virtual bool Write(int64_t, uint32_t, uint8_t*, uint32_t&);    // not permitted

protected:

	bool ParseHeaders(list<std::string>& headersList);

    string msHost;

    boost::asio::io_service mIOService;
};

class cHTTPSFile : public cHTTPFile
{
public:
	virtual ~cHTTPSFile();
	virtual bool Open(string sURL, bool bWrite);
	virtual bool Close();
	virtual bool Read(int64_t nOffset, uint32_t nBytes, uint8_t* pDestination, uint32_t& nBytesRead);
    virtual bool Write(int64_t, uint32_t, uint8_t*, uint32_t&);    // not permitted

protected:
	atomic <boost::asio::ssl::context*> mpSSLContext;
};
