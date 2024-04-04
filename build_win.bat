@if not exist build\CMake\Debug mkdir build\CMake\Debug
@if not exist build\CMake\Release mkdir build\CMake\Release
@if not exist bin\Debug mkdir bin\Debug
@if not exist bin\Release mkdir bin\Release

@if "%~1"=="Debug" GOTO :BUILD_AND_RUN_DEBUG
@if "%~1"=="debug" GOTO :BUILD_AND_RUN_DEBUG
@if "%~1"=="--debug" GOTO :BUILD_AND_RUN_DEBUG
@if "%~1"=="Release" GOTO :BUILD_AND_RUN_RELEASE
@if "%~1"=="release" GOTO :BUILD_AND_RUN_RELEASE
@if "%~1"=="--release" GOTO :BUILD_AND_RUN_RELEASE

@ECHO "ERROR: No build mode specified (Debug/Release)"
@GOTO :END

:BUILD_AND_RUN_DEBUG
@cd build\CMake\Debug
cmake -DCMAKE_BUILD_TYPE=Debug -S ../../../ -B . -G "MinGW Makefiles" 
mingw32-make.exe && mingw32-make.exe Shaders
@cd ../../../bin\Debug
sumire
@cd ../../
@GOTO :END

:BUILD_AND_RUN_RELEASE
@cd build\CMake\Release
cmake -DCMAKE_BUILD_TYPE=Release -S ../../../ -B . -G "MinGW Makefiles" 
mingw32-make.exe && mingw32-make.exe Shaders
@cd ../../../bin\Release
sumire
@cd ../../
@GOTO :END

:END
@PAUSE