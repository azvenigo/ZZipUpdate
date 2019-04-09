In order to build ZZipUpdate you'll need the boost libraries here. 


Following directions from:
https://www.boost.org/doc/libs/1_69_0/more/getting_started/windows.html#build-from-the-visual-studio-ide

1) Get ahold of 1_69_0 and unzip into the path:
%git_path_for_project%/common/boost_1_69_0/


Open a Visual Studio command prompt
Run the following:
cd %git_path_for_project%/common/boost_1_69_0/
bootstrap.bat
.\b2


This will take some time but should build the requisite libraries needed for linking.