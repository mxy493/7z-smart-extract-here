@echo off
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Please run as Administrator
    pause
    exit /b 1
)

set "DLL_PATH=%~dp07zSmartExtractHere.dll"
regsvr32 /u /s "%DLL_PATH%"
if %errorlevel% equ 0 (
    echo Uninstall succeeded.
    echo Note: You may need to restart Explorer to refresh the context menu.
) else (
    echo Uninstall done (DLL may not have been registered)
)
pause
