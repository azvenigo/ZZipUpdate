// MIT License
// Copyright 2019 Alex Zvenigorodsky
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "HTTPCache.h"
#include <algorithm>

atomic<int64_t> gnTotalCacheLinesReserved = 0;
atomic<int64_t> gnTotalBytesReserved = 0;
atomic<int64_t> gnTotalBytesSaved = 0;
atomic<int64_t> gnTotalBytesCopiedAcrossCacheLines = 0;

bool Intersect(tIntPair& pResult, const tIntPair& a, const tIntPair& b)
{
    if (b.first >= a.second || a.first >= b.second)
        return false;
    
    pResult.first = std::max(a.first, b.first);
    pResult.second = std::min(a.second, b.second);

    return true;
}

HTTPCacheLine::HTTPCacheLine()
{
    mbCommitted = false;
    mnBaseOffset = -1;
    mnBufferData = 0;
    memset(&mData[0], 0, kHTTPCacheLineSize);
    mUnfullfilledInterval = tIntPair(0, 0);
}

HTTPCacheLine::~HTTPCacheLine()
{
}



HTTPCache::HTTPCache()
{
}

HTTPCache::~HTTPCache()
{
    //cout << "-HTTPCache Report-\n";
    //cout << "Total Bytes Reserved:            " << gnTotalBytesReserved << "\n";
    //cout << "Total Cache Lines Reserved:      " << gnTotalCacheLinesReserved << "\n";
    //cout << "Bytes Copied Across Cache Lines: " << gnTotalBytesCopiedAcrossCacheLines << "\n";
    //cout << "Bytes Saved :                    " << gnTotalBytesSaved << "\n";

}

shared_ptr<HTTPCacheLine> HTTPCache::Reserve(int64_t nOffset)
{
    // mMutex already locked at this point

    // Create a new cache line
    shared_ptr<HTTPCacheLine> pNewItem(new HTTPCacheLine());
    pNewItem->mnBaseOffset = nOffset;
    pNewItem->mRequestTime = std::chrono::system_clock::now();
    pNewItem->mUnfullfilledInterval = tIntPair(nOffset, nOffset + kHTTPCacheLineSize);

    // See if any cache data can be copied into new item
    // See if there is data in other cache lines we can copy
    for (tOffsetToHTTPCacheLineMap::iterator it = mOffsetToHTTPCacheLineMap.begin(); it != mOffsetToHTTPCacheLineMap.end(); it++)
    {
        std::shared_ptr<HTTPCacheLine> pExistingItem = (*it).second;

        // TBD consider using an event flag
        while (!pExistingItem->mbCommitted)
            std::this_thread::sleep_for(std::chrono::milliseconds(0));

        tIntPair overlap;
        tIntPair existingItemAvailableInterval(pExistingItem->mnBaseOffset, pExistingItem->mnBaseOffset + kHTTPCacheLineSize);

        // Look for overlapping data
        if (Intersect(overlap, existingItemAvailableInterval, pNewItem->mUnfullfilledInterval))
        {
            uint32_t nOffsetIntoNewBuffer = (uint32_t)(overlap.first - pNewItem->mnBaseOffset);
            uint32_t nOffsetIntoExistingBuffer = (uint32_t)(overlap.first - pExistingItem->mnBaseOffset);
            uint32_t nUsableData = (uint32_t)(overlap.second - overlap.first);

            memcpy(pNewItem->mData + nOffsetIntoNewBuffer, pExistingItem->mData + nOffsetIntoExistingBuffer, nUsableData);

            if (overlap.first == pNewItem->mUnfullfilledInterval.first)    // usable data for the beginning of the new buffer
            {
                pNewItem->mUnfullfilledInterval.first += nUsableData;
            }
            else
            {
                pNewItem->mUnfullfilledInterval.second -= nUsableData;
            }

            //cout << "offset:" << nOffset << " Copied " << nUsableData << "bytes into new buffer at offset:" << nOffset << "\n";
            gnTotalBytesCopiedAcrossCacheLines += nUsableData;
        }

    }

    pNewItem->mnBufferData = (int32_t) kHTTPCacheLineSize- (int32_t) (pNewItem->mUnfullfilledInterval.second - pNewItem->mUnfullfilledInterval.first);


    // Does one need to be evicted?
    if (mOffsetToHTTPCacheLineMap.size() >= kMaxCacheLines)
    {
        // find oldest reservation and remove

        chrono::time_point<chrono::system_clock> now = std::chrono::system_clock::now();
        std::chrono::microseconds highest_elapsed = std::chrono::duration_cast<std::chrono::microseconds> (now-now);

        tOffsetToHTTPCacheLineMap::iterator highest_it = mOffsetToHTTPCacheLineMap.end();

        for (tOffsetToHTTPCacheLineMap::iterator it = mOffsetToHTTPCacheLineMap.begin(); it != mOffsetToHTTPCacheLineMap.end(); it++)
        {
            shared_ptr<HTTPCacheLine> pItem = (*it).second;

            // if no buffer data then this is a pending request, do not evict
            if (pItem->mnBufferData > 0)
            {
                std::chrono::microseconds elapsed = std::chrono::duration_cast<std::chrono::microseconds> (now - pItem->mFullfilledTime);
                if (elapsed > highest_elapsed)
                {
                    highest_elapsed = elapsed;
                    highest_it = it;
                }
            }
        }

        if (highest_it == mOffsetToHTTPCacheLineMap.end())
        {
            // couldn't find any item to evict!  
            // Means all cache lines are pending.... we must wait until one is available
            // haven't encountered this condition yet but may need aa solution

            // TBD
            cout << "Fatal error! Ran out of cache lines!\n";
        }
        else
        {
            mOffsetToHTTPCacheLineMap.erase(highest_it);
        }
    }


    mOffsetToHTTPCacheLineMap[nOffset] = pNewItem;
    //cout << "reserve new at offset:" << nOffset << "\n";

    gnTotalCacheLinesReserved++;
    gnTotalBytesReserved += kHTTPCacheLineSize;

    return pNewItem;
}


