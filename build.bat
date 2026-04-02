@echo off
set "MSVC=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717"
set "WINSDK=C:\Program Files (x86)\Windows Kits\10"
set "SDKVER=10.0.26100.0"
set "VSCMAKE=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake"

set "PATH=%MSVC%\bin\Hostx64\x64;%VSCMAKE%\CMake\bin;%VSCMAKE%\Ninja;%WINSDK%\bin\%SDKVER%\x64;%PATH%"
set "INCLUDE=%MSVC%\include;%WINSDK%\Include\%SDKVER%\ucrt;%WINSDK%\Include\%SDKVER%\um;%WINSDK%\Include\%SDKVER%\shared;%INCLUDE%"
set "LIB=%MSVC%\lib\x64;%WINSDK%\Lib\%SDKVER%\um\x64;%WINSDK%\Lib\%SDKVER%\ucrt\x64;%LIB%"

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
if %errorlevel% neq 0 exit /b %errorlevel%

cmake --build build
if %errorlevel% neq 0 exit /b %errorlevel%

echo.
echo === Build succeeded ===
echo Running tests...
echo.
ctest --test-dir build --output-on-failure
