@echo off
set "MSVC=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717"
set "WINSDK=C:\Program Files (x86)\Windows Kits\10"
set "SDKVER=10.0.26100.0"

set "PATH=%MSVC%\bin\Hostx64\x64;%PATH%"
set "INCLUDE=%MSVC%\include;%WINSDK%\Include\%SDKVER%\ucrt;%WINSDK%\Include\%SDKVER%\um;%WINSDK%\Include\%SDKVER%\shared;%INCLUDE%"
set "LIB=%MSVC%\lib\x64;%WINSDK%\Lib\%SDKVER%\um\x64;%WINSDK%\Lib\%SDKVER%\ucrt\x64;%LIB%"

cl /nologo /EHsc /std:c++17 /utf-8 /DNOMINMAX /I"src" /DUNICODE /D_UNICODE ^
  tests\test_analyzer.cpp src\ArchiveAnalyzer.cpp ^
  /link shell32.lib ole32.lib shlwapi.lib advapi32.lib ^
  /out:test_analyzer.exe

if %errorlevel% equ 0 (
    echo.
    echo === Running tests ===
    test_analyzer.exe
)
