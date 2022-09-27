//////////////////////////////////////////////////////////////////////////////////////////////////
// ZipJob
// Purpose: This high level API can be used at a high level to kick off multithreaded commands.
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
#include <thread>
#include <mutex>
#include "ZipHeaders.h"
#include "ZZipTrackers.h"


class ZZipAPI;
typedef std::list< std::thread* > tThreadList;

class ZipJob
{
public:
    enum eJobType
    {
        kNone = 0,
        kExtract = 1,  
        kCompress = 2,
        kDiff = 3,
        kList = 4
    };

    ZipJob(eJobType jobType) : mbSkipCRC(false), mbKillHoldingProcess(false), mnThreads(6), mOutputFormat(kTabs), mbVerbose(false) { mJobType = jobType; }

    ~ZipJob();

    // Configuration API
    void SetURL(const std::string& sURL)                { msPackageURL = sURL; }
    void SetNamePassword(const std::string& sName, const std::string& sPassword) { msName = sName; msPassword = sPassword; }
    void SetBaseFolder(const std::string& sBaseFolder);
    void SetPattern(const std::string& sPattern)        { msPattern = sPattern; }
    void SetSkipCRC(bool bSkip)                     { mbSkipCRC = bSkip; }
    void SetKillHoldingProcess(bool bKill)          { mbKillHoldingProcess = bKill; }
    void SetNumThreads(uint32_t nThreads)           { if (!mbVerbose) mnThreads = nThreads; }   // verbose mode is single threaded
    void SetOutputFormat(eToStringFormat format)    { mOutputFormat = format; }
    void SetVerbose(bool bVerbose)                  { mbVerbose = bVerbose; if (mbVerbose) mnThreads = 1; }
    
    // Controls
    bool Run();
    bool Join();

    // Accessors
    bool IsDone() { return mJobStatus.mStatus == JobStatus::eJobStatus::kFinished || mJobStatus.mStatus == JobStatus::eJobStatus::kError; }
    JobStatus GetStatus() { return mJobStatus; }    // makes a copy
    Progress GetProgress() { return mJobProgress; } // makes a copy

private:
    bool FileNeedsUpdate(const std::string& sPath, uint64_t nComparedFileSize, uint32_t nComparedFileCRC);

    static void RunDecompressionJob(void* pContext);
    static void RunCompressionJob(void* pContext);
    static void RunDiffJob(void* pContext);
    static void RunListJob(void* pContext);

    tThreadList         mWorkers;
    std::mutex          mMutex;

    eJobType            mJobType;           
    std::string             msPackageURL;           // source package URL
    std::string             msName;                 // Auth
    std::string             msPassword;             // Auth
    std::string             msBaseFolder;           // Destination base folder. (default is the folder of ZZipUpdate.exe)
    std::string             msPattern;              // wildcard pattern to match (example "*base*/*.exe"  matches all directories that have the string "base" in them and in those directories all files that end in .exe)
    bool                mbSkipCRC;              // If true, skips CRC diff and syncs down all files that match pattern
    bool                mbKillHoldingProcess;   // If true, kills the process holding a necessary file open
    uint32_t            mnThreads;              // How many threads to use
    eToStringFormat     mOutputFormat;
    JobStatus           mJobStatus; 
    Progress            mJobProgress;
    bool                mbVerbose;
};


