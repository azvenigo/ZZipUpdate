//////////////////////////////////////////////////////////////////////////////////////////////////
// ZZipTrackers
// Purpose: A collection of status/progress tracking classes
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

//////////////////////////////////////////////////////////////////////////////////////////
class DecompressTaskResult
{
public:
    enum eDecompressTaskStatus
    {
        kError = -1,
        kNone = 0,
        kAlreadyUpToDate = 1,
        kExtracted = 2,
        kFolderCreated = 3,
        kSkipping = 4
    };

    DecompressTaskResult() : mDecompressTaskStatus(kNone), mOSErrorCode(0), mBytesDownloaded(0), mBytesWrittenToDisk(0), mRetriesRemaining(0) {}
    DecompressTaskResult(eDecompressTaskStatus nStatus, uint32_t nOSErrorCode, uint64_t nBytesDownloaded, uint64_t nBytesWrittenToDisk, int32_t retriesRemaining, const string& fileName, const string& result) :
        mDecompressTaskStatus(nStatus),
        mOSErrorCode(nOSErrorCode),
        mBytesDownloaded(nBytesDownloaded),
        mBytesWrittenToDisk(nBytesWrittenToDisk),
        mRetriesRemaining(retriesRemaining),
        mFilename(fileName),
        mResult(result) {}

    friend ostream& operator << (ostream& os, const eDecompressTaskStatus& status)
    {
        switch (status)
        {
        case kError: os << "Error"; return os;
        case kNone:  os << "None"; return os;
        case kAlreadyUpToDate: os << "AlreadyUpToDate"; return os;
        case kExtracted: os << "Extracted"; return os;
        case kFolderCreated: os << "FolderCreated"; return os;
        case kSkipping: os << "Skipping"; return os;
        }
        return os;
    }

    friend ostream& operator << (ostream& os, const DecompressTaskResult& result) { os << "Filename:" << result.mFilename << " Status:" << result.mDecompressTaskStatus << " OSError:" << result.mOSErrorCode << " mBytesDownloaded:" << result.mBytesDownloaded << " mBytesWrittenToDisk:" << result.mBytesWrittenToDisk << " mRetriesRemaining:" << result.mRetriesRemaining << " Result:" << result.mResult;  return os; }

    eDecompressTaskStatus	mDecompressTaskStatus;
    uint32_t	            mOSErrorCode;
    uint64_t	            mBytesDownloaded;
    uint64_t	            mBytesWrittenToDisk;
    int32_t		            mRetriesRemaining;

    string		            mFilename;
    string                  mResult;
};

//////////////////////////////////////////////////////////////////////////////////////////
class DiffTaskResult
{
public:
    enum eDiffTaskStatus
    {
        kError = -1,
        kNone = 0,
        kFileMatch = 1,
        kDirMatch = 2,
        kFileDifferent = 3,

        kFilePackageOnly = 4,
        kDirPackageOnly = 5,

        kFilePathOnly = 6,
        kDirPathOnly = 7
    };

    DiffTaskResult() : mDiffTaskStatus(kNone), mnSize(0) {}
    DiffTaskResult(eDiffTaskStatus nStatus, uint64_t nSize, const string& fileName) :
        mDiffTaskStatus(nStatus),
        mnSize(nSize),
        mFilename(fileName) {}

    friend ostream& operator << (ostream& os, const eDiffTaskStatus& status)
    {
        switch (status)
        {
        case kError:            os << "Error"; return os;
        case kNone:             os << "None"; return os;
        case kFileMatch:        os << "File Match"; return os;
        case kDirMatch:         os << "Dir Match"; return os;
        case kFileDifferent:    os << "File Different"; return os;
        case kFilePackageOnly:  os << "File in Package Only"; return os;
        case kDirPackageOnly:   os << "Dir in Package Only"; return os;
        case kFilePathOnly:     os << "File in Path Only"; return os;
        case kDirPathOnly:      os << "Dir in Path Only"; return os;
        }
        return os;
    }

    friend ostream& operator << (ostream& os, const DiffTaskResult& result) { os << "FileName:" << result.mFilename << " Status:" << result.mDiffTaskStatus; return os; }

