@echo off
rem Build a portable package with the App\Data layout:
rem   build\output\SystemInformer-Portable\
rem     App\SystemInformer\        - program files (from bin\Release64)
rem     Data\                      - persistent settings (SystemInformer.exe.settings.json)
rem     SystemInformerPortable.exe - launcher (built from tools\PortableLauncher)
rem Note: Data\ is intentionally preserved across repacks.

setlocal EnableExtensions
cd /d "%~dp0.."

set "MSBUILD=D:\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"
if not exist "%MSBUILD%" set "MSBUILD=D:\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
if not exist "%MSBUILD%" (
    echo MSBuild not found. Set the MSBUILD variable to the amd64 MSBuild.exe path.
    exit /b 1
)

set "OUTPUT=build\output\SystemInformer-Portable"

rem Read the version of the built main executable and pass it to the
rem launcher resources, so the launcher version always matches.
if not exist "bin\Release64\SystemInformer.exe" (
    echo Main executable not found: bin\Release64\SystemInformer.exe
    exit /b 1
)
set "APPVER="
for /f "usebackq delims=" %%v in (`powershell -NoProfile -Command "(Get-Item -LiteralPath 'bin\Release64\SystemInformer.exe').VersionInfo.FileVersion"`) do set "APPVER=%%v"
if not defined APPVER (
    echo Failed to read the version of bin\Release64\SystemInformer.exe
    exit /b 1
)
for /f "tokens=1-4 delims=. " %%a in ("%APPVER%") do (
    set "VMAJOR=%%a"
    set "VMINOR=%%b"
    set "VBUILD=%%c"
    set "VREV=%%d"
)
echo Launcher version: %APPVER%

rem Rebuild the launcher (static CRT, windowed subsystem).
"%MSBUILD%" tools\PortableLauncher\PortableLauncher.vcxproj /p:Configuration=Release /p:Platform=x64 /p:ExternalPreprocessorOptions="PHAPP_VERSION_MAJOR=%VMAJOR%;PHAPP_VERSION_MINOR=%VMINOR%;PHAPP_VERSION_BUILD=%VBUILD%;PHAPP_VERSION_REVISION=%VREV%" /m /v:m /nologo
if errorlevel 1 exit /b 1
if not exist "tools\PortableLauncher\bin\Release64\SystemInformerPortable.exe" (
    echo Launcher exe not found after build.
    exit /b 1
)

if exist "%OUTPUT%\App" rmdir /s /q "%OUTPUT%\App"
if exist "%OUTPUT%\SystemInformerPortable.bat" del /f /q "%OUTPUT%\SystemInformerPortable.bat"
if exist "%OUTPUT%\SystemInformerPortable.exe" del /f /q "%OUTPUT%\SystemInformerPortable.exe"
mkdir "%OUTPUT%\App\SystemInformer" 2>nul
mkdir "%OUTPUT%\Data" 2>nul

robocopy bin\Release64 "%OUTPUT%\App\SystemInformer" /e /xf *.pdb /njh /njs /ndl /nfl
if errorlevel 8 (
    echo robocopy failed with code %errorlevel%
    exit /b 1
)

copy /y "tools\PortableLauncher\bin\Release64\SystemInformerPortable.exe" "%OUTPUT%\SystemInformerPortable.exe" >nul

echo.
echo Portable package created: %CD%\%OUTPUT%
exit /b 0
