# ZZipUpdate
A cross platform utility for creating, exploring, diffing, extracting and syncing ZIP files both local and on remote HTTP servers.

The Windows binary is included.



# Requirements

To build you will need to get ahold of boost. It was build with 1.69.0 but a newer version should not be a problem to use. (Check readme.txt in the boost folder for instructions on building your own local libraries.)
You will also need to get ahold of openssl in order to build boost libraries with SSL support. (Check readme.txt in the openssl folder for instructions. Currently only the win32 build is working.
zlib version 1.2.8 is included but you can update it if necessary.


# Main Features

* ZIP file support
* ZIP64 support (for very large zip archives.)
* Simple use and simple code to use in your own projects. Perform in-memory or disk to disk compression/decompression.
* Deflate support
* Multithreaded, job system based.
* Supports HTTP and HTTPS connections.
* Very fast
* Uses boost for file IO and networking.
* Uses OpenSSL for TLS support.
* Uses FastCRC for very fast CRC calculation.


# Multithreaded

  The utility performs multithreaded extraction/syncing/verification for taking advantage of SSDs having no seek times making operations extremely fast.

  For example:
    Test ZIP file of 1.68GB containing 499 folders and 3962 files compressed to 58% of normal size.

    Extraction time: 7-Zip      takes 2m 13s
                     ZZipUpdate takes 0m 19s
            
    Once extracted, to do another sync (verifying CRCs of all files) takes ZZipUpdate only 1.7s

# Remote Zip File Access
  
You can extract one, all or some wildcard matching set of files from a remotely hosted zip file. You don't have to download the entire zip file first. So for particularly large zip archives this is a very fast and convenient way of listing or extracting files.
  
# Sync

The utility can compare the contents of a locally extracted subtree of files to a zip file (either local or remote) and extract only the files that are different. This can be used for a very fast and easy way of keeping your local files in sync with a published zip archive. Only the ZIP central directory is used for doing the file comparisons so in cases where no files have changed the entire operation can complete in milliseconds. An application could use this functionality on startup to perform a quick self-update.
  
# Diff

  The utility can diff files in a local subtree and zip file and report what's different. Which files/folders exist only on the disk vs which ones are only in the ZIP archive and which are different.
  The reports can be output in tab, commas or HTML for easy reading or use by other applications.
  

# Work in Progress
 It's a work in progres so there are still features I would love to add or see added.
 -Unicode support
 -Additional compression methods


# ZZipUpdate.exe Usage Examples

The following will download any files missing or different from c:/example that are in the package:
    
    ZZipUpdate.exe -sync -pkg:https://www.mysecuresite.com/latest/files.zip -path:c:/example

The following will extract all jpg files with 8 threads:
    
    ZZipUpdate.exe -extract -pkg:d:/downloads/pictures.zip -path:c:/albums -threads:8 -pattern:*.jpg

The follwing will report differences between a path and a package and create an HTML report called results.html:

    ZZipUpdate.exe -diff -pkg:http://www.mysite.com/game_1.5.2.zip -path:"c:/Program Files (x86)/Game/" -outputformat:html > results.html

The following will list the contents of a zip file on a server in a comma delimited format

    ZZipUpdate.exe -list -pkg:https://www.mysite.com/sample.zip -outputformat:commas

The following will create a new zip archive and all JPG files that contain "Maui" in the specified path:

    ZZipUpdate.exe -create -pkg:c:/temp/mytrip.zip -path:"f:\My Albums\2019\Hawaii Trip\" -pattern:*Maui*.jpg


# Code Usage Examples

There are several ways in which you can use these classes.
At the highest level you can instantiate a ZZipJob object, configure it and let it run. Querying it for status or calling Join when you want to wait for it to complete.

    ZipJob newJob(ZipJob::kExtract);
    newJob.SetBaseFolder(L"c:/output");
    newJob.SetURL(L"c:/downloads/files.zip");
    newJob.Run();
    newJob.Join();


You can also use ZZipAPI directly:

    ZZipAPI zipAPI;
    zipAPI.Init(L"http://www.sample.net/files.zip");
    zipAPI.DecompressToBuffer("readme.txt", pOutputBuffer);