bool HTTPCache::CheckOrReserve(int64_t nOffset, int32_t nBytes, shared_ptr<HTTPCacheLine>& pCacheLine)
{
    std::unique_lock<mutex> lock(mMutex);

    // If the byte range can be satisfied, return the appropriate cache line
    for (auto it : mOffsetToHTTPCacheLineMap)
    {
        shared_ptr<HTTPCacheLine> pItem = it.second;

        if (nOffset >= pItem->mnBaseOffset && nOffset+nBytes < pItem->mnBaseOffset + kHTTPCacheLineSize)
        {
            pCacheLine = pItem;
            return false;   // not new
        }
    }

    // Create a new cache line for the request, copy whatever content can be and return
    pCacheLine = Reserve(nOffset);
    return true;
}

bool HTTPCacheLine::Get(int64_t nOffset, int32_t nBytes, uint8_t* pDestination)
{
    if (nOffset >= mnBaseOffset && nOffset + nBytes < mnBaseOffset + kHTTPCacheLineSize)
    {
        // if mnBufferData is 0 then this data is pending. Wait until it is fullfilled
        const uint64_t kSleepTimeOut = 60 * 1000;   // 60 second timeout reasonable?
        while (!mbCommitted)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(0));
            chrono::time_point<chrono::system_clock> now = std::chrono::system_clock::now();
            std::chrono::milliseconds elapsed = std::chrono::duration_cast<std::chrono::milliseconds> (now - mRequestTime);

            if (elapsed.count() > kSleepTimeOut)
            {
                cout << "TIMEOUT waiting on cache data!\n";
                return false;
            }
        }


        // Retrieve the requested byte range
        uint32_t nIndexIntoCacheLine = (uint32_t) (nOffset - mnBaseOffset);
        //cout << "found line item offset:" << mnBaseOffset << " copying from index:" << nIndexIntoCacheLine << " bytes:" << nBytes << "\n";
        memcpy(pDestination, mData + nIndexIntoCacheLine, nBytes);
        gnTotalBytesSaved += nBytes;
        return true;
    }

    // cannot fullfill!
    cout << "Fatal Error! This cache line does not and will not contain the requested bytes!\n";
    return false;
}

bool HTTPCacheLine::Commit(int32_t nBytes)
{
    mFullfilledTime = std::chrono::system_clock::now();
    mUnfullfilledInterval.first = 0;
    mUnfullfilledInterval.second = 0;
    mnBufferData += nBytes;

#ifdef _DEBUG
    assert(mnBufferData == kHTTPCacheLineSize);
#endif
    mbCommitted = true;

    return true;
}
