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
wstring             gsPackageURL;	                            // source package URL
wstring             gsBaseFolder;                               // base folder. (default is the folder of ZZipUpdate.exe)
wstring             gsPattern;                                  // wildcard pattern to match (example "*base*/*.exe"  matches all directories that have the string "base" in them and in those directories all files that end in .exe)
bool                gbSkipCRC		= false;                    // Whether to bypass CRC checks when doing sync
bool                gbKill			= false;                    // TBD
uint32_t            gNumThreads		= 6;	                    // Multithreaded sync/extraction
eToStringFormat     gOutputFormat	= kTabs;                    // For lists or diff operations, output in various formats
bool                gbVerbose       = false;                    // Diagnostics. Forces single threaded operation and spits out a lot of logging data.

bool ParseParams(int argc, _TCHAR* argv[])
{

	auto parseParamsFunc = [](std::list<wstring>& params, std::list<wstring>& flags)
	{
        const wstring sCreateCommand(L"create");
        const wstring sExtractCommand(L"extract");
        const wstring sListCommand(L"list");
        const wstring sDiffCommand(L"diff");
		const wstring sSyncCommand(L"sync");

        const wstring sPatternFlag(L"-pattern:");
		const wstring sSkipCRCFlag(L"-skipcrc");
		const wstring sKillFlag(L"-kill");
		const wstring sOutputFormatFlag(L"-outputformat:");
		const wstring sThreadsFlag(L"-threads:");
        const wstring sVerboseFlag(L"-verbose");

        // first parameter is the launched app itself. Don't need it
        params.pop_front();

        // next parameter should be the command
        wstring sCommand = *params.begin();
        params.pop_front();

        makelower(sCommand);

        uint32_t nRequiredParameters = 0;

        if (sCommand.find(sListCommand) == 0)
        {
            gCommand = ZipJob::kList;
            nRequiredParameters = 1;
        }
        else if (sCommand.find(sDiffCommand) == 0)
        {
            gCommand = ZipJob::kDiff;
            nRequiredParameters = 2;
        }
        else if (sCommand.find(sCreateCommand) == 0)
        {
            gCommand = ZipJob::kCompress;
            nRequiredParameters = 2;
        }
        else if (sCommand.find(sSyncCommand) == 0)
        {
            gCommand = ZipJob::kExtract;
            gbSkipCRC = false;      // Sync job is just decompress while checking CRCs
            nRequiredParameters = 2;
        }
        else if (sCommand.find(sExtractCommand) == 0)
        {
            gCommand = ZipJob::kExtract;
            gbSkipCRC = true;       // Bypass any CRC checks, force overwrite
            nRequiredParameters = 2;
        }
        else
        {
            // command not found
            wcout << "ERROR: Unknown command: \"" << sCommand << "\"\n";
            return false;
        }

        if (params.size() < nRequiredParameters)
        {
            wcout << "ERROR: Too few parameters specified for command: \"" << sCommand << "\"\n";
            return false;
        }

        // next parameter should be the URL or file
        gsPackageURL = *params.begin();
        params.pop_front();

        if (gsPackageURL.find(L"?") != std::string::npos ||
            gsPackageURL.find(L"*") != std::string::npos)
        {
            wcout << "ERROR: URL \"" << gsPackageURL << "\" includes illegal wildcards.\n";
            return false;
        }



        // If the command is extract, sync, create or diff then the next parameter should be the folder
        if (gCommand == ZipJob::kDiff ||
            gCommand == ZipJob::kCompress ||
            gCommand == ZipJob::kExtract)
        {
            gsBaseFolder = *params.begin();
            params.pop_front();
        }

        if (gsBaseFolder.find(L"?") != std::string::npos ||
            gsBaseFolder.find(L"*") != std::string::npos)
        {
            wcout << "ERROR: Folder \"" << gsBaseFolder << "\" includes illegal wildcards.\n";
            return false;
        }

        // If there are any more parameters it must be a pattern
        if (!params.empty())
        {
            gsPattern = *params.begin();
            params.pop_front();
        }

        if (!params.empty())
        {
            wcout << "WARNING: Ignoring extra specified parameters\n";
            for (auto param : params)
                wcout << param << "\n";
        }


        // New command line format
        // -list URL [PATTERN] [optional params]
        // -extract URL FOLDER [PATTERN] [optional params]
        // -sync    URL FOLDER [PATTERN] [optional params]
        // -create  URL FOLDER [PATTERN] [optional params]
        // -diff    URL FOLDER [optional params]
        //
        // Optional Params
        // -skipcrc
        // -kill
        // -outputformat
        // -threads
        // -verbose

        // parse any specified flags
        for (std::list<wstring>::iterator it = flags.begin(); it != flags.end(); it++)
        {
            wstring sFlag(*it);

            if (sFlag.find(sSkipCRCFlag) == 0)
                gbSkipCRC = true;
            else if (sFlag.find(sKillFlag) == 0)
                gbKill = true;
            else if (sFlag.find(sVerboseFlag) == 0)
                gbVerbose = true;
            else if (sFlag.find(sOutputFormatFlag) == 0)
            {
                wstring sFormat(sFlag.substr(sOutputFormatFlag.length()));
                if (sFormat == L"html")
                    gOutputFormat = kHTML;
                else if (sFormat == L"tabs")
                    gOutputFormat = kTabs;
                else if (sFormat == L"commas")
                    gOutputFormat = kCommas;
                else
                    gOutputFormat = kUnknown;
            }
            else if (sFlag.find(sThreadsFlag) == 0)
            {
                wstring sNumThreads(sFlag.substr(sThreadsFlag.length()));
                gNumThreads = boost::lexical_cast<int32_t> (sNumThreads);
            }
        }

        return true;
	};


    std::list<wstring> params;
    std::list<wstring> flags;

    // separate the flags from the paremeters
    for (int i = 0; i < argc; i++)
    {
        if (argv[i][0] == L'-')      // flags all start with '-'
            flags.push_back(wstring(argv[i]));
        else
            params.push_back(wstring(argv[i]));
    }

    return parseParamsFunc(params, flags);
}

