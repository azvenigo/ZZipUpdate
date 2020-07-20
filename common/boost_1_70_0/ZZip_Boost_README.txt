Most of boost doesn't require built libraries except for a select few. ZZipUpdate does use three libraries (regex, date_time, filesystem) that need to be build.

If you want to save quite a bit of time and space building boost I recommend building only the libraries needed by this project. 
(This will save you 80% of the space boost built libraries require. Less the 1GB instead of over 5.5GB)



**Instructions as follow for building with Visual Studio 2017**

1) Get ahold of the latest boost build. 
    - At the time of this doc it's 1.70

2) Extract into the boost folder in the project
    - In this example: %project_path%/common/boost_1_70_0/

3) Open the solution in Visual Studio and open up a Visual Studio Command Prompt from the "Tools" menu.
   
4) Change the current directory to the boost folder.
    - %project_path%/common/boost_1_70_0/

5) Run bootstrap.bat

6) Run the following three commands
    - Note: In this example I specify at temporary boost build folder that can be deleted after buildling.
    - in this example "c:/temp/boost_build_temp/"

   
b2 --build-dir=c:/temp/boost_build_temp/ threading=multi --with-date_time toolset=msvc-14.1 --build-type=complete
b2 --build-dir=c:/temp/boost_build_temp/ threading=multi --with-regex toolset=msvc-14.1 --build-type=complete
b2 --build-dir=c:/temp/boost_build_temp/ threading=multi --with-filesystem toolset=msvc-14.1 --build-type=complete


7) Delete the temporary files in the boost build folder.
    - rmdir /s /q "c:/temp/boost_build_temp/"

8) Compile ZZipUpdate in any combination of x86/x64, Debug/Release.