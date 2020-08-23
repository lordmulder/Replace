@echo off
cd /d "%~dp0"

REM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
REM Get current date
REM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

set "ISO_DATE="

for /F "usebackq tokens=1" %%a in (`start /WAIT /B "" "%~dp0.\etc\utils\win32\core-utils\date.exe" +"%%Y-%%m-%%d"`) do (
	set "ISO_DATE=%%a"
)

if "%ISO_DATE%"=="" (
	echo Failed to determine the current date!
	pause
	goto:eof
)

set "OUTFILE=%~dp0.\out\replace.%ISO_DATE%.zip"

REM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
REM Clean-up
REM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

mkdir "%~dp0.\out" 2> NUL
del /F "%OUTFILE%" 2> NUL

if exist "%OUTFILE%" (
	echo Failed to delete existing "%OUTFILE%" file!
	pause
	goto:eof
)

rmdir /Q /S "%~dp0.\out\~package" 2> NUL

if exist "%~dp0.\out\~package" (
	echo Failed to delete existing "%~dp0.\out\~package" directory!
	pause
	goto:eof
)

REM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
REM Build!
REM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

set MAKE_NONINTERACTIVE=1

call ".\make.cmd"

if not "%ERRORLEVEL%"=="0" (
	pause
	goto:eof
)

REM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
REM Copy binaries
REM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

mkdir "%~dp0.\out\~package"
mkdir "%~dp0.\out\~package\x86"
mkdir "%~dp0.\out\~package\x86+sse2"
mkdir "%~dp0.\out\~package\x64"

copy /Y "%~dp0.\*.txt"                        "%~dp0.\out\~package"
copy /Y "%~dp0.\bin\Win32\Release\*.exe"      "%~dp0.\out\~package\x86"
copy /Y "%~dp0.\bin\Win32\Release_SSE2\*.exe" "%~dp0.\out\~package\x86+sse2"
copy /Y "%~dp0.\bin\x64\Release\*.exe"        "%~dp0.\out\~package\x64"

attrib +R "%~dp0.\out\~package\*.*" /S

REM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
REM Create ZIP package
REM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

pushd "%~dp0.\out\~package"

if not "%ERRORLEVEL%"=="0" (
	pause
	goto:eof
)

"%~dp0.\etc\utils\win32\info-zip\zip.exe" -r -9 "%OUTFILE%" "*.*"

if not "%ERRORLEVEL%"=="0" (
	pause
	goto:eof
)

popd

rmdir /Q /S "%~dp0.\out\~package" 2> NUL

attrib +R "%OUTFILE%"

echo.
echo PACKAGE COMPLETED.
echo.

pause