int _tmain(int argc, _TCHAR* argv[])
{
//	_CrtMemState s1;
//	_CrtMemCheckpoint(&s1);

	// validate commands
    bool bShowUsage = !ParseParams(argc, argv);


    // New command line format
    // list URL [PATTERN] [optional params]
    // extract URL FOLDER [PATTERN] [optional params]
    // sync    URL FOLDER [PATTERN] [optional params]
    // create  URL FOLDER [PATTERN] [optional params]
    // diff    URL FOLDER [optional params]
    //
    // Flags
    // -skipcrc
    // -kill
    // -outputformat
    // -threads
    // -verbose





	if (bShowUsage)
	{
        wcout << "\n";
        wcout << "============================================================================\n";
		wcout << "                                     *Usage*\n\n";
        wcout << "----------------------------------------------------------------------------\n";
        wcout << "-Commands-\n";
        wcout << "ZZipUpdate list    URL [PATTERN] [flags]\n";
        wcout << "ZZipUPdate extract URL FOLDER [PATTERN] [flags]\n";
        wcout << "ZZipUpdate sync    URL FOLDER [PATTERN] [flags]\n";
        wcout << "ZZipUpdate create  URL FOLDER [PATTERN] [flags]\n";
        wcout << "ZZipUpdate diff    URL FOLDER [flags]\n";
        wcout << "----------------------------------------------------------------------------\n";
        wcout << "-Flags-\n";
		wcout << "-skipcrc              Skip CRC checks for matching files and overwrite everything. (note: The extract and sync commands are identical except extract automatically skips CRC checking.)\n";
		//wcout << "-kill                 Kills any processes that may be holding destination files locked. <TBD>\n";
		wcout << "-outputformat:FORMAT  one of {html, commas, tabs} for outputing list or diff\n";
        wcout << "-threads:NUM          Number of threads to use. Default: 6 (note: -create is single threaded since data written to the zip file must be serial.)\n";
        wcout << "-verbose              Noisy logging for diagnostic purposes. (note: can slow down operations significantly. Also forces single threaded operation.)\n";
        wcout << "----------------------------------------------------------------------------\n";
        wcout << "note: In the following, URL can be a fully qualified web URL or a local path.\n";
        wcout << "      For example the following are all ok:\n";
        wcout << "http://www.example.com/sample.zip\n";
        wcout << "https://www.example.com/sample.zip\n";
        wcout << "c:/example/sample.zip\n";
        wcout << "\n============================================================================\n";
        wcout << "\n";

        return -1;
	}


    ZipJob newJob(gCommand);
    newJob.SetBaseFolder(gsBaseFolder);
    newJob.SetURL(gsPackageURL);
    newJob.SetSkipCRC(gbSkipCRC);
    newJob.SetNumThreads(gNumThreads);
    newJob.SetOutputFormat(gOutputFormat);
    newJob.SetPattern(gsPattern);
    newJob.SetKillHoldingProcess(gbKill);
    newJob.SetVerbose(gbVerbose);

    newJob.Run();
    newJob.Join();  // will output progress to cout until completed

    cout << std::flush;
    wcout << std::flush;

//	_CrtMemDumpStatistics(&s1);  
//	_CrtDumpMemoryLeaks();

	return 0;

}

