//////////////////////////////////////////////////////////////////////////////////////////////////
// ZipHeaders
// Purpose: Collection of utility classes to deal with Zip Headers
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
#include <string>
#include <list>
#include <thread>
#include <iostream>
#include "common/ZZFileAPI.h"
#include "common/StringHelpers.h"

using namespace std;
using namespace StringHelpers;

// Zip Tags (aka Signature)
const uint32_t kZipEndofCDTag                       = 0x06054b50;
const uint32_t kZip64EndofCDTag                     = 0x06064b50;
const uint32_t kZip64EndofCDLocatorTag              = 0x07064b50;
const uint32_t kZipCDTag                            = 0x02014b50;
const uint32_t kZipLocalFileHeaderTag               = 0x04034b50;
const uint16_t kZipExtraFieldZip64ExtendedInfoTag   = 0x0001;
const uint16_t kZipExtraFieldNTFSTag                = 0x000a;
const uint16_t kZipExtraFieldUnicodePathTag         = 0x7075;   // TBD unicode support
const uint16_t kZipExtraFieldUnicodeCommentTag      = 0x6375;   // TBD unicode support

const uint16_t kDefaultMinVersionToExtract          = 45;
const uint16_t kDefaultVersionMadeBy                = 45;
const uint16_t kDefaultGeneralPurposeFlag           = 2;


//////////////////////////////////////////////////////////////////////////////////////////
class cExtensibleFieldEntry
{
public:
    cExtensibleFieldEntry() : mnHeader(0), mnSize(0) {}
    cExtensibleFieldEntry(const cExtensibleFieldEntry& from);
    ~cExtensibleFieldEntry() {}

    string					ToString();
    string					HeaderToString();

    uint16_t				mnHeader;
    uint16_t				mnSize;
    std::shared_ptr<uint8_t>     mpData;
};

typedef list<cExtensibleFieldEntry> tExtensibleFieldList;



//////////////////////////////////////////////////////////////////////////////////////////
class cLocalFileHeader
{
public:
    static const uint32_t kStaticDataSize =
        sizeof(uint32_t) + //    mLocalFileTag;                  
        sizeof(uint16_t) + //    mMinVersionToExtract;           
        sizeof(uint16_t) + //    mGeneralPurposeBitFlag;         
        sizeof(uint16_t) + //    mCompressionMethod;             
        sizeof(uint16_t) + //    mLastModificationTime;          
        sizeof(uint16_t) + //    mLastModificationDate;          
        sizeof(uint32_t) + //    mCRC32;                         
        sizeof(uint32_t) + //    mCompressedSize;                
        sizeof(uint32_t) + //    mUncompressedSize;              
        sizeof(uint16_t) + //    mFilenameLength;                
        sizeof(uint16_t);  //    mExtraFieldLength;        

                           // The following is only the extra field we need when writing
    static const uint16_t kExtendedFieldLength = sizeof(uint16_t) /*tag*/ + sizeof(uint16_t) /*size of field*/ + sizeof(uint64_t) /*uncompressed size*/ + sizeof(uint64_t) /*compressed size*/;

    cLocalFileHeader() : mLocalFileTag(kZipLocalFileHeaderTag), mMinVersionToExtract(kDefaultMinVersionToExtract), mGeneralPurposeBitFlag(kDefaultGeneralPurposeFlag), mCompressionMethod(0),
        mLastModificationTime(0), mLastModificationDate(0), mCRC32(0), mCompressedSize(0), mUncompressedSize(0), mFilenameLength(0), mExtraFieldLength(0) {}

    bool                    ParseRaw(uint8_t* pBuffer, uint32_t& nNumBytesProcessed);     // Returns the number of bytes parsed for everything below
    static string           FieldNames(eToStringFormat format = kTabs);        // returns tab delimited field names that correspond with the ones returned from ToString
    string                  ToString(eToStringFormat format = kTabs);

    uint64_t                Size(); // in bytes

    bool                    Read(cZZFile& file, uint64_t nOffsetToLocalFileHeader, uint32_t& nNumBytesProcessed);
    bool                    Write(cZZFile& file, uint64_t nOffsetToLocalFileHeader);