    eDiffTaskStatus mDiffTaskStatus;
    uint64_t        mnSize;
    string          mFilename;

};

//////////////////////////////////////////////////////////////////////////////////////////
class Progress
{
public:
    Progress() : mnBytesProcessed(0), mnBytesToProcess(0) {}

    Progress(uint64_t nBytesProcessed, uint64_t nBytesToProcess) : mnBytesProcessed(nBytesProcessed), mnBytesToProcess(nBytesToProcess) {}

    Progress(const Progress& rhs)
    {
        mStart = rhs.mStart;
        mnBytesProcessed.store(rhs.mnBytesProcessed);
        mnBytesToProcess.store(rhs.mnBytesToProcess);
    }

    uint64_t GetBytesProcessed()
    {
        return mnBytesProcessed;
    }

    uint64_t GetBytesToProcess()
    {
        return mnBytesToProcess;
    }

    void Reset()
    {
        mnBytesToProcess = 0;
        mnBytesProcessed = 0;
        mStart = std::chrono::system_clock::now();
    }

    // As we find work to do, add it here to track progress
    void AddBytesToProcess(uint64_t nBytesToProcess)
    {
        mnBytesToProcess += nBytesToProcess;
    }

    // as we complete work, track it here
    void AddBytesProcessed(uint64_t nBytes)
    {
        mnBytesProcessed += nBytes;
    }


    uint64_t GetElapsedTimeMS()
    {
        uint64_t nReturn = 0;
        std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();

        //        mMutex.lock();
        std::chrono::milliseconds elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds> (now - mStart);
        //        mMutex.unlock();

        nReturn = elapsed_ms.count();

        return nReturn;
    }

    uint64_t GetPercentageComplete()
    {
        //        mMutex.lock();
        if (mnBytesToProcess == 0)
            return 0;

        uint64_t nPercentComplete = ((mnBytesProcessed * 100) / mnBytesToProcess);
        //        mMutex.unlock();

        return nPercentComplete;
    }

    uint64_t GetBytesPerSecond()
    {
        uint64_t nElapsedSeconds = GetElapsedTimeMS() / 1000;
        uint64_t nReturn = 0;
        //        mMutex.lock();
        if (nElapsedSeconds > 0)
            nReturn = mnBytesProcessed / nElapsedSeconds;
        //        mMutex.unlock();

        return nReturn;
    }

    uint64_t GetEstimatedSecondsRemaining()
    {
        uint64_t nBytesPerSecond = GetBytesPerSecond();

        //        mMutex.lock();
        uint64_t nBytesRemaining = mnBytesToProcess - mnBytesProcessed;
        //        mMutex.unlock();

        uint64_t nSecondsRemaining = 0;
        if (nBytesPerSecond > 0)
            nSecondsRemaining = nBytesRemaining / nBytesPerSecond;

        return nSecondsRemaining;
    }

    chrono::time_point<std::chrono::system_clock>   mStart;       // for time tracking
    std::atomic<uint64_t>                           mnBytesProcessed;
    std::atomic<uint64_t>                           mnBytesToProcess;
};

//////////////////////////////////////////////////////////////////////////////////////////
class JobStatus
{
public:
    enum eJobStatus
    {
        kError = -1,
        kNone = 0,
        kRunning = 1,
        kFinished = 2
    };

    JobStatus() : mStatus(kNone), mOSErrorCode(0) {}

    JobStatus(eJobStatus status, uint32_t osErrorCode, wstring sErrorMessage) : mStatus(status), mOSErrorCode(osErrorCode), msErrorMessage(sErrorMessage) {}

    JobStatus(const JobStatus& rhs)
    {
        mStatus = rhs.mStatus;
        mOSErrorCode = rhs.mOSErrorCode;
        msErrorMessage = rhs.msErrorMessage;
    }

    void SetError(uint32_t osErrorCode, wstring sErrorMessage = L"")
    {
        mStatus = kError;
        mOSErrorCode = osErrorCode;
        msErrorMessage = sErrorMessage;
    }

    eJobStatus      mStatus;
    uint32_t        mOSErrorCode;
    wstring         msErrorMessage;
};


