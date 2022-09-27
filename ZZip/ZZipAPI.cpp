// MIT License
// Copyright 2019 Alex Zvenigorodsky
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "ZZipAPI.h"
#include <iostream>
#include <iomanip>
#include "zlibAPI.h"
#include "common/FNMatch.h"
#include <filesystem>
#include <time.h>
#include <ctime>
#include <chrono>
//#include <boost/lexical_cast.hpp>
#include "common/CrC32Fast.h"


using namespace std;


/*template <typename TP>
std::time_t to_time_t(TP tp)
{
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now()
        + system_clock::now());
    return system_clock::to_time_t(sctp);
}*/

uint16_t zip_date_from_std_time(time_t& tt)
{
    // format
    // Bits 00 - 04: day
    // Bits 05 - 08: month
    // Bits 09-15: years from 1980

    tm* now = localtime(&tt);


    uint16_t nDay = (uint16_t)now->tm_mday;
    uint16_t nMonth = (uint16_t)now->tm_mon;
    uint16_t nYear = (uint16_t)now->tm_year;

    return nDay | nMonth << 5 | nYear << 9;
}

uint16_t zip_time_from_std_time(time_t& tt)
{
    // format
    // Bits 00 - 04: seconds divided by 2
    // Bits 05 - 10 : minute
    // Bits 11 - 15 : hour

    tm* now = localtime(&tt);

    uint16_t nSecs = (uint16_t)(now->tm_sec / 2);
    uint16_t nMins = (uint16_t)now->tm_min;
    uint16_t nHour = (uint16_t)now->tm_hour;

    return nSecs | nMins << 5 | nHour << 11;
}

ZZipAPI::ZZipAPI() : mnCompressionLevel(0)
{
    mbInitted = false;
}

ZZipAPI::~ZZipAPI()
{
    Shutdown();
}

bool ZZipAPI::Init(const string& sZipURL, eOpenType type, int32_t nCompressionLevel, const string& sName, const string& sPassword)
{
    if (mbInitted)
    {
        cout << "ZZipAPI already open!  Cannot Reinitialize.\n";
        return false;
    }


    mOpenType = type;
    msZipURL = sZipURL;
    msName = sName;
    msPassword = sPassword;
    mnCompressionLevel = nCompressionLevel;

    if (mOpenType == kZipOpen)
        mbInitted = OpenForReading();
    else
        mbInitted = CreateZipFile();

    return mbInitted;
}

bool ZZipAPI::Shutdown()
{
    if (mbInitted)
    {
        // If we're creating a new archive then write out the CD
        if (mOpenType == ZZipAPI::kZipCreate)
        {
            std::streampos nStartOfCDOffset = mpZZFile->GetFileSize();

            mZipCD.ComputeCDRecords((uint64_t)nStartOfCDOffset);
            bool bSuccess = mZipCD.Write(*mpZZFile);

            if (!bSuccess)
            {
                cerr << "ZZipAPI::Shutdown - Failure to write Central Directory Headers!\n";
            }
        }

        mpZZFile->Close();
        msZipURL.clear();

        mbInitted = false;
    }

    return true;
}


bool ZZipAPI::OpenForReading()
{
    if (!cZZFile::Open(msZipURL, cZZFile::ZZFILE_READ, mpZZFile, msName, msPassword))
    {
        std::cerr << "Couldn't open file for reading: \"" << msZipURL << "\"\n";
        return false;
    }

    //cout << "Parsing \"" << msZipURL.c_str() << "\"\n";

    mZipCD.Init(*mpZZFile);


    return true;
}

bool ZZipAPI::CreateZipFile()
{
    if (!cZZFile::Open(msZipURL, cZZFile::ZZFILE_WRITE, mpZZFile))
    {
        cerr << "Couldn't open file for writing \"" << msZipURL << "\"!\n";
        return false;
    }

    return true;
}


