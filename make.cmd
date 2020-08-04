@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

set "MSVC_PATH=C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC"

REM ///////////////////////////////////////////////////////////////////////////
REM // Setup environment
REM ///////////////////////////////////////////////////////////////////////////

if not exist "%MSVC_PATH%\vcvarsall.bat" (
	echo VisualC++ could not be found. Please check MSVC_PATH and try again ^^!^^!^^!
	pause
	goto:eof
)

call "%MSVC_PATH%\vcvarsall.bat" x86

REM ///////////////////////////////////////////////////////////////////////////
REM // Clean up temp files
REM ///////////////////////////////////////////////////////////////////////////

for %%i in (bin,obj) do (
	rmdir /Q /S "%~dp0.\%%i" 2> NUL
)

REM ///////////////////////////////////////////////////////////////////////////
REM // Build the binaries
REM ///////////////////////////////////////////////////////////////////////////

set "MSBuildUseNoSolutionCache=1"

for %%p in (Win32,x64) do (
	for %%c in (Release,Release_SSE2,Debug) do (
		MSBuild /property:Platform=%%p /property:Configuration=%%c /target:Clean   "%~dp0\replace.sln"
		if not "!ERRORLEVEL!"=="0" goto:BuildHasFailed
		MSBuild /property:Platform=%%p /property:Configuration=%%c /target:Rebuild "%~dp0\replace.sln"
		MSBuild /property:Platform=%%p /property:Configuration=%%c /target:Build   "%~dp0\replace.sln"
		if not "!ERRORLEVEL!"=="0" goto:BuildHasFailed
	)
)

echo.
echo BUILD COMPLETED.
pause
goto:eof

REM ///////////////////////////////////////////////////////////////////////////
REM // Failed
REM ///////////////////////////////////////////////////////////////////////////

:BuildHasFailed
echo.
echo BUILD HAS FAILED ^^!^^!^^!
pause
goto:eof
