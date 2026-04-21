@echo off
setlocal enabledelayedexpansion

set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Release

echo.
echo  ============================================
echo   AtlasMP Build Script
echo   Config: %CONFIG%
echo  ============================================
echo.

cd /d "%~dp0.."

echo [INFO] Initializing submodules...
git submodule update --init --recursive 2>nul

echo [INFO] Detecting Visual Studio version...
set VS_GENERATOR=
set VS_FOUND=

for /f "tokens=*" %%i in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul') do (
    set VS_PATH=%%i
    set VS_FOUND=1
)

if defined VS_FOUND (
    for /f "tokens=*" %%v in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property catalog_productLineVersion 2^>nul') do (
        set VS_VERSION=%%v
    )
    echo [INFO] Found: Visual Studio !VS_VERSION!
    if "!VS_VERSION!"=="2022" set VS_GENERATOR=Visual Studio 17 2022
    if "!VS_VERSION!"=="2026" set VS_GENERATOR=Visual Studio 18 2026
    if "!VS_VERSION!"=="2025" set VS_GENERATOR=Visual Studio 18 2026
)

if not defined VS_GENERATOR (
    echo [INFO] Using CMake default generator
    set VS_GENERATOR=
)

echo [INFO] Configuring CMake...
if defined VS_GENERATOR (
    cmake -B build -G "!VS_GENERATOR!" -A x64 -DCMAKE_BUILD_TYPE=%CONFIG% .
) else (
    cmake -B build -A x64 -DCMAKE_BUILD_TYPE=%CONFIG% .
)

if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed
    exit /b %ERRORLEVEL%
)

echo [INFO] Building...
cmake --build build --config %CONFIG% --parallel

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed
    exit /b %ERRORLEVEL%
)

echo.
echo  ============================================
echo   Build complete!
echo  ============================================
echo.
echo   Output locations:
echo     Client DLL  : .client\AtlasMP-Client.dll
echo     Server      : .server\AtlasMP-Server.exe
echo     Launcher    : .launcher\AtlasMP-Launcher.exe
echo     Injector    : .launcher\AtlasInject.exe
echo.
echo   To launch GTA V with AtlasMP:
echo     .launcher\AtlasMP-Launcher.exe --launch
echo.
echo   To test injection manually:
echo     .launcher\AtlasInject.exe .client\AtlasMP-Client.dll
echo.