void ZZipAPI::DumpReport(const string& sOutputFilename)
{
    std::fstream outFile;
    outFile.open(sOutputFilename, ios_base::out | ios_base::trunc);
    if (outFile.fail())
    {
        cout << "Failed to open " << sOutputFilename.c_str() << " for report.\n";
        return;
    }
    outFile << "<html><body>";
    outFile << "Report on " << msZipURL << "<br>";

    outFile << "<br>";

    outFile << "<br><table border='1'>";
    outFile << "<tr><td>TotalFiles</td><td>" << mZipCD.GetNumTotalFiles() << "</td></tr>";
    outFile << "<tr><td>TotalFolders</td><td>" << mZipCD.GetNumTotalFolders() << "</td></tr>";
    outFile << "<tr><td>TotalCompressedBytes</td><td>" << mZipCD.GetTotalCompressedBytes() << "</td></tr>";
    outFile << "<tr><td>TotalUncompressedBytes</td><td>" << mZipCD.GetTotalUncompressedBytes() << "</td></tr>";

    uint64_t nTotalK = mZipCD.GetTotalUncompressedBytes() / 1024;
    uint64_t nSavingsInK = nTotalK - (mZipCD.GetTotalCompressedBytes() / 1024);
    float fPercent = ((float)nSavingsInK / (float)nTotalK) * 100.0f;

    outFile << "<tr><td>Percent Compression</td><td>" << setprecision(2) << fPercent << "</td></tr>";
    outFile << "</table>";


    outFile << "EndOfCDRecord";
    outFile << "<table border='1'>";
    outFile << mZipCD.mEndOfCDRecord.FieldNames().c_str();
    outFile << mZipCD.mEndOfCDRecord.ToString().c_str();
    outFile << "</table>";

    outFile << "<br>";

    outFile << "\nIs64Bit=" << (mZipCD.mbIsZip64 ? "true" : "false") << "<br>";
    if (mZipCD.mbIsZip64)
    {
        outFile << "Zip64EndOfCDLocator";
        outFile << "<table border='1'>";
        outFile << mZipCD.mZip64EndOfCDLocator.FieldNames().c_str();
        outFile << mZipCD.mZip64EndOfCDLocator.ToString().c_str();
        outFile << "</table>";

        outFile << "<br>";

        outFile << "Zip64EndOfCDRecord";
        outFile << "<table border='1'>";
        outFile << mZipCD.mZip64EndOfCDRecord.FieldNames().c_str() << "\n";
        outFile << mZipCD.mZip64EndOfCDRecord.ToString().c_str() << "\n";
        outFile << "</table>";
    }

    mZipCD.DumpCD(outFile, "*", true, eToStringFormat::kHTML);

    outFile << "</html>";

    outFile.close();
}