    // offsets
    uint32_t                mLocalFileTag;                  // 0
    uint16_t                mMinVersionToExtract;           // 4
    uint16_t                mGeneralPurposeBitFlag;         // 6
    uint16_t                mCompressionMethod;             // 8
    uint16_t                mLastModificationTime;          // 10
    uint16_t                mLastModificationDate;          // 12
    uint32_t                mCRC32;                         // 14
    uint64_t                mCompressedSize;                // 18       // 32 bit in the local file header
    uint64_t                mUncompressedSize;              // 22       // 32 bit in the local file header
    uint16_t                mFilenameLength;                // 26
    uint16_t                mExtraFieldLength;              // 28
    string                  mFilename;                      // 30
    tExtensibleFieldList    mExtensibleFieldList;           // 30 + mFilenameLength;
};

//////////////////////////////////////////////////////////////////////////////////////////
class cEndOfCDRecord
{
public:
    static const uint32_t kStaticDataSize =
        sizeof(uint32_t) + // mEndOfCDRecTag
        sizeof(uint16_t) + // mDiskNum
        sizeof(uint16_t) + // mDiskNumOfCD
        sizeof(uint16_t) + // mNumCDRecordsThisDisk
        sizeof(uint16_t) + // mNumTotalRecords
        sizeof(uint32_t) + // mNumBytesOfCD
        sizeof(uint32_t) + // mCDStartOffset
        sizeof(uint16_t);  // mNumBytesOfComment

    cEndOfCDRecord() : mEndOfCDRecTag(kZipEndofCDTag), mDiskNum(0), mDiskNumOfCD(0), mNumCDRecordsThisDisk((uint16_t)-1), mNumTotalRecords((uint16_t)-1), mNumBytesOfCD((uint32_t)-1),
        mCDStartOffset((uint32_t)-1), mNumBytesOfComment(0) {}

    bool            ParseRaw(uint8_t* pBuffer, uint32_t& nNumBytesProcessed);     // Returns the number of bytes parsed for everything below
    static string   FieldNames(eToStringFormat format = kTabs);                   // returns tab delimited field names that correspond with the ones returned from ToString
    string          ToString(eToStringFormat format = kTabs);
    bool            Write(cZZFile& file); // assumes must be written at end of file

    uint64_t        Size() { return (uint64_t) kStaticDataSize + (uint64_t) mNumBytesOfComment; }

    // offsets
    uint32_t        mEndOfCDRecTag;                 // 0        
    uint16_t        mDiskNum;                       // 4
    uint16_t        mDiskNumOfCD;                   // 6
    uint16_t        mNumCDRecordsThisDisk;          // 8
    uint16_t        mNumTotalRecords;               // 10
    uint32_t        mNumBytesOfCD;                  // 12
    uint32_t        mCDStartOffset;                 // 16
    uint16_t        mNumBytesOfComment;             // 20
    string          mComment;                       // 22
};

//////////////////////////////////////////////////////////////////////////////////////////
class cZip64EndOfCDRecord
{
public:
    static const uint32_t kStaticDataSize =
        sizeof(uint32_t) + // mZip64EndOfCDRecTag
        sizeof(uint64_t) + // mSizeOfZiP64EndOfCDRecord
        sizeof(uint16_t) + // mVersionMadeBy
        sizeof(uint16_t) + // mMinVersionToExtract
        sizeof(uint32_t) + // mDiskNum
        sizeof(uint32_t) + // mDiskNumOfCD
        sizeof(uint64_t) + // mNumCDRecordsThisDisk
        sizeof(uint64_t) + // mNumTotalRecords
        sizeof(uint64_t) + // mNumBytesOfCD
        sizeof(uint64_t);  // mCDStartOffset

    cZip64EndOfCDRecord() : mZip64EndOfCDRecTag(kZip64EndofCDTag), mSizeOfZiP64EndOfCDRecord(0), mVersionMadeBy(kDefaultVersionMadeBy), mMinVersionToExtract(kDefaultMinVersionToExtract),
        mDiskNum(0), mDiskNumOfCD(0), mNumCDRecordsThisDisk(0), mNumTotalRecords(0), mNumBytesOfCD(0), mCDStartOffset(0), mpZip64ExtensibleDataSector(NULL), mnDerivedSizeOfExtensibleDataSector(0) {}
    ~cZip64EndOfCDRecord();

