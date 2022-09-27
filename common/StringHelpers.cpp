// MIT License
// Copyright 2019 Alex Zvenigorodsky
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "common/StringHelpers.h"
#include <stdint.h>
#include <inttypes.h>
using namespace std;

string StringHelpers::int_to_hex_string(uint32_t nVal)
{
	char buf[64];
	sprintf_s(&buf[0], 64, "0x%X", nVal);
	string sRet;
	sRet.assign(buf);
	return sRet;
}

string StringHelpers::int_to_hex_string(uint64_t nVal)
{
	char buf[64];
	sprintf_s(&buf[0], 64, "0x%llX", nVal);
	string sRet;
	sRet.assign(buf);
	return sRet;
}

string StringHelpers::binary_to_hex(uint8_t* pBuf, int32_t nLength)
{
	string sRet;
	char buf[64];
	for (int32_t i = 0; i < nLength; i++)
	{
		sprintf_s(&buf[0], 64, "%02X", *(pBuf + i));
		sRet.append(buf);
	}

	return sRet;
}

string StringHelpers::FormatFriendlyBytes(uint64_t nBytes, int64_t sizeType)
{
    const uint64_t kTB = 1024ull * 1024ull * 1024ull * 1024ull;
    const uint64_t kGB = 1024ull * 1024ull * 1024ull;
    const uint64_t kMB = 1024ull * 1024ull;
    const uint64_t kKB = 1024ull;

    if (sizeType == kAuto)
    {
        if (nBytes > kTiB)   // TB show in GB
            sizeType = kGiB;
        if (nBytes > kGiB)	// GB show in MB
            sizeType = kMiB;
        else if (nBytes > kMiB)	// MB show in KB
            sizeType = kKiB;
        else
            sizeType = kBytes;
    }

    char buf[128];

    switch (sizeType)
    {
    case kKiB:
        sprintf(buf, "%" PRId64 "KiB", nBytes / kKiB);
        return string(buf);
        break;
    case kMiB:
        sprintf(buf, "%" PRId64 "MiB", nBytes / kMiB);
        return string(buf);
        break;
    case kGiB:
        sprintf(buf, "%" PRId64 "GiB", nBytes / kGiB);
        return string(buf);
        break;
    }

    if (nBytes > kGiB)       // return in MB
    {
        sprintf(buf, "%" PRId64 "MiB", nBytes / kMiB);
        return string(buf);
    }
    else if (nBytes > kMB)  // return in KB
    {
        sprintf(buf, "%" PRId64 "KiB", nBytes / kKiB);
        return string(buf);
    }

    else sprintf(buf, "%" PRId64 "bytes", nBytes);

    return string(buf);
}



// Converts user readable numbers into ints
// Supports hex (0x12345)
// Strips commas (1,000,000)
// Supports trailing scaling labels  (k, kb, kib, m, mb, mib, etc.)
int64_t StringHelpers::IntFromUserReadable(string sReadable)
{
    std::transform(sReadable.begin(), sReadable.end(), sReadable.begin(), [](unsigned char c) { return (unsigned char)std::toupper(c); });

    // strip any commas in case human readable string has those
    sReadable.erase(std::remove(sReadable.begin(), sReadable.end(), ','), sReadable.end());


    // Determine if this is a hex value
    int32_t nNumberBase = 10;
    if (sReadable.substr(0, 2) == "0X")
    {
        nNumberBase = 16;
        sReadable = sReadable.substr(2);
    }


    int32_t nReadableLength = (int32_t)sReadable.length();

    // count how many chars in the trailing label (if any)
    int32_t nLabelChars = 0;
    for (int32_t i = nReadableLength - 1; i >= 0; i--)
    {
        char c = sReadable[i];
        if (c < 'A' || c > 'Z')
            break;

        nLabelChars++;
    }

    for (int i = 0; i < sizeEntryTableSize; i++)
    {
        const sSizeEntry& entry = sizeEntryTable[i];

        if (sReadable.substr(nReadableLength - nLabelChars).compare(entry.label) == 0)
        {
            int64_t nOut = strtoll(sReadable.substr(0, nReadableLength - nLabelChars).c_str(), NULL, nNumberBase);
            return nOut * entry.value;
        }
    }

    int64_t nOut = strtoll(sReadable.c_str(), NULL, nNumberBase);
    return nOut;
}

// If the number is a power of two, converts to a more readable form
// example 32768   -> 32KiB
//         1048576 -> 1MiB
string StringHelpers::UserReadableFromInt(int64_t nValue)
{
    char buf[128];
    if (nValue % kPiB == 0)
        sprintf(buf, "%" PRId64 "PiB", nValue / kPiB);
    else if (nValue % kPB == 0)
        sprintf(buf, "%" PRId64 "PB", nValue / kPB);

    else if (nValue % kTiB == 0)
        sprintf(buf, "%" PRId64 "TiB", nValue / kTiB);
    else if (nValue % kTB == 0)
        sprintf(buf, "%" PRId64 "TB", nValue / kTB);

    else if (nValue % kGiB == 0)
        sprintf(buf, "%" PRId64 "GiB", nValue / kGiB);
    else if (nValue % kGB == 0)
        sprintf(buf, "%" PRId64 "GB", nValue / kGB);

    else if (nValue % kMiB == 0)
        sprintf(buf, "%" PRId64 "MiB", nValue / kMiB);
    else if (nValue % kMB == 0)
        sprintf(buf, "%" PRId64 "MB", nValue / kMB);

    else if (nValue % kKiB == 0)
        sprintf(buf, "%" PRId64 "KiB", nValue / kKiB);
    else if (nValue % kKB == 0)
        sprintf(buf, "%" PRId64 "KB", nValue / kKB);

    else sprintf(buf, "%" PRId64, nValue);

    return string(buf);
}