bool ZZipAPI::ExtractRawStream(const string& sFilename, const string& sOutputFilename, Progress* pProgress)
{
    if (!mbInitted)
        return false;

    cCDFileHeader cdFileHeader;
    if (!mZipCD.GetFileHeader(sFilename, cdFileHeader))
        return false;

    cLocalFileHeader localFileHeader;

    uint32_t nHeaderBytesProcessed = 0;
    if (!localFileHeader.Read(*mpZZFile, cdFileHeader.mLocalFileHeaderOffset, nHeaderBytesProcessed))
    {
        cerr << "Failed to read localFileHeader.\n";
        return false;
    }

    const uint32_t kSize = 16*1024 * 1024;  
    uint8_t* pStream = new uint8_t[kSize];

    shared_ptr<cZZFile> pOutFile;
    if (!cZZFile::Open(sOutputFilename, cZZFile::ZZFILE_WRITE, pOutFile))
    {
        delete[] pStream;
        cout << "Failed to open " << sOutputFilename.c_str() << " for extraction. Reason: " << pOutFile->GetLastError() << "\n";
        return false;
    }

    uint64_t nBytesProcessed = 0;
    while (nBytesProcessed < cdFileHeader.mCompressedSize)
    {
        uint64_t nReadOffset = cdFileHeader.mLocalFileHeaderOffset + nHeaderBytesProcessed + nBytesProcessed;

        // Either grab another full block of compressed data or adjust down to the remainder of the compressed stream
        uint64_t nBytesToProcess = kSize;
        if (nBytesProcessed + nBytesToProcess > cdFileHeader.mCompressedSize)
            nBytesToProcess = cdFileHeader.mCompressedSize - nBytesProcessed;


        uint32_t nBytesRead = 0;
        if (!mpZZFile->Read(nReadOffset, (uint32_t)nBytesToProcess, pStream, nBytesRead))
        {
            delete[] pStream;
            cerr << "Failed to read stream for file " << sFilename.c_str() << " at offset " << cdFileHeader.mLocalFileHeaderOffset + nHeaderBytesProcessed + nBytesProcessed << ". Tried to read " << nBytesToProcess << " bytes. Total compressed stream size: " << cdFileHeader.mCompressedSize << "\n";
            return false;
        }

        uint32_t nBytesWritten = 0;
        if (!pOutFile->Write(cZZFile::ZZFILE_NO_SEEK, (uint32_t) nBytesToProcess, pStream, nBytesWritten))
        {
            delete[] pStream;
            cerr << "Failed to seek to write stream for file " << sFilename.c_str() << " to file " << sOutputFilename.c_str() << ".  Reason: " << errno << "\n";
            return false;
        }

        nBytesProcessed += nBytesToProcess;
        if (pProgress)
            pProgress->AddBytesProcessed(nBytesToProcess);
    }

    delete[] pStream;

    //cout << "Extracted \"" << sFilename.c_str() << "\" to \"" << sOutputFilename.c_str() << "\"\n";

    return true;
}

bool ZZipAPI::DecompressToFile(const string& sFilename, const string& sOutputFilename, Progress* pProgress)
{
    if (!mbInitted)
        return false;

    cCDFileHeader cdFileHeader;
    if (!mZipCD.GetFileHeader(sFilename, cdFileHeader))
        return false;

    cLocalFileHeader localFileHeader;

    uint32_t nHeaderBytesProcessed = 0;
    if (!localFileHeader.Read(*mpZZFile, cdFileHeader.mLocalFileHeaderOffset, nHeaderBytesProcessed))
    {
        cerr << "Failed to read localFileHeader.\n";
        return false;
    }

    // If the file is uncompressed just extract it
    if (localFileHeader.mCompressionMethod == 0)
    {
        return ExtractRawStream(sFilename, sOutputFilename);
    }
    else if (localFileHeader.mCompressionMethod != 8)
    {
        cerr << "Unsupported compression method: " << localFileHeader.mCompressionMethod;
        return false;
    }

    const uint32_t kCompressStreamProcessSize = 1024 * 1024;  // one meg at a time

    uint8_t* pCompStream = new uint8_t[kCompressStreamProcessSize];

    ZDecompressor decompressor;
    decompressor.Init();

    shared_ptr<cZZFile> pOutFile;
    if (!cZZFile::Open(sOutputFilename, cZZFile::ZZFILE_WRITE, pOutFile))
    {
        delete[] pCompStream;
        cout << "Failed to open " << sOutputFilename.c_str() << " for extraction. Reason: " << errno << "\n";
        return false;
    }


    uint64_t nCompressedBytesProcessed = 0;
    while (nCompressedBytesProcessed < cdFileHeader.mCompressedSize)
    {
        uint64_t nReadOffset = cdFileHeader.mLocalFileHeaderOffset + nHeaderBytesProcessed + nCompressedBytesProcessed;

        // Either grab another full block of compressed data or adjust down to the remainder of the compressed stream
        uint64_t nBytesToProcess = kCompressStreamProcessSize;
        if (nCompressedBytesProcessed + nBytesToProcess > cdFileHeader.mCompressedSize)
            nBytesToProcess = cdFileHeader.mCompressedSize - nCompressedBytesProcessed;

        uint32_t nBytesRead = 0;

        if (!mpZZFile->Read(nReadOffset, (uint32_t)nBytesToProcess, pCompStream, nBytesRead))
        {
            delete[] pCompStream;
            cerr << "Failed to read compression stream for file " << sFilename.c_str() << " at offset " << cdFileHeader.mLocalFileHeaderOffset + nHeaderBytesProcessed + nCompressedBytesProcessed << ". Tried to read " << nBytesToProcess << " bytes. Total compressed stream size: " << cdFileHeader.mCompressedSize << "\n";
            return false;
        }

        decompressor.InitStream(pCompStream, (uint32_t)nBytesToProcess);
        int32_t nStatus = Z_OK;
        int32_t nOutIndex = 0;
        while (decompressor.HasMoreOutput())
        {
            if (nStatus == Z_OK || nStatus == Z_STREAM_END)
            {
                nStatus = decompressor.Decompress();
                uint32_t nDecompressedBytes = (uint32_t)decompressor.GetDecompressedBytes();
                uint32_t nBytesWritten = 0;
                if (!pOutFile->Write(cZZFile::ZZFILE_NO_SEEK, (uint32_t) decompressor.GetDecompressedBytes(), decompressor.GetDecompressedBuffer(), nBytesWritten))
                {
                    delete[] pCompStream;
                    cerr << "Failed to seek to write decompressed stream for file " << sFilename.c_str() << " to file " << sOutputFilename.c_str() << ".  Reason: " << pOutFile->GetLastError() << "\n";
                    return false;
                }

                nOutIndex += nDecompressedBytes;

                if (pProgress)
                    pProgress->AddBytesProcessed(nDecompressedBytes);
            }

            if (nStatus < 0)
            {
                break;
            }
        }

        if (!(nStatus == Z_STREAM_END || nStatus == Z_OK))
        {
            delete[] pCompStream;
            cerr << "Decompress Error #:" << to_string(nStatus) << "\n";
            return false;
        }

        nCompressedBytesProcessed += nBytesToProcess;
    }

    delete[] pCompStream;

    //cout << "thread: " << this_thread::get_id() << " Extracted \"" << sFilename.c_str() << "\" to \"" << sOutputFilename.c_str() << "\"\n";

    return true;
}

