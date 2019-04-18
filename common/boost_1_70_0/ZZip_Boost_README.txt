If you want to save quite a bit of time and space building boost I recommend building only the libraries needed by this project. (regex, date_time, filesystem)
This will save you 80% of the space boost built libraries require. (Less the 1GB instead of over 5.5GB)

Instructions as follow for building with Visual Studio 2017

1) Get ahold of the latest boost build. (At the time of this doc it's 1.70)

2) Extract into
	%project_path%/common/boost_1_70_0/

3) Open the solution in Visual Studio and open up a Visual Studio Command Prompt from the "Tools" menu.
   
4) Create a temporary boost build folder.
   - in this example "c:/temp/boost_build_temp/"
  
5) Change the current directory to the boost folder.
   - %project_path%/common/boost_1_70_0/

6) Run bootstrap.bat

7) Run the following three commands
   
b2 --build-dir=c:/temp/boost_build_temp/ threading=multi --with-date_time toolset=msvc-14.1 --build-type=complete
b2 --build-dir=c:/temp/boost_build_temp/ threading=multi --with-regex toolset=msvc-14.1 --build-type=complete
b2 --build-dir=c:/temp/boost_build_temp/ threading=multi --with-filesystem toolset=msvc-14.1 --build-type=complete


8) Delete the temporary files in the boost build folder.

9) Compile ZZipUpdate in any combination of x86/x64, Debug/Release.