@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0\.."

REM -----------------------------------------------------------------------------
REM Script: build_clean.cmd
REM Description: Invokes CustomBuildTool cleanup for the repository.
REM -----------------------------------------------------------------------------

REM Initialize script state and tool paths.
set "ExitCode=0"
set "IsCI=false"
set "CustomBuildTool=tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe"

REM Run the main script flow and capture the final exit code.
call :DetectCi
call :Main
if errorlevel 1 set "ExitCode=%errorlevel%"

:end
REM Pause only for interactive, non-CI invocations before returning.
if /i "%IsCI%"=="false" call :PauseIfInteractive
Endlocal & exit /b %ExitCode%

REM -----------------------------------------------------------------------------
REM Function: Main
REM Description: Validates prerequisites and runs the cleanup action.
REM -----------------------------------------------------------------------------
:Main
call :CheckCustomBuildTool
if errorlevel 1 exit /b %errorlevel%

call :RunCustomBuildTool "-cleanup"
if errorlevel 1 exit /b %errorlevel%

call :RemoveCustomBuildToolBin
if errorlevel 1 exit /b %errorlevel%

call :RemovePortableDirectory
if errorlevel 1 exit /b %errorlevel%

exit /b 0

REM -----------------------------------------------------------------------------
REM Function: CheckCustomBuildTool
REM Description: Ensures the CustomBuildTool executable is available.
REM -----------------------------------------------------------------------------
:CheckCustomBuildTool
if exist "%CustomBuildTool%" exit /b 0
echo CustomBuildTool.exe not found. Run build\build_init.cmd first.
exit /b 1

REM -----------------------------------------------------------------------------
REM Function: RunCustomBuildTool
REM Description: Executes CustomBuildTool with the supplied arguments.
REM Parameters:
REM   %* - Arguments forwarded to CustomBuildTool.
REM -----------------------------------------------------------------------------
:RunCustomBuildTool
start /B /W "" "%CustomBuildTool%" %*
exit /b %errorlevel%

REM -----------------------------------------------------------------------------
REM Function: RemoveCustomBuildToolBin
REM Description: Removes the CustomBuildTool bin tree. The running tool cannot
REM              delete its own image during -cleanup, so this is done here by
REM              the script after the tool has exited.
REM -----------------------------------------------------------------------------
:RemoveCustomBuildToolBin
if not exist "tools\CustomBuildTool\bin" exit /b 0
rd /s /q "tools\CustomBuildTool\bin"
if exist "tools\CustomBuildTool\bin" (
    echo Failed to remove tools\CustomBuildTool\bin. Ensure no CustomBuildTool instance is running.
    exit /b 1
)
echo CustomBuildTool bin removed. Run build\build_init.cmd to rebuild build tools.
exit /b 0

REM -----------------------------------------------------------------------------
REM Function: RemovePortableDirectory
REM Description: Removes the bin\portable directory produced by portable builds.
REM -----------------------------------------------------------------------------
:RemovePortableDirectory
if not exist "bin\portable" exit /b 0
rd /s /q "bin\portable"
if exist "bin\portable" (
    echo Failed to remove bin\portable. Ensure no portable instance is running.
    exit /b 1
)
exit /b 0

REM -----------------------------------------------------------------------------
REM Function: DetectCi
REM Description: Detects whether the script is running under CI.
REM -----------------------------------------------------------------------------
:DetectCi
if /i "%GITHUB_ACTIONS%"=="true" set "IsCI=true"
if /i "%TF_BUILD%"=="true" set "IsCI=true"
exit /b 0

REM -----------------------------------------------------------------------------
REM Function: PauseIfInteractive
REM Description: Pauses only when stdin is attached to an interactive console.
REM -----------------------------------------------------------------------------
:PauseIfInteractive
set "STDIN_REDIRECTED=False"
for /f %%i in ('powershell -NoProfile -Command "[Console]::IsInputRedirected"') do set "STDIN_REDIRECTED=%%i"
if /i not "%STDIN_REDIRECTED%"=="True" pause
exit /b 0