template <typename TP>
std::time_t to_time_t(TP tp)
{
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now()
        + system_clock::now());
    return system_clock::to_time_t(sctp);
}



bool ZZipAPI::AddToZipFile(const string& sFilename, const string& sBaseFolder, Progress* pProgress)
{
    // Precondition:  mZipFile seek offset should be set to where the new file should be added.

    if (!mbInitted)
    {
        cout << "AddToZipFile - Not Initialized!\n";
        return false;
    }

    if (mOpenType != kZipCreate)
    {
        cout << "AddToZipFile - ZZipAPI not open for creation!\n";
        return false;
    }

    bool bInputIsFile = filesystem::is_regular_file(sFilename);

    string sFileOrFolder(sFilename);
    std::replace(sFileOrFolder.begin(), sFileOrFolder.end(), '\\', '/');    // Only forward slashes

    if (!bInputIsFile)
    {
        // If this is a folder and it doesn't end in '/' then append it
        if (sFileOrFolder[sFileOrFolder.length() - 1] != '/')
            sFileOrFolder.append("/");
    }


    std::streampos nOffsetToLocalFileHeader = mpZZFile->GetFileSize();

    /////////////////////////////////////////////////
    // Fill in header info
    cLocalFileHeader newLocalHeader;
    shared_ptr<cZZFile> pInFile;
    if (bInputIsFile)
    {
        if (!cZZFile::Open(sFileOrFolder, cZZFile::ZZFILE_READ, pInFile))
        {
            cout << "Failed to open " << sFileOrFolder.c_str() << " for compression. Reason: " << pInFile->GetLastError() << "\n";
            return false;
        }

        newLocalHeader.mUncompressedSize = pInFile->GetFileSize();

        // Date and Time
        filesystem::file_time_type fileTime = filesystem::last_write_time(sFileOrFolder);


        time_t tt = to_time_t(fileTime);

        newLocalHeader.mLastModificationDate = zip_date_from_std_time(tt);
        newLocalHeader.mLastModificationTime = zip_time_from_std_time(tt);
        if (newLocalHeader.mUncompressedSize > 0) // TBD add uncompressed?
            newLocalHeader.mCompressionMethod = 8;  // only deflate supported. 
    }

    newLocalHeader.mMinVersionToExtract = kDefaultMinVersionToExtract;
    newLocalHeader.mGeneralPurposeBitFlag = kDefaultGeneralPurposeFlag;

    uint16_t nRelativeLength = (uint16_t)(sFileOrFolder.length() - sBaseFolder.length());

    // relative path
    newLocalHeader.mFilename = sFileOrFolder.substr(sFileOrFolder.length() - nRelativeLength, nRelativeLength);
    newLocalHeader.mFilenameLength = nRelativeLength;


    uint64_t nOffsetOfStreamData = ((uint64_t)nOffsetToLocalFileHeader) + cLocalFileHeader::kStaticDataSize + newLocalHeader.mFilenameLength + cLocalFileHeader::kExtendedFieldLength;

    if (bInputIsFile)
    {
        const uint32_t kStreamProcessSize = 1024 * 1024;  // one meg at a time
        uint8_t* pStream = new uint8_t[kStreamProcessSize];

        ZCompressor compressor;
        compressor.Init(mnCompressionLevel);

        uint32_t nCRC = 0;
        uint64_t nBytesProcessed = 0;
        while (nBytesProcessed < pInFile->GetFileSize())
        {
            // Either grab another full block of compressed data or adjust down to the remainder of the compressed stream
            uint64_t nBytesToProcess = kStreamProcessSize;
            if (nBytesProcessed + nBytesToProcess > pInFile->GetFileSize())
                nBytesToProcess = pInFile->GetFileSize() - nBytesProcessed;

            uint32_t nBytesRead = 0;
            if (!pInFile->Read(cZZFile::ZZFILE_NO_SEEK, (uint32_t) nBytesToProcess, pStream, nBytesRead))
            {
                delete[] pStream;
                cerr << "Failed to read input stream for file " << sFileOrFolder.c_str() << " at offset " << nBytesProcessed << ". Tried to read " << nBytesToProcess << " bytes. Total file size: " << pInFile->GetFileSize() << "\n";
                return false;
            }

            // Update our CRC calculation
            nCRC = crc32_16bytes(pStream, (int32_t) nBytesToProcess, nCRC);

            compressor.InitStream(pStream, (uint32_t)nBytesToProcess);
            int32_t nStatus = Z_OK;
            int32_t nOutIndex = 0;
            while (compressor.HasMoreOutput())
            {
                if (nStatus == Z_OK)
                {
                    bool bFinalBlock = (nBytesProcessed + nBytesToProcess == pInFile->GetFileSize());
                    nStatus = compressor.Compress(bFinalBlock);
                    uint32_t nCompressedBytes = (uint32_t)compressor.GetCompressedBytes();

                    uint32_t nNumWritten = 0;
                    if (!mpZZFile->Write(nOffsetOfStreamData, (uint32_t)compressor.GetCompressedBytes(), compressor.GetCompressedBuffer(), nNumWritten))
                    {
                        delete[] pStream;
                        cerr << "Failed to write compressed stream for file " << sFileOrFolder.c_str() << " to file " << msZipURL.c_str() << ".  Reason: " << errno << "\n";
                        return false;
                    }

                    nOffsetOfStreamData += nCompressedBytes;
                    nOutIndex += nCompressedBytes;
                    newLocalHeader.mCompressedSize += nCompressedBytes;
                }

                if (nStatus != Z_OK)
                {
                    break;
                }
            }

            if (!(nStatus == Z_OK || nStatus == Z_STREAM_END))
            {
                delete[] pStream;
                cerr << "Compress Error #:" << to_string(nStatus) << "\n";
                return false;
            }

            nBytesProcessed += nBytesToProcess;

            if (pProgress)
                pProgress->AddBytesProcessed(nBytesToProcess);
        }

        delete[] pStream;

        // Now write the localfile header
        newLocalHeader.mCRC32 = nCRC;
    }

    // seek to start of compression stream data
    if (!newLocalHeader.Write(*mpZZFile, nOffsetToLocalFileHeader))
    {
        return false;
    }

    // Add a new CD entry
    cCDFileHeader newCDFileHeader;
    newCDFileHeader.mLastModificationTime = newLocalHeader.mLastModificationTime;
    newCDFileHeader.mLastModificationDate = newLocalHeader.mLastModificationDate;
    newCDFileHeader.mCRC32 = newLocalHeader.mCRC32;
    newCDFileHeader.mCompressionMethod = newLocalHeader.mCompressionMethod;
    newCDFileHeader.mCompressedSize = newLocalHeader.mCompressedSize;
    newCDFileHeader.mUncompressedSize = newLocalHeader.mUncompressedSize;
    newCDFileHeader.mLocalFileHeaderOffset = nOffsetToLocalFileHeader;
    newCDFileHeader.mFileName = newLocalHeader.mFilename;
    newCDFileHeader.mFilenameLength = newLocalHeader.mFilenameLength;

    mZipCD.mCDFileHeaderList.push_back(newCDFileHeader);

    //    cout << "thread: " << this_thread::get_id() << " Added \"" << sFileOrFolder.c_str() << "\"\n";

    return true;
}

