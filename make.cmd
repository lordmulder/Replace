@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

set "MSVC_PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community"
set "WSDK_PATH=%ProgramFiles(x86)%\Microsoft SDKs\Windows\v7.1A"

REM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
REM Setup environment
REM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

if not exist "%MSVC_PATH%\VC\Auxiliary\Build\vcvarsall.bat" (
	echo VisualC++ could not be found. Please check MSVC_PATH and try again ^^!^^!^^!
	goto:BuildHasFailed
)

if not exist "%WSDK_PATH%\Include\Windows.h" (
	echo Windows SDK could not be found. Please check WSDK_PATH and try again ^^!^^!^^!
	goto:BuildHasFailed
)

call "%MSVC_PATH%\VC\Auxiliary\Build\vcvarsall.bat" x64_x86 8.1

where cl.exe
if not "%ERRORLEVEL%"=="0" (
	echo VisualC++ could not be found on path ^^!^^!^^!
	goto:BuildHasFailed
)

set "INCLUDE=%WSDK_PATH%\Include;%INCLUDE%"
set "LIB=%WSDK_PATH%\Lib;%LIB%"
set "PATH=%WSDK_PATH%\Bin;%PATH%"

REM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
REM Clean-up
REM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

for %%i in (bin,obj,.vs) do (
	rmdir /Q /S "%~dp0.\%%i" 2> NUL
	if exist "%~dp0.\%%i\" (
		echo Failed to delete existing "%~dp0.\%%i" directory!
		goto:BuildHasFailed
	)
)

REM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
REM Build!
REM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

set "MSBuildUseNoSolutionCache=1"

for %%p in (Win32,x64) do (
	for %%c in (Release,Release_SSE2,Release_AVX,Debug) do (
		MSBuild /property:Platform=%%~p /property:Configuration=%%~c /target:Clean   "%~dp0\replace.sln"
		if not "!ERRORLEVEL!"=="0" goto:BuildHasFailed
		MSBuild /property:Platform=%%~p /property:Configuration=%%~c /target:Rebuild "%~dp0\replace.sln"
		MSBuild /property:Platform=%%~p /property:Configuration=%%~c /target:Build   "%~dp0\replace.sln"
		if not "!ERRORLEVEL!"=="0" goto:BuildHasFailed
	)
)

for %%p in (Win32,x64) do (
	for %%c in (Release,Release_SSE2,Release_AVX) do (
		for %%f in (%~dp0.\bin\%%~p\%%~c\*.exe) do (
			"%~dp0.\etc\utils\win32\rchhdrrsr\rchhdrrsr.exe" "%%~f"
		)
	)
)

echo.
echo BUILD COMPLETED.
echo.

if not "%MAKE_NONINTERACTIVE%"=="1" pause
exit /B 0

REM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
REM Failed
REM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:BuildHasFailed

echo.
echo BUILD HAS FAILED ^^!^^!^^!
echo.

if not "%MAKE_NONINTERACTIVE%"=="1" pause
exit /B 1
