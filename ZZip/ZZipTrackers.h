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
    DecompressTaskResult(eDecompressTaskStatus nStatus, uint32_t nOSErrorCode, uint64_t nBytesDownloaded, uint64_t nBytesWrittenToDisk, int32_t retriesRemaining, const std::string& fileName, const std::string& result) :
        mDecompressTaskStatus(nStatus),
        mOSErrorCode(nOSErrorCode),
        mBytesDownloaded(nBytesDownloaded),
        mBytesWrittenToDisk(nBytesWrittenToDisk),
        mRetriesRemaining(retriesRemaining),
        mFilename(fileName),
        mResult(result) {}

    friend std::ostream& operator << (std::ostream& os, const eDecompressTaskStatus& status)
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

    friend std::ostream& operator << (std::ostream& os, const DecompressTaskResult& result) { os << "Filename:" << result.mFilename << " Status:" << result.mDecompressTaskStatus << " OSError:" << result.mOSErrorCode << " mBytesDownloaded:" << result.mBytesDownloaded << " mBytesWrittenToDisk:" << result.mBytesWrittenToDisk << " mRetriesRemaining:" << result.mRetriesRemaining << " Result:" << result.mResult;  return os; }

    eDecompressTaskStatus   mDecompressTaskStatus;
    uint32_t                mOSErrorCode;
    uint64_t                mBytesDownloaded;
    uint64_t                mBytesWrittenToDisk;
    int32_t                 mRetriesRemaining;
    std::string                  mFilename;
    std::string                  mResult;
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
    DiffTaskResult(eDiffTaskStatus nStatus, uint64_t nSize, const std::string& fileName) :
        mDiffTaskStatus(nStatus),
        mnSize(nSize),
        mFilename(fileName) {}

    friend std::ostream& operator << (std::ostream& os, const eDiffTaskStatus& status)
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

    friend std::ostream& operator << (std::ostream& os, const DiffTaskResult& result) { os << "FileName:" << result.mFilename << " Status:" << result.mDiffTaskStatus; return os; }

    eDiffTaskStatus mDiffTaskStatus;
    uint64_t        mnSize;
    std::string          mFilename;

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
        std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
        std::chrono::milliseconds elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds> (now - mStart);

        return elapsed_ms.count();
    }

    uint64_t GetPercentageComplete()
    {
        if (mnBytesToProcess == 0)
            return 0;

        uint64_t nPercentComplete = ((mnBytesProcessed * 100) / mnBytesToProcess);

        return nPercentComplete;
    }

    uint64_t GetBytesPerSecond()
    {
        uint64_t nElapsedSeconds = GetElapsedTimeMS() / 1000;
        if (nElapsedSeconds > 0)
            return mnBytesProcessed / nElapsedSeconds;

        return 0;
    }

    uint64_t GetEstimatedSecondsRemaining()
    {
        uint64_t nBytesPerSecond = GetBytesPerSecond();
        uint64_t nBytesRemaining = mnBytesToProcess - mnBytesProcessed;

        if (nBytesPerSecond > 0)
            return  nBytesRemaining / nBytesPerSecond;

        return 0;
    }

    std::chrono::time_point<std::chrono::system_clock>   mStart;       // for time tracking
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

    enum eJobError
    {
        kError_None = 0,
        kError_Undefined = -1,
        kError_NotFound = -2,
        kError_OpenFailed = -3,
        kError_ReadFailed = -4
    };


    JobStatus() : mStatus(kNone), mErrorCode(0) {}

    JobStatus(eJobStatus status, int32_t errorCode, std::string sErrorMessage) : mStatus(status), mErrorCode(errorCode), msErrorMessage(sErrorMessage) {}

    JobStatus(const JobStatus& rhs)
    {
        mStatus = rhs.mStatus;
        mErrorCode = rhs.mErrorCode;
        msErrorMessage = rhs.msErrorMessage;
    }


    friend std::ostream& operator << (std::ostream& os, const JobStatus& jobStatus)
    {
        os << "Status:";
        switch (jobStatus.mStatus)
        {
        case kNone:     os << "None"; return os;
        case kRunning:  os << "Running"; return os;
        case kFinished: os << "Finished"; return os;
        }

        if (jobStatus.mStatus == kError)
            os << "Error Code:" << jobStatus.mErrorCode << " - " << jobStatus.msErrorMessage.c_str();

        return os;

    };


    void SetError(int32_t osErrorCode, std::string sErrorMessage = "")
    {
        mStatus = kError;
        mErrorCode = osErrorCode;
        msErrorMessage = sErrorMessage;
    }

    eJobStatus      mStatus;
    int32_t         mErrorCode;
    std::string         msErrorMessage;
};