bool ZZipAPI::AddToZipFileFromBuffer(uint8_t* pInputBuffer, uint32_t nInputBufferSize, const string& sFilename, Progress* pProgress)
{
    // Precondition:  mZipFile seek offset should be set to where the new file should be added.

    if (!mbInitted)
    {
        cout << "AddToZipFile - Not Initialized!\n";
        return false;
    }

    if (mOpenType != kZipCreate)
    {
        cout << "AddToZipFile - ZZipAPI not open for creation!\n";
        return false;
    }

    std::streampos nOffsetToLocalFileHeader = mpZZFile->GetFileSize();

    /////////////////////////////////////////////////
    // Fill in header info
    cLocalFileHeader newLocalHeader;
    newLocalHeader.mUncompressedSize = nInputBufferSize;

    // Date and Time
    std::time_t fileTime = std::time(NULL);
    const std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
  
    time_t tt = to_time_t(now);

    newLocalHeader.mLastModificationDate = zip_date_from_std_time(tt);
    newLocalHeader.mLastModificationTime = zip_time_from_std_time(tt);
    newLocalHeader.mCompressionMethod = 8;  // only deflate supported. 

    newLocalHeader.mMinVersionToExtract = kDefaultMinVersionToExtract;
    newLocalHeader.mGeneralPurposeBitFlag = kDefaultGeneralPurposeFlag;

    // relative path
    newLocalHeader.mFilename = sFilename;
    newLocalHeader.mFilenameLength = (uint16_t)sFilename.length();


    uint64_t nOffsetOfStreamData = ((uint64_t)nOffsetToLocalFileHeader) + cLocalFileHeader::kStaticDataSize + newLocalHeader.mFilenameLength + cLocalFileHeader::kExtendedFieldLength;

    ZCompressor compressor;
    compressor.Init(mnCompressionLevel);
    compressor.InitStream(pInputBuffer, (uint32_t)nInputBufferSize);
    int32_t nStatus = Z_OK;
    int32_t nOutIndex = 0;
    while (compressor.HasMoreOutput())
    {
        if (nStatus == Z_OK)
        {
            nStatus = compressor.Compress(true);    // true for final block since we're giving it the entire buffer
            uint32_t nCompressedBytes = (uint32_t)compressor.GetCompressedBytes();
            uint32_t nNumWritten = 0;
            if (!mpZZFile->Write(nOffsetOfStreamData, nCompressedBytes, compressor.GetCompressedBuffer(), nNumWritten))
            {
                cerr << "Failed to write compressed stream for memory buffer " << sFilename.c_str() << " to file " << msZipURL.c_str() << ".  Reason: " << errno << "\n";
                return false;
            }

            nOffsetOfStreamData += nCompressedBytes;
            nOutIndex += nCompressedBytes;
            newLocalHeader.mCompressedSize += nCompressedBytes;
        }

        if (nStatus != Z_OK)
        {
            break;
        }
    }

    if (!(nStatus == Z_OK || nStatus == Z_STREAM_END))
    {
        cerr << "Compress Error #:" << to_string(nStatus) << "\n";
        return false;
    }

    if (pProgress)
        pProgress->AddBytesProcessed(nInputBufferSize);


    // Now write the localfile header
    //newLocalHeader.mCRC32 = (uint32_t)crcCalc;
    newLocalHeader.mCRC32 = crc32_16bytes(pInputBuffer, nInputBufferSize, 0);

    // seek to start of compression stream data
    if (!newLocalHeader.Write(*mpZZFile, nOffsetToLocalFileHeader))
    {
        return false;
    }

    // Add a new CD entry
    cCDFileHeader newCDFileHeader;
    newCDFileHeader.mLastModificationTime = newLocalHeader.mLastModificationTime;
    newCDFileHeader.mLastModificationDate = newLocalHeader.mLastModificationDate;
    newCDFileHeader.mCRC32 = newLocalHeader.mCRC32;
    newCDFileHeader.mCompressionMethod = newLocalHeader.mCompressionMethod;
    newCDFileHeader.mCompressedSize = newLocalHeader.mCompressedSize;
    newCDFileHeader.mUncompressedSize = newLocalHeader.mUncompressedSize;
    newCDFileHeader.mLocalFileHeaderOffset = nOffsetToLocalFileHeader;
    newCDFileHeader.mFileName = newLocalHeader.mFilename;
    newCDFileHeader.mFilenameLength = newLocalHeader.mFilenameLength;

    mZipCD.mCDFileHeaderList.push_back(newCDFileHeader);

    //    wcout << "thread: " << this_thread::get_id() << " Added \"" << sFilename.c_str() << "\"\n";

    return true;
}


