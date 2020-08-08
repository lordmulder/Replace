@echo off
cd /d "%~dp0"

rmdir /Q /S "%~dp0.\out\~package" 2> NUL

if exist "%~dp0.\out\~package" (
	echo Failed to delete existing "%~dp0.\out\~package" directory!
	pause
	goto:eof
)

set MAKE_NONINTERACTIVE=1
call ".\make.cmd"

if not "%ERRORLEVEL%"=="0" (
	pause
	goto:eof
)

mkdir "%~dp0.\out\~package"
mkdir "%~dp0.\out\~package\sse2"
mkdir "%~dp0.\out\~package\x64"

copy /Y "%~dp0.\*.txt"                        "%~dp0.\out\~package"
copy /Y "%~dp0.\bin\Win32\Release\*.exe"      "%~dp0.\out\~package"
copy /Y "%~dp0.\bin\Win32\Release_SSE2\*.exe" "%~dp0.\out\~package\sse2"
copy /Y "%~dp0.\bin\x64\Release\*.exe"        "%~dp0.\out\~package\x64"

attrib +R "%~dp0.\out\~package\*.*" /S

echo.
echo PACKAGE COMPLETED.
echo.

pause
