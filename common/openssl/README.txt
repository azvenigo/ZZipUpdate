Building OpenSSL on Windows is......challenging.

I'm including the Windows binaries in these paths for linking but for other platforms you'll need to supply them in this folder.

Here are the steps I needed to get the binaries built, but your mileage may vary.



1) Get the latest OpenSSL available. 
 - I built with 1.1.1-dev

2) Install ActiveState Perl
 - https://www.activestate.com/products/activeperl/downloads/

3) Install NASM 
 - I installed mine in c:\nasm\


4) Open Visual Studio command prompt

5) Ensure right environment vars are set
 - for 64 bit:'"C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat" amd64'

6) add NASM to your path.  
 - "set path=%path%;c:\nasm\"

7) Configure build parameters
 - "perl Configure VC-WIN64A"

8) Clean any artifacts that may interfere if needed. But this was an important step for me to get it fully built.
 - "nmake clean"

9) Build
 - nmake

10) Copy the built libraries listed above into the common/openssl/openssl-64/openssl/ (for 64bit)