    bool            ParseRaw(uint8_t* pBuffer, uint32_t& nNumBytesProcessed);     // Returns the number of bytes parsed for everything below
    static string   FieldNames(eToStringFormat format = kTabs);                   // returns tab delimited field names that correspond with the ones returned from ToString
    string          ToString(eToStringFormat format = kTabs);
    bool            Write(cZZFile& file);   // assumes must be written at end of file

    uint64_t        Size() { return (uint64_t) kStaticDataSize + (uint64_t) mnDerivedSizeOfExtensibleDataSector; }


    // offsets
    uint32_t        mZip64EndOfCDRecTag;            // 0 
    uint64_t        mSizeOfZiP64EndOfCDRecord;      // 4
    uint16_t        mVersionMadeBy;                 // 12
    uint16_t        mMinVersionToExtract;           // 14
    uint32_t        mDiskNum;                       // 16
    uint32_t        mDiskNumOfCD;                   // 20
    uint64_t        mNumCDRecordsThisDisk;          // 24
    uint64_t        mNumTotalRecords;               // 32
    uint64_t        mNumBytesOfCD;                  // 40
    uint64_t        mCDStartOffset;                 // 48
    uint8_t*        mpZip64ExtensibleDataSector;

    uint32_t        mnDerivedSizeOfExtensibleDataSector; // mSizeOfZiP64EndOfCDRecord + 12 - 56     
};

//////////////////////////////////////////////////////////////////////////////////////////
class cZip64EndOfCDLocator
{
public:
    static const uint32_t kStaticDataSize =
        sizeof(uint32_t) + // mZip64EndOfCDLocatorTag
        sizeof(uint32_t) + // mDiskNumOfCD
        sizeof(uint64_t) + // mCDStartOffset
        sizeof(uint32_t);  // mNumTotalDisks

    cZip64EndOfCDLocator() : mZip64EndOfCDLocatorTag(kZip64EndofCDLocatorTag), mDiskNumOfCD(0), mZip64EndofCDOffset(0), mNumTotalDisks(1) {}

    bool            ParseRaw(uint8_t* pBuffer, uint32_t& nNumBytesProcessed);     // Returns the number of bytes parsed for everything below
    static string   FieldNames(eToStringFormat format = kTabs);                   // returns tab delimited field names that correspond with the ones returned from ToString
    string          ToString(eToStringFormat format = kTabs);

    bool            Write(cZZFile& file);   // assumes must be written at end of file
    uint64_t        Size() { return kStaticDataSize; }

    // offsets
    uint32_t        mZip64EndOfCDLocatorTag;        // 0
    uint32_t        mDiskNumOfCD;                   // 4
    uint64_t        mZip64EndofCDOffset;            // 8
    uint32_t        mNumTotalDisks;                 // 12

};

//////////////////////////////////////////////////////////////////////////////////////////
class cCDFileHeader
{
public:
    static const uint32_t kStaticDataSize =
        sizeof(uint32_t) + //    mCDTag;                  
        sizeof(uint16_t) + //    mVersionMadeBy;                  
        sizeof(uint16_t) + //    mMinVersionToExtract;                  
        sizeof(uint16_t) + //    mGeneralPurposeBitFlag;                  
        sizeof(uint16_t) + //    mCompressionMethod;                  
        sizeof(uint16_t) + //    mLastModificationTime;                  
        sizeof(uint16_t) + //    mLastModificationDate;                  
        sizeof(uint32_t) + //    mCRC32;                  
        sizeof(uint32_t) + //    mCompressedSize;                  
        sizeof(uint32_t) + //    mUncompressedSize;                  
        sizeof(uint16_t) + //    mFilenameLength;                  
        sizeof(uint16_t) + //    mExtraFieldLength;                  
        sizeof(uint16_t) + //    mFileCommentLength;                  
        sizeof(uint16_t) + //    mDiskNumFileStart;                  
        sizeof(uint16_t) + //    mInternalFileAttributes;                  
        sizeof(uint32_t) + //    mExternalFileAttributes;                  
        sizeof(uint32_t);  //    mLocalFileHeaderOffset;                  