bool ZZipAPI::DecompressToFolder(const string& sPattern, const string& sOutputFolder, Progress* pProgress)
{
    uint64_t nFilesDecompressed = 0;
    uint64_t nFoldersCreated = 0;
    uint64_t nFilesSkipped = 0;
    uint64_t nCompressedBytesProcessed = 0;
    uint64_t nDecompressedBytesWritten = 0;
    std::chrono::time_point<std::chrono::system_clock> start, end;

    start = std::chrono::system_clock::now();

    tCDFileHeaderList filesToDecompress;

    // Create folder structure and build list of files that match pattern
    wcout << "Creating Folders.\n";
    for (tCDFileHeaderList::iterator it = mZipCD.mCDFileHeaderList.begin(); it != mZipCD.mCDFileHeaderList.end(); it++)
    {
        cCDFileHeader& cdFileHeader = *it;

        if (FNMatch(sPattern, cdFileHeader.mFileName.c_str()))
        {
            //            wcout << "Pattern: \"" << sPattern.c_str() << "\" File: \"" << cdFileHeader.mFileName.c_str() << "\" matches. \n";

            std::filesystem::path fullPath(sOutputFolder);
            fullPath.append(cdFileHeader.mFileName);

            // Ensure folder exists

            // decompress
            if (!std::filesystem::is_directory(fullPath.parent_path()))
            {
                //                wcout << "Creating Path: \"" << fullPath.branch_path().c_str() << "\"\n";
                if (!std::filesystem::create_directories(fullPath.parent_path()))
                {
                    wcout << "Failed to create path! :" << fullPath.parent_path() << "\n";
                    return false;
                }

                nFoldersCreated++;
            }

            // If the path ends in '/' it's a folder and shouldn't be processed for decompression
            if (cdFileHeader.mFileName[cdFileHeader.mFileName.length() - 1] != '/')
                filesToDecompress.push_back(cdFileHeader);

            nFilesDecompressed++;
            nCompressedBytesProcessed += cdFileHeader.mCompressedSize;
            nDecompressedBytesWritten += cdFileHeader.mUncompressedSize;
        }
        else
        {
            //wcout << "File Skipped: \"" << cdFileHeader.mFileName.c_str() << "\"\n";
            nFilesSkipped++;
        }
    }

    for (auto header : filesToDecompress)
    {
        std::filesystem::path fullPath(sOutputFolder);
        fullPath.append(header.mFileName);

        DecompressToFile(header.mFileName, fullPath.string(), pProgress);
    }


    end = std::chrono::system_clock::now();
    std::chrono::milliseconds elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds> (end - start);

    wcout << "***DecompressToFolder Summary***\n";
    wcout << "Pattern: \"" << sPattern.c_str() << "\"\n";
    wcout << "Output Folder: \"" << sOutputFolder.c_str() << "\"\n";
    wcout << "Files Decompressed: " << nFilesDecompressed << "\n";
    wcout << "Folder Created: " << nFoldersCreated << "\n";
    wcout << "Files Skipped: " << nFilesSkipped << "\n";
    wcout << "Compressed Bytes Processed: " << nCompressedBytesProcessed << "\n";
    wcout << "Decompressed Bytes Written: " << nDecompressedBytesWritten << "\n";
    wcout << "Duration: " << elapsed_ms.count() << "\n";

    return true;
}


