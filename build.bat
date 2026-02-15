@echo off
setlocal

:: --- Configuration ---
SET OUT_DIR=build
SET OUT_NAME=%OUT_DIR%\tabs_scoreboard.exe
SET IMGUI_DIR=./imgui
SET BACKEND_DIR=./imgui/backends

:: Create the build directory if it doesn't exist
if not exist %OUT_DIR% mkdir %OUT_DIR%

:: --- Sync Assets ---
:: This copies everything from an 'assets' folder to your 'build' folder
:: /I creates the folder if it doesn't exist, /Y overwrites without asking
if exist "assets" (
    echo Syncing assets...
    xcopy /y /i /e "assets" "%OUT_DIR%\assets" >nul
)

echo Building project...

:: --- Compilation ---
:: /Fo%OUT_DIR%\  tells clang-cl to put all .obj files in the build folder
clang-cl ^
    -std:c++17 ^
    win32_dx11_boilerplate.cpp ^
    TABSScoreBoard.cpp ^
    %IMGUI_DIR%/*.cpp ^
    %BACKEND_DIR%/imgui_impl_win32.cpp ^
    %BACKEND_DIR%/imgui_impl_dx11.cpp ^
    /I %IMGUI_DIR% ^
    /I %BACKEND_DIR% ^
    /EHsc ^
    /Zi ^
    /Fo%OUT_DIR%\ ^
    /Fe:%OUT_NAME% ^
    /link ^
    user32.lib d3d11.lib d3dcompiler.lib dwmapi.lib gdi32.lib shell32.lib

:: --- Run if build succeeded ---
if %ERRORLEVEL% EQU 0 (
    echo Build Successful!
    copy imgui.ini %OUT_DIR% >nul 2>&1
    .\%OUT_NAME%
) else (
    echo Build Failed!
    pause
)

endlocal