                           // The following is only the extra field we need when writing
    static const uint16_t kExtendedFieldLength = sizeof(uint64_t) /*uncompressed size*/ + sizeof(uint64_t) /*compressed size*/ + sizeof(uint64_t) /*offset of localfile header*/ + sizeof(uint32_t) /* Disk Number */;
    static const uint16_t kExtraFieldLength = kExtendedFieldLength + sizeof(uint16_t) /*tag*/ + sizeof(uint16_t) /*bytes of extended header */;

    cCDFileHeader() : mCDTag(kZipCDTag), mVersionMadeBy(kDefaultVersionMadeBy), mMinVersionToExtract(kDefaultMinVersionToExtract), mGeneralPurposeBitFlag(kDefaultGeneralPurposeFlag),
        mCompressionMethod(0), mLastModificationTime(0), mLastModificationDate(0), mCRC32(0), mCompressedSize(0), mUncompressedSize(0), mFilenameLength(0), mExtraFieldLength(0),
        mFileCommentLength(0), mDiskNumFileStart(0), mInternalFileAttributes(0), mExternalFileAttributes(0), mLocalFileHeaderOffset(0) {}

    bool                    ParseRaw(uint8_t* pBuffer, uint32_t& nNumBytesProcessed);     // Returns the number of bytes parsed for everything below
    static string           FieldNames(eToStringFormat format = kTabs);                   // returns tab delimited field names that correspond with the ones returned from ToString
    string                  ToString(eToStringFormat format = kTabs);

    bool                    Write(cZZFile& file);   // assumes must be written at end of file

    uint64_t                Size();                         // in bytes

                                                            // offsets
    uint32_t                mCDTag;                         // 0
    uint16_t                mVersionMadeBy;                 // 4
    uint16_t                mMinVersionToExtract;           // 6
    uint16_t                mGeneralPurposeBitFlag;         // 8
    uint16_t                mCompressionMethod;             // 10
    uint16_t                mLastModificationTime;          // 12
    uint16_t                mLastModificationDate;          // 14
    uint32_t                mCRC32;                         // 16
    uint64_t                mCompressedSize;                // 20       32bit for regular zip.  kept here as 64 bit (and from Zip64ExtendedInfo field) for Zip64
    uint64_t                mUncompressedSize;              // 24       32bit for regular zip.  kept here as 64 bit (and from Zip64ExtendedInfo field) for Zip64
    uint16_t                mFilenameLength;                // 28
    uint16_t                mExtraFieldLength;              // 30
    uint16_t                mFileCommentLength;             // 32
    uint16_t                mDiskNumFileStart;              // 34
    uint16_t                mInternalFileAttributes;        // 36
    uint32_t                mExternalFileAttributes;        // 38
    uint64_t                mLocalFileHeaderOffset;         // 42       32bit for regular zip.  kept here as 64 bit (and from Zip64ExtendedInfo field) for Zip64
    string                  mFileName;                      // 46
    tExtensibleFieldList    mExtensibleFieldList;           // 46 + mFilenameLength;
    string                  mFileComment;                   // 46 + mFilenameLength + mExtraFieldLength;
};

typedef list<cCDFileHeader> tCDFileHeaderList;

//////////////////////////////////////////////////////////////////////////////////////////
class cZipCD
{
public:
    cZipCD();
    ~cZipCD();

    bool                    Init(cZZFile& httpFile);
    bool                    GetFileHeader(const string& sFilename, cCDFileHeader& fileHeader);        // returns a header (if there is one) for the file in the zip package
    uint64_t                GetNumTotalEntries() { return mCDFileHeaderList.size(); }
    uint64_t                GetNumTotalFiles();
    uint64_t                GetNumTotalFolders();
    uint64_t                GetTotalCompressedBytes();
    uint64_t                GetTotalUncompressedBytes();

    bool                    Write(cZZFile& file);   // assumes must be written at end of file
    uint64_t                Size();     // size of CD in bytes

    void                    DumpCD(std::ostream& out, const wstring& sPattern, bool bVerbose, eToStringFormat format);

    tCDFileHeaderList       mCDFileHeaderList;
    cEndOfCDRecord          mEndOfCDRecord;
    cZip64EndOfCDLocator    mZip64EndOfCDLocator;
    cZip64EndOfCDRecord     mZip64EndOfCDRecord;

    bool                    ComputeCDRecords(uint64_t nStartOfCDOffset);     // called right before writing

    bool                    mbIsZip64;
    bool                    mbInitted;

};