bool ZZipAPI::DecompressToBuffer(const string& sFilename, uint8_t* pOutputBuffer, Progress* pProgress)
{
    if (!mbInitted)
        return false;

    cCDFileHeader cdFileHeader;
    if (!mZipCD.GetFileHeader(sFilename, cdFileHeader))
        return false;

    cLocalFileHeader localFileHeader;

    uint32_t nNumBytesProcessed = 0;
    if (!localFileHeader.Read(*mpZZFile, cdFileHeader.mLocalFileHeaderOffset, nNumBytesProcessed))
    {
        cerr << "Failed to read localFileHeader.\n";
        return false;
    }

    uint64_t nStreamOffset = cdFileHeader.mLocalFileHeaderOffset + nNumBytesProcessed;
    uint8_t* pCompStream = new uint8_t[(uint32_t)cdFileHeader.mCompressedSize];

    uint32_t nBytesRead = 0;
    if (!mpZZFile->Read(nStreamOffset, (uint32_t)cdFileHeader.mCompressedSize, pCompStream, nBytesRead))
    {
        delete[] pCompStream;
        cerr << "Failed to seek to read compression stream\n";
        return false;
    }

    ZDecompressor decompressor;
    decompressor.Init();

    decompressor.InitStream(pCompStream, (int32_t)cdFileHeader.mCompressedSize);
    int32_t nStatus = decompressor.Decompress();
    int32_t nOutIndex = 0;
    while (decompressor.HasMoreOutput())
    {
        if (nStatus == Z_OK || nStatus == Z_STREAM_END)
        {
            uint32_t nDecompressedBytes = (uint32_t)decompressor.GetDecompressedBytes();
            memcpy(pOutputBuffer + nOutIndex, decompressor.GetDecompressedBuffer(), nDecompressedBytes);
            nOutIndex += nDecompressedBytes;

            if (pProgress)
                pProgress->AddBytesProcessed(nDecompressedBytes);
        }

        if (nStatus < 0)
        {
            break;
        }
    }

    delete[] pCompStream;

    if (nStatus != Z_STREAM_END)
    {
        cerr << "Decompress Error #:" << to_string(nStatus) << "\n";
        return false;
    }

    return true;
}
