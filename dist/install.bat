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

echo Unregistering old version (if any)...
regsvr32 /u /s "%DLL_PATH%"

echo Registering: %DLL_PATH%
regsvr32 "%DLL_PATH%"
if %errorlevel% equ 0 (
    echo Install succeeded.
) else (
    echo Install failed. Error code: %errorlevel%
)

echo.
echo Please restart Explorer for changes to take effect.
echo   - Run:  taskkill /f /im explorer.exe ^&^& start explorer.exe
echo.
pause
