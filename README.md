# ZZipUpdate
A cross platform utility for creating, exploring, diffing, extracting and syncing ZIP files both local and on remote HTTP servers.

The Windows binary is included.


# Latest Updates
* As of May 3, 2019 I added an HTTPCache which greatly improves performance in conditions where there are many small reads to a server. 


# Requirements

To build you will need to get ahold of boost. It was build with 1.69.0 but a newer version should not be a problem to use. (Check readme.txt in the boost folder for instructions on building your own local libraries.)
You will also need to get ahold of openssl in order to build boost libraries with SSL support. (Check readme.txt in the openssl folder for instructions. Currently only the win32 build is working.
zlib version 1.2.8 is included but you can update it if necessary.


# Main Features

* Cross Platform
  * Uses boost for file IO and networking.
  * Uses OpenSSL for TLS support.
* ZIP file support
  * ZIP64 support (for very large zip archives.)
  * Deflate support
* Simple use and simple code to use in your own projects. 
  * Perform in-memory or disk to disk compression/decompression.
* Secure and flexible
  * Supports HTTP and HTTPS connections.
* Very fast
  * Multithreaded, job system based.
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
Sync can also be used as a form of "repair" for an application in that any locally modified, incomplete or corrupt files can be fixed.
  
# Diff

  The utility can diff files in a local subtree and zip file and report what's different. Which files/folders exist only on the disk vs which ones are only in the ZIP archive and which are different.
  The reports can be output in tab, commas or HTML for easy reading or use by other applications.

# Examine
  The utility can list the contents of ZIP archives without downloading them.

# Asset Update
  You can embed the code in your own application to perform updates of assets at any time, on startup or on demand.
  
# Self Update
  The utility can be used for your application to do a fast self update. See "Usage" for details.

# Work in Progress
 It's a work in progres so there are still features I would love to add or see added.
 -Unicode support
 -Additional compression methods
 -Support for killing applications holding files open during extraction

# Usage
Check the Wiki page: https://github.com/azvenigo/ZZipUpdate/wiki



