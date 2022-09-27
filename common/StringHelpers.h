//////////////////////////////////////////////////////////////////////////////////////////////////
// StringHelpers
// Purpose: A collection of String formating and conversion utilities. 
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
#include <cctype>
#include <locale>
#include <codecvt>
#include <algorithm>
#include <list>
//#include <boost/lexical_cast.hpp>

namespace StringHelpers
{
    //////////////////////////////////////////////////////////////////////////////////////////////
    // Common conversions
    // some commonly used conversions
    std::string	int_to_hex_string(uint32_t nVal);
    std::string	int_to_hex_string(uint64_t nVal);
    std::string	binary_to_hex(uint8_t* pBuf, int32_t nLength);

    

    inline std::string  wstring_to_string(const std::wstring& rhs)
    {
        using convert_typeX = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<convert_typeX, wchar_t> converterX;

        return converterX.to_bytes(rhs);
    }

    inline std::wstring string_to_wstring(const std::string& rhs)
    {
        using convert_typeX = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<convert_typeX, wchar_t> converterX;

        return converterX.from_bytes(rhs);
    }

    
    inline void makelower(std::string& rhs) { std::transform(rhs.begin(), rhs.end(), rhs.begin(), [](unsigned char c) { return (unsigned char) std::tolower(c); }); }
    inline void makelower(std::wstring& rhs) { std::transform(rhs.begin(), rhs.end(), rhs.begin(), [](wchar_t c) { return (wchar_t) std::tolower(c); }); }

    inline void makeupper(std::string& rhs) { std::transform(rhs.begin(), rhs.end(), rhs.begin(), [](unsigned char c) { return (unsigned char) std::toupper(c); }); }
    inline void makeupper(std::wstring& rhs) { std::transform(rhs.begin(), rhs.end(), rhs.begin(), [](wchar_t c) { return (wchar_t) std::toupper(c); }); }


    
    





    //////////////////////////////////////////////////////////////////////////////////////////////
    // Output format helpers
    // A way of formatting output into tabs, commas or HTML
	enum eToStringFormat
	{
		kUnknown = 0,
		kTabs = 1,
		kCommas = 2,
		kHTML = 3
	};

    inline std::string StartPageHeader(eToStringFormat format)
    {
        if (format == kHTML)
            return "<html><style> *{ font-family: 'Lucida Console', Lucida, sans-serif; font-size: 10px !important;}</style><body>";

        return "";
    }

    inline std::string EndPageFooter(eToStringFormat format)
    {
        if (format == kHTML)
            return "</body></html>";

        return "";
    }

    inline std::string StartSection(eToStringFormat format)
    {
        if (format == kHTML)
            return "<table border=1>";

        return "";
    }

    inline std::string EndSection(eToStringFormat format)
    {
        if (format == kHTML)
            return "</table>";

        return "";
    }


	inline std::string StartDelimiter(eToStringFormat format, int32_t nSpan = 1)
	{
		if (format == kHTML)
			return "<tr><td colspan=\"" + std::to_string(nSpan) + "\">";

		return "";
	}

	inline std::string Separator(eToStringFormat format)
	{
		switch (format)
		{
		case kHTML:
			return "</td><td>";
			break;
		case kTabs:
			return "\t";
			break;
		}

		return ",";
	}

	inline std::string EndDelimiter(eToStringFormat format)
	{
		if (format == kHTML)
			return "</td></tr>\n";

		return "\n";
	}

	inline std::string NextLine(eToStringFormat format)
	{
		if (format == kHTML)
			return "<br>\n";

		return "\n";
	}

	template <class ...A>
    std::string FormatStrings(eToStringFormat format, A... arg)
	{

		int32_t numArgs = sizeof...(arg);
        std::list<std::string> stringList = { arg... };

        std::string sReturn(StartDelimiter(format));

		if (numArgs > 0)
		{
            std::list<std::string>::iterator it = stringList.begin();
			for (int32_t i = 0; i < numArgs - 1; i++) // add all but the last one with the separator trailing
				sReturn += (*it++) + Separator(format);

			sReturn += (*it);
		}

		sReturn += EndDelimiter(format);

		return sReturn;
	}
    

    //////////////////////////////////////////////////////////////////////////////////////////////
    struct sSizeEntry
    {
        const char* label;
        int64_t     value;
    };

    static const int64_t kAuto = 0LL;
    static const int64_t kBytes = 1LL;
    static const int64_t kK = 1000LL;
    static const int64_t kKB = 1000LL;
    static const int64_t kKiB = 1024LL;

    static const int64_t kM = 1000LL * 1000LL;
    static const int64_t kMB = 1000LL * 1000LL;
    static const int64_t kMiB = 1024LL * 1024LL;

    static const int64_t kG = 1000LL * 1000LL * 1000LL;
    static const int64_t kGB = 1000LL * 1000LL * 1000LL;
    static const int64_t kGiB = 1024LL * 1024LL * 1024LL;

    static const int64_t kT = 1000LL * 1000LL * 1000LL * 1000LL;
    static const int64_t kTB = 1000LL * 1000LL * 1000LL * 1000LL;
    static const int64_t kTiB = 1024LL * 1024LL * 1024LL * 1024LL;

    static const int64_t kP = 1000LL * 1000LL * 1000LL * 1000LL * 1000LL;
    static const int64_t kPB = 1000LL * 1000LL * 1000LL * 1000LL * 1000LL;
    static const int64_t kPiB = 1024LL * 1024LL * 1024LL * 1024LL * 1024LL;


    static const sSizeEntry sizeEntryTable[] =
    {
        { "B"     , 1 },

        { "K"     , kK},
        { "KB"    , kKB},
        { "KIB"   , kKiB},

        { "M"     , kM},
        { "MB"    , kMB},
        { "MIB"   , kMiB},

        { "G"     , kG},
        { "GB"    , kGB},
        { "GIB"   , kGiB},

        { "T"     , kT},
        { "TB"    , kTB},
        { "TIB"   , kTiB},

        { "P"     , kP},
        { "PB"    , kPB},
        { "PIB"   , kPiB}
    };

    static const int sizeEntryTableSize = sizeof(sizeEntryTable) / sizeof(sSizeEntry);



    std::string FormatFriendlyBytes(uint64_t nBytes, int64_t sizeType = kAuto);
    std::string UserReadableFromInt(int64_t nValue);
    int64_t IntFromUserReadable(std::string sReadable);
};

