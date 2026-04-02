@echo off
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Please run as Administrator
    pause
    exit /b 1
)

set "DLL_PATH=%~dp07zSmartExtractHere.dll"
if not exist "%DLL_PATH%" (
    echo DLL not found: %DLL_PATH%
    echo Please build the project first
    pause
    exit /b 1
)

echo Registering: %DLL_PATH%
regsvr32 "%DLL_PATH%"
if %errorlevel% equ 0 (
    echo Install succeeded.
) else (
    echo Install failed. Error code: %errorlevel%
)
pause
