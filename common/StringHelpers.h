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
#include <boost/lexical_cast.hpp>

using namespace std;

namespace StringHelpers
{
    //////////////////////////////////////////////////////////////////////////////////////////////
    // Common conversions
    // some commonly used conversions
    string	int_to_hex_string(uint32_t nVal);
    string	int_to_hex_string(uint64_t nVal);
    string	binary_to_hex(uint8_t* pBuf, int32_t nLength);

    

    inline string  wstring_to_string(const wstring& rhs)
    {
        using convert_typeX = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<convert_typeX, wchar_t> converterX;

        return converterX.to_bytes(rhs);
    }

    inline wstring string_to_wstring(const string& rhs)
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

    inline string StartPageHeader(eToStringFormat format)
    {
        if (format == kHTML)
            return "<html><style> *{ font-family: 'Lucida Console', Lucida, sans-serif; font-size: 10px !important;}</style><body>";

        return "";
    }

    inline string EndPageFooter(eToStringFormat format)
    {
        if (format == kHTML)
            return "</body></html>";

        return "";
    }

    inline string StartSection(eToStringFormat format)
    {
        if (format == kHTML)
            return "<table border=1>";

        return "";
    }

    inline string EndSection(eToStringFormat format)
    {
        if (format == kHTML)
            return "</table>";

        return "";
    }


	inline string StartDelimiter(eToStringFormat format, int32_t nSpan = 1)
	{
		if (format == kHTML)
			return "<tr><td colspan=\"" + to_string(nSpan) + "\">";

		return "";
	}

	inline string Separator(eToStringFormat format)
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

	inline string EndDelimiter(eToStringFormat format)
	{
		if (format == kHTML)
			return "</td></tr>\n";

		return "\n";
	}

	inline string NextLine(eToStringFormat format)
	{
		if (format == kHTML)
			return "<br>\n";

		return "\n";
	}

	template <class ...A>
	string FormatStrings(eToStringFormat format, A... arg)
	{

		int32_t numArgs = sizeof...(arg);
		list<string> stringList = { arg... };

		string sReturn(StartDelimiter(format));

		if (numArgs > 0)
		{
			list<string>::iterator it = stringList.begin();
			for (int32_t i = 0; i < numArgs - 1; i++) // add all but the last one with the separator trailing
				sReturn += (*it++) + Separator(format);

			sReturn += (*it);
		}

		sReturn += EndDelimiter(format);

		return sReturn;
	}
    

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Readable byte sizes
    enum eSize
    {
        kSizeAuto   = 0,
        kSizeBytes  = 1,
        kSizeKB     = 2,
        kSizeMB     = 3,
        kSizeGB     = 4,
        kSizeTB     = 5
    };


    inline
    string FormatFriendlyBytes(uint64_t nBytes, eSize type = kSizeAuto)
    {
        const uint64_t kTB = 1024ull * 1024ull * 1024ull * 1024ull;
        const uint64_t kGB = 1024ull * 1024ull * 1024ull;
        const uint64_t kMB = 1024ull * 1024ull;
        const uint64_t kKB = 1024ull;

        if (type == kSizeAuto)
        {
            if (nBytes > kTB)   // TB show in GB
                type = kSizeGB;
            if (nBytes > kGB)	// GB show in MB
                type = kSizeMB;
            else if (nBytes > kMB)	// MB show in KB
                type = kSizeKB;
            else
                type = kSizeBytes;
        }

        switch (type)
        {
        case kSizeKB:
            return boost::lexical_cast<string> (nBytes / kKB) + "KB";
            break;
        case kSizeMB:
            return boost::lexical_cast<string> (nBytes / kMB) + "MB";
            break;
        case kSizeGB:
            return boost::lexical_cast<string> (nBytes / kGB) + "GB";
            break;
        }

        if (nBytes > kGB)       // return in MB
            return boost::lexical_cast<string> (nBytes / kMB) + "MB";
        else if (nBytes > kMB)  // return in KB
            return boost::lexical_cast<string> (nBytes / kKB) + "KB";

        return boost::lexical_cast<string> (nBytes) + "bytes";
    }


};

