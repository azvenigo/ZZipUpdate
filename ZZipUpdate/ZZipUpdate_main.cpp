// MIT License
// Copyright 2019 Alex Zvenigorodsky
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

/*#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>  */

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <iostream>
#include <tchar.h>
#include <locale>
#include <string>
#include <boost/lexical_cast.hpp>
#include "ZipHeaders.h"
#include "zlibAPI.h"
#include <boost/filesystem.hpp>
#include "ZipJob.h"

using namespace std;

// App Globals for reading command line
ZipJob::eJobType    gCommand        = ZipJob::eJobType::kNone;
string              gsPackageURL;	                            // source package URL
string              gsBaseFolder;                               // base folder. (default is the folder of ZZipUpdate.exe)
string              gsPattern;                                  // wildcard pattern to match (example "*base*/*.exe"  matches all directories that have the string "base" in them and in those directories all files that end in .exe)
bool                gbSkipCRC		= false;                    // Whether to bypass CRC checks when doing sync
bool                gbKill			= false;                    // TBD
uint32_t            gNumThreads		= 6;	                    // Multithreaded sync/extraction
eToStringFormat     gOutputFormat	= kTabs;                    // For lists or diff operations, output in various formats
bool                gbVerbose       = false;                    // Diagnostics. Forces single threaded operation and spits out a lot of logging data.

void ParseCommands(int argc, _TCHAR* argv[])
{
	auto parseCommandFunc = [](_TCHAR* pArg)
	{
		wstring sWParam(pArg);
		string sParam = wstring_to_string(sWParam);
        const string sCreateCommand("-create");
        const string sExtractCommand("-extract");
        const string sListCommand("-list");
        const string sDiffCommand("-diff");
		const string sSyncCommand("-sync");
		const string sZIPPath("-pkg:");
		const string sPathFlag("-path:");
        const string sPatternFlag("-pattern:");
		const string sSkipCRCFlag("-skipcrc");
		const string sKillFlag("-kill");
		const string sOutputFormat("-outputformat:");
		const string sThreads("-threads:");
        const string sVerbose("-verbose");


        if (sParam.find(sListCommand) == 0)
            gCommand = ZipJob::kList;
        else if (sParam.find(sDiffCommand) == 0)
            gCommand = ZipJob::kDiff;
        else if (sParam.find(sCreateCommand) == 0)
            gCommand = ZipJob::kCompress;
        else if (sParam.find(sSyncCommand) == 0)
        {
            gCommand = ZipJob::kExtract;
            gbSkipCRC = false;      // Sync job is just decompress while checking CRCs
        }
        else if (sParam.find(sExtractCommand) == 0)
        {
            gCommand = ZipJob::kExtract;
            gbSkipCRC = true;       // Bypass any CRC checks, force overwrite
        }

        if (sParam.find(sZIPPath) == 0)
            gsPackageURL.assign(sParam.substr(sZIPPath.length()));
        else if (sParam.find(sPathFlag) == 0)
            gsBaseFolder.assign(sParam.substr(sPathFlag.length()));
        else if (sParam.find(sPatternFlag) == 0)
            gsPattern.assign(sParam.substr(sPatternFlag.length()));
        else if (sParam.find(sSkipCRCFlag) == 0)
            gbSkipCRC = true;
        else if (sParam.find(sKillFlag) == 0)
            gbKill = true;
        else if (sParam.find(sVerbose) == 0)
            gbVerbose = true;
		else if (sParam.find(sOutputFormat) == 0)
		{
			string sOutputFormat(sParam.substr(sOutputFormat.length()));
			if (sOutputFormat == "html")
				gOutputFormat = kHTML;
			else if (sOutputFormat == "tabs")
				gOutputFormat = kTabs;
			else if (sOutputFormat == "commas")
				gOutputFormat = kCommas;
			else
				gOutputFormat = kUnknown;
		}
		else if (sParam.find(sThreads) == 0)
		{
			string sThreads(sParam.substr(sThreads.length()));
			gNumThreads = boost::lexical_cast<int32_t> (sThreads);
		}
	};


	for (int i = 0; i < argc; i++)
		parseCommandFunc(argv[i]);		
}

int _tmain(int argc, _TCHAR* argv[])
{
//	_CrtMemState s1;
//	_CrtMemCheckpoint(&s1);


	int nReturn = 0;
	ParseCommands(argc, argv);

	// validate commands
	bool bShowUsage = false;

    if (gCommand == ZipJob::kNone)
    {
        wcout << "Command not specified.\n";
        bShowUsage = true;
    }

    if (gsPackageURL.empty())
    {
        wcout << "Package not specified.\n";
        bShowUsage = true;
    }

	if (gOutputFormat == kUnknown)
	{
		wcout << "Error. Unknown output format!\n";
		bShowUsage = true;
	}

	if (bShowUsage)
	{
		wcout << "*commands*\n";
        wcout << "ZZipUpdate -list    -pkg:URL or PACKAGE_PATH\n";
        wcout << "ZZipUPdate -extract -pkg:URL or PACKAGE_PATH -path:BASE_PATH\n";
        wcout << "ZZipUpdate -sync    -pkg:URL or PACKAGE_PATH -path:BASE_PATH\n";
        wcout << "ZZipUpdate -diff    -pkg:URL or PACKAGE_PATH -path:BASE_PATH\n";
        wcout << "ZZipUpdate -create  -pkg:FILE_PATH           -path:BASE_PATH (note: URL package path is not supported for create. Only local packages can be created.)\n";
        wcout << "\n";

        wcout << "*parameters*\n";
		wcout << "-pkg:URL or PATH      package to extract from.\n";
        wcout << "-match:PATTERN        wildcard pattern to match.\n";
		wcout << "-path:PATH            Will use the location of ZZipUpdate.exe as a base path by default\n";
		wcout << "-skipcrc              Skip CRC checks for matching files and overwrite everything. (note: The -extract and -sync commands are identical except -extract automatically skips CRC checking.)\n";
		wcout << "-kill                 Kills any processes that may be holding destination files locked. <TBD>\n";
		wcout << "-outputformat:FORMAT  one of {html, commas, tabs} for outputing list or diff\n";
        wcout << "-threads:NUM          Number of threads to use. Default: 6 (note: -create is single threaded since data written to the zip file must be serial.)\n";
        wcout << "-verbose              Noisy logging for diagnostic purposes. (note: can slow down operations significantly. Also forces single threaded operation.)";

        wcout << "\n";
		wcout << "ENTER to continue...\n";
		wcin.get();

        return -1;
	}


    ZipJob newJob(gCommand);
    newJob.SetBaseFolder(string_to_wstring(gsBaseFolder));
    newJob.SetURL(string_to_wstring(gsPackageURL));
    newJob.SetSkipCRC(gbSkipCRC);
    newJob.SetNumThreads(gNumThreads);
    newJob.SetOutputFormat(gOutputFormat);
    newJob.SetPattern(string_to_wstring(gsPattern));
    newJob.SetKillHoldingProcess(gbKill);
    newJob.SetVerbose(gbVerbose);

    newJob.Run();
    newJob.Join();  // will output progress to cout until completed

    cout << std::flush;
    wcout << std::flush;

//	_CrtMemDumpStatistics(&s1);  
//	_CrtDumpMemoryLeaks();

	return nReturn;

}

