// MIT License
// Copyright 2019 Alex Zvenigorodsky
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "ZZFileAPI.h"
#include <stdio.h>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <mutex>

using namespace std;
using namespace boost::asio;
using boost::asio::ip::tcp;

const string kHTTPTag("http://");
const string kHTTPSTag("https://");

enum
{
    kZZfileError_None           = 0,
    kZZFileError_Exception      = -5001,
    kZZFileError_General        = -5002,
    kZZFileError_Unsupported    = -5003,
    kZZFileError_URL_Error      = -5004,
    kZZFileError_No_SSL_Context = -5005
};


// Factory
bool cZZFile::Open(const string& sURL, bool bWrite, shared_ptr<cZZFile>& pFile)
{
    cZZFile* pNewFile = nullptr;
    if (sURL.substr(0, kHTTPSTag.length()) == kHTTPSTag)
    {
        pNewFile = new cHTTPSFile();
    }
    else if (sURL.substr(0, kHTTPTag.length()) == kHTTPTag)
    {
        pNewFile = new cHTTPFile();
    }
    else
    {
        pNewFile = new cZZFileLocal();
    }

    pFile.reset(pNewFile);
    return pNewFile->OpenInternal(sURL, bWrite);   // call protected virtualized Open
}

bool cZZFile::Open(const wstring& sURL, bool bWrite, shared_ptr<cZZFile>& pFile)
{
    return Open(string(sURL.begin(), sURL.end()), bWrite, pFile);
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

bool cZZFileLocal::OpenInternal(string sURL, bool bWrite)
{
    mnLastError = kZZfileError_None;

    if (bWrite)
        mFileStream.open(sURL, ios_base::in | ios_base::out | ios_base::binary | ios_base::trunc);
    else
        mFileStream.open(sURL, ios_base::in | ios_base::binary);

    if (mFileStream.fail())
    {
        mnLastError = errno;
        //		cerr << "Failed to open file:" << sURL.c_str() << "! Reason: " << errno << "\n";
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



cHTTPFile::cHTTPFile() : cZZFile()
{
}

cHTTPFile::~cHTTPFile()
{
    cHTTPFile::Close();
}

bool cHTTPFile::OpenInternal(string sURL, bool bWrite)       // todo maybe someday use real URI class
{
    if (bWrite)
    {
        mnLastError = kZZFileError_Unsupported;
        std::cerr << "cHTTPFile does not support writing.....yet........maybe ever." << std::endl;
        return false;
    }

    bool bShowHeaders = false;

    // Ensure this isn't an HTTPS request
    if (sURL.substr(0, kHTTPSTag.length()) == kHTTPSTag)
    {
        mnLastError = kZZFileError_URL_Error;
        std::cout << "Attempting to open SSL URL with cHTTP instead of cHTTPS.\n";
        return false;
    }

    sURL = sURL.substr(kHTTPTag.length());		// strip off the "http://"
    msHost = sURL.substr(0, sURL.find_first_of("/"));
    msPath = sURL.substr(msHost.length());

    // Resolve the domain name
    tcp::resolver resolver(mIOService);
    std::auto_ptr<tcp::resolver::query> pQuery(new tcp::resolver::query(msHost, "http"));
    tcp::resolver::iterator ipIterator = resolver.resolve(*pQuery);

    boost::asio::ip::tcp::socket sock(mIOService);
    boost::asio::connect(sock, ipIterator);

    // Retrieve headers
    try
    {
        // Form the request
        boost::asio::streambuf request;
        std::ostream request_stream(&request);

        request_stream << "GET " << msPath << " HTTP/1.1\r\n";
        request_stream << "Host: " << msHost << "\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Cache-Control: no-cache\r\n";
        request_stream << "Connection: close\r\n\r\n";
        request_stream.flush();

        // Fire it off
        boost::asio::write(sock, request);


        // read the first line
        boost::asio::streambuf response;
        boost::asio::read_until(sock, response, "\r\n");

        // check response
        std::istream response_stream(&response);
        std::string http_version;

        response_stream >> http_version;

        uint32_t nHTTPStatusCode = 0;
        response_stream >> nHTTPStatusCode;

        string sStatusString;
        getline(response_stream, sStatusString);

        if (!response_stream || http_version.substr(0, 5) != "HTTP/")
        {
            mnLastError = nHTTPStatusCode;
            cout << "Invalid response\n";
            return false;
        }
        if (nHTTPStatusCode != 200)
        {
            mnLastError = nHTTPStatusCode;
            cout << "Response Status Code " << nHTTPStatusCode << " message: \"" << sStatusString << "\"\n";
            return false;
        }

        // get headers
        boost::asio::read_until(sock, response, "\r\n\r\n");

        list<std::string> headersList;

        string sHeader;
        while (std::getline(response_stream, sHeader) && sHeader != "\r")
        {
            // strip off any trailing '\r'
            if (sHeader.substr(sHeader.length() - 1) == "\r")
                sHeader = sHeader.substr(0, sHeader.length() - 1);
            // strip off any trailing '\n'
            if (sHeader.substr(sHeader.length() - 1) == "\n")
                sHeader = sHeader.substr(0, sHeader.length() - 1);

            headersList.push_back(sHeader);
            if (bShowHeaders)
                cout << "header: \"" << sHeader << "\"\n";
        }

        ParseHeaders(headersList);      // extract whatever we need

    }
    catch (exception& e)
    {
        mnLastError = kZZFileError_Exception;
        cout << "Exception: " << e.what() << "\n";
        return false;
    }

    cout << "Opened HTTP server_host:\"" << msHost << "\"\n server_path:\"" << msPath << "\"\n filesize:" << mnFileSize << "\n";

    return true;
}

// Tracking stats
atomic<int64_t> gnTotalHTTPBytesRequested = 0;
atomic<int64_t> gnTotalRequestsIssued = 0;

bool cHTTPFile::Close()
{
    //cout << "Total HTTP Requests:" << gnTotalRequestsIssued << "\n";
    //cout << "Total HTTP bytes requested:" << gnTotalHTTPBytesRequested << "\n";

    return true;
}


bool cHTTPFile::Read(int64_t nOffset, uint32_t nBytes, uint8_t* pDestination, uint32_t& nBytesRead)
{
    mnLastError = kZZfileError_None;

    bool bUseCache = false;
    int64_t nOffsetToRequest = nOffset;
    uint32_t nBytesToRequest = nBytes;
    uint8_t* pBufferWrite = pDestination;
    shared_ptr<HTTPCacheLine> cacheLine;


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
        bool bShowHeaders = false;
        try
        {
            // Resolve the domain name
            tcp::resolver resolver(mIOService);
            tcp::resolver::query query(msHost, "http");
            tcp::resolver::iterator ipIterator = resolver.resolve(query);

            boost::asio::ip::tcp::socket sock(mIOService);
            boost::asio::connect(sock, ipIterator);


            // Form the request
            boost::asio::streambuf request;
            std::ostream request_stream(&request);

            request_stream << "GET " << msPath << " HTTP/1.1\r\n";
            request_stream << "Host: " << msHost << "\r\n";
            request_stream << "Accept: */*\r\n";
            request_stream << "Cache-Control: no-cache\r\n";
            request_stream << "Range: bytes=" << nOffsetToRequest << "-" << nOffsetToRequest + nBytesToRequest - 1 << "\r\n";
            request_stream << "Connection: close\r\n\r\n";
            request_stream.flush();

            // Fire it off
            boost::asio::write(sock, request);

            gnTotalHTTPBytesRequested += nBytesToRequest;
            gnTotalRequestsIssued++;

            // read the first line
            boost::asio::streambuf response;
            boost::asio::read_until(sock, response, "\r\n");

            // check response
            std::istream response_stream(&response);
            std::string http_version;

            response_stream >> http_version;

            uint32_t nHTTPStatusCode = 0;
            response_stream >> nHTTPStatusCode;

            string sStatusString;
            getline(response_stream, sStatusString);

            if (!response_stream || http_version.substr(0, 5) != "HTTP/")
            {
                mnLastError = nHTTPStatusCode;
                cout << "Invalid response\n";
                return false;
            }
            if (!(nHTTPStatusCode == 200 || nHTTPStatusCode == 206))
            {
                mnLastError = nHTTPStatusCode;
                cout << "Response Status Code " << nHTTPStatusCode << " message: \"" << sStatusString << "\"\n";
                return false;
            }

            // get headers
            boost::asio::read_until(sock, response, "\r\n\r\n");

            list<std::string> headersList;

            string sHeader;
            while (std::getline(response_stream, sHeader) && sHeader != "\r")
            {
                // strip off any trailing '\r'
                if (sHeader.substr(sHeader.length() - 1) == "\r")
                    sHeader = sHeader.substr(0, sHeader.length() - 1);
                // strip off any trailing '\n'
                if (sHeader.substr(sHeader.length() - 1) == "\n")
                    sHeader = sHeader.substr(0, sHeader.length() - 1);


                headersList.push_back(sHeader);
                if (bShowHeaders)
                    cout << "header: \"" << sHeader << "\"\n";
            }


            {

                std::string responseString;

                // Now read the data
                int32_t nBytesLeftToRead = (int32_t)nBytesToRequest;
                response_stream.read((char*)pBufferWrite, nBytesToRequest);    // copy as many bytes as we can to the destination buffer
                nBytesRead = (uint32_t)response_stream.gcount();  // get how many bytes of file data we've read so far
                nBytesLeftToRead -= nBytesRead;
                pBufferWrite += nBytesRead;     // advance the pointer to the write buffer

                char buf[1024];
                boost::system::error_code ec;
                do {
                    size_t bytesTransferred = sock.receive(boost::asio::buffer(buf), {}, ec);
                    if (!ec) responseString.append(buf, buf + bytesTransferred);
                } while (!ec);

                int32_t nResponseStringLength = (int32_t)responseString.length();
                if (nBytesLeftToRead == nResponseStringLength)
                {
                    memcpy(pBufferWrite, responseString.data(), responseString.length());
                    nBytesRead = nBytesToRequest;
                }
                else
                {
                    mnLastError = ec.value();
                    cout << "Didn't get right number of bytes!\n";
                    return false;
                }
            }
        }
        catch (exception& e)
        {
            mnLastError = kZZFileError_Exception;
            cout << "Exception: " << e.what() << "\n";

            return false;
        }
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


bool cHTTPFile::ParseHeaders(list<std::string>& headersList)
{
    for (list<std::string>::iterator it = headersList.begin(); it != headersList.end(); it++)
    {
        std::string sHeader(*it);

        // add headers to look for here
        const std::string ksContentLengthTag("Content-Length: ");
        const std::string ksContentTypeTag("Content-Type: ");

        if (sHeader.substr(0, ksContentLengthTag.length()) == ksContentLengthTag)
        {
            std::string sValue = sHeader.substr(ksContentLengthTag.length());        // get everything after the tag

            mnFileSize = boost::lexical_cast<uint64_t> (sValue);
        }
        else if (sHeader.substr(0, ksContentTypeTag.length()) == ksContentTypeTag)
        {

        }
    }

    return true;
}


cHTTPSFile::cHTTPSFile() : cHTTPFile()
{
}

cHTTPSFile::~cHTTPSFile()
{
    delete mpSSLContext;
}


bool cHTTPSFile::OpenInternal(string sURL, bool bWrite)
{
    mnLastError = kZZfileError_None;
    if (bWrite)
    {
        mnLastError = kZZFileError_Unsupported;
        std::cerr << "cHTTPSFile does not support writing.....yet........maybe ever." << std::endl;
        return false;
    }

    bool bShowHeaders = false;

    // Strip off "http://" if needed
    if (sURL.substr(0, kHTTPTag.length()) == kHTTPSTag)
    {
        mnLastError = kZZFileError_URL_Error;
        std::cout << "Attempting to open non-SSL URL with cHTTPS instead of cHTTP.\n";
        return false;
    }

    sURL = sURL.substr(kHTTPSTag.length());		// strip off the "https://"
    msHost = sURL.substr(0, sURL.find_first_of("/"));
    msPath = sURL.substr(msHost.length());

    mpSSLContext = new ssl::context(boost::asio::ssl::context::sslv23);
    auto_ptr<ssl::stream<ip::tcp::socket>> pSock(new ssl::stream<ip::tcp::socket>(mIOService, *mpSSLContext));

    // Resolve the domain name
    tcp::resolver resolver(mIOService);
    std::auto_ptr<tcp::resolver::query> pQuery(new tcp::resolver::query(msHost, "https"));
    tcp::resolver::iterator ipIterator = resolver.resolve(*pQuery);
    boost::asio::connect(pSock->lowest_layer(), ipIterator);

    pSock->handshake(ssl::stream_base::handshake_type::client);


    // Retrieve headers
    try
    {
        // Form the request
        boost::asio::streambuf request;
        std::ostream request_stream(&request);

        request_stream << "GET " << msPath << " HTTP/1.1\r\n";
        request_stream << "Host: " << msHost << "\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Cache-Control: no-cache\r\n";
        request_stream << "Connection: close\r\n\r\n";
        request_stream.flush();

        // Fire it off
        boost::asio::write(*pSock, request);


        // read the first line
        boost::asio::streambuf response;
        boost::asio::read_until(*pSock, response, "\r\n");

        // check response
        std::istream response_stream(&response);
        std::string http_version;

        response_stream >> http_version;

        uint32_t nHTTPStatusCode = 0;
        response_stream >> nHTTPStatusCode;

        string sStatusString;
        getline(response_stream, sStatusString);

        if (!response_stream || http_version.substr(0, 5) != "HTTP/")
        {
            mnLastError = nHTTPStatusCode;
            cout << "Invalid response\n";
            return false;
        }
        if (nHTTPStatusCode != 200)
        {
            mnLastError = nHTTPStatusCode;
            cout << "Response Status Code " << nHTTPStatusCode << " message: \"" << sStatusString << "\"\n";
            return false;
        }

        // get headers
        boost::asio::read_until(*pSock, response, "\r\n\r\n");

        list<std::string> headersList;

        string sHeader;
        while (std::getline(response_stream, sHeader) && sHeader != "\r")
        {
            // strip off any trailing '\r'
            if (sHeader.substr(sHeader.length() - 1) == "\r")
                sHeader = sHeader.substr(0, sHeader.length() - 1);
            // strip off any trailing '\n'
            if (sHeader.substr(sHeader.length() - 1) == "\n")
                sHeader = sHeader.substr(0, sHeader.length() - 1);

            headersList.push_back(sHeader);
            if (bShowHeaders)
                cout << "header: \"" << sHeader << "\"\n";
        }

        ParseHeaders(headersList);      // extract whatever we need

    }
    catch (exception& e)
    {
        mnLastError = kZZFileError_Exception;
        cout << "Exception: " << e.what() << "\n";
        return false;
    }

    cout << "Opened HTTPS host:\"" << msHost << "\"\n path:\"" << msPath << "\"\n filesize:" << mnFileSize << "\n";

    return true;
}

bool cHTTPSFile::Read(int64_t nOffset, uint32_t nBytes, uint8_t* pDestination, uint32_t& nBytesRead)
{
    mnLastError = kZZfileError_None;

    bool bUseCache = false;
    int64_t nOffsetToRequest = nOffset;
    uint32_t nBytesToRequest = nBytes;
    uint8_t* pBufferWrite = pDestination;
    shared_ptr<HTTPCacheLine> cacheLine;


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
        bool bShowHeaders = false;

        try
        {
            if (!mpSSLContext)
            {
                mnLastError = kZZFileError_No_SSL_Context;
                cout << "cHTTPSFile::Read No SSLContext!\n";
                return false;
            }

            auto_ptr<ssl::stream<ip::tcp::socket>> pSock(new ssl::stream<ip::tcp::socket>(mIOService, *mpSSLContext));

            // Resolve the domain name
            tcp::resolver resolver(mIOService);
            std::auto_ptr<tcp::resolver::query> pQuery(new tcp::resolver::query(msHost, "https"));
            tcp::resolver::iterator ipIterator = resolver.resolve(*pQuery);
            boost::asio::connect(pSock->lowest_layer(), ipIterator);

            pSock->handshake(ssl::stream_base::handshake_type::client);


            // Form the request
            boost::asio::streambuf request;
            std::ostream request_stream(&request);

            request_stream << "GET " << msPath << " HTTP/1.1\r\n";
            request_stream << "Host: " << msHost << "\r\n";
            request_stream << "Accept: */*\r\n";
            request_stream << "Cache-Control: no-cache\r\n";
            request_stream << "Range: bytes=" << nOffsetToRequest << "-" << nOffsetToRequest + nBytesToRequest - 1 << "\r\n";
            request_stream << "Connection: close\r\n\r\n";
            request_stream.flush();

            // Fire it off
            boost::asio::write(*pSock, request);

            gnTotalHTTPBytesRequested += nBytesToRequest;
            gnTotalRequestsIssued++;


            // read the first line
            boost::asio::streambuf response;
            boost::asio::read_until(*pSock, response, "\r\n");

            // check response
            std::istream response_stream(&response);
            std::string http_version;

            response_stream >> http_version;

            uint32_t nHTTPStatusCode = 0;
            response_stream >> nHTTPStatusCode;

            string sStatusString;
            getline(response_stream, sStatusString);

            if (!response_stream || http_version.substr(0, 5) != "HTTP/")
            {
                mnLastError = nHTTPStatusCode;
                cout << "Invalid response\n";
                return false;
            }
            if (!(nHTTPStatusCode == 200 || nHTTPStatusCode == 206))
            {
                mnLastError = nHTTPStatusCode;
                cout << "Response Status Code " << nHTTPStatusCode << " message: \"" << sStatusString << "\"\n";
                return false;
            }

            // get headers
            boost::asio::read_until(*pSock, response, "\r\n\r\n");

            list<std::string> headersList;

            string sHeader;
            while (std::getline(response_stream, sHeader) && sHeader != "\r")
            {
                // strip off any trailing '\r'
                if (sHeader.substr(sHeader.length() - 1) == "\r")
                    sHeader = sHeader.substr(0, sHeader.length() - 1);
                // strip off any trailing '\n'
                if (sHeader.substr(sHeader.length() - 1) == "\n")
                    sHeader = sHeader.substr(0, sHeader.length() - 1);


                headersList.push_back(sHeader);
                if (bShowHeaders)
                    cout << "header: \"" << sHeader << "\"\n";
            }


            {

                std::string responseString;

                // Now read the data
                int32_t nBytesLeftToRead = (int32_t)nBytesToRequest;
                response_stream.read((char*)pBufferWrite, nBytesToRequest);    // copy as many bytes as we can to the destination buffer
                nBytesRead = (uint32_t)response_stream.gcount();  // get how many bytes of file data we've read so far
                nBytesLeftToRead -= nBytesRead;
                pBufferWrite += nBytesRead;     // advance the pointer to the write buffer

                char buf[1024];
                boost::system::error_code ec;
                do {
                    size_t bytesTransferred = pSock->read_some(boost::asio::buffer(buf), ec);
                    if (!ec) responseString.append(buf, buf + bytesTransferred);
                } while (!ec);

                int32_t nResponseStringLength = (int32_t)responseString.length();
                if (nBytesLeftToRead == nResponseStringLength)
                {
                    memcpy(pBufferWrite, responseString.data(), responseString.length());
                    nBytesRead = nBytesToRequest;
                }
                else
                {
                    mnLastError = ec.value();
                    cout << "Didn't get right number of bytes!\n";
                    return false;
                }
            }
        }
        catch (exception& e)
        {
            mnLastError = kZZFileError_Exception;
            cout << "Exception: " << e.what() << "\n";

            return false;
        }
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

bool cHTTPSFile::Write(int64_t, uint32_t, uint8_t*, uint32_t&)
{
    mnLastError = kZZFileError_Unsupported;
    std::cerr << "cHTTPSFile does not support writing.....yet........maybe ever." << std::endl;
    return false;
}

bool cHTTPSFile::Close()
{
    delete mpSSLContext;
    mpSSLContext = NULL;
    return true;
}
