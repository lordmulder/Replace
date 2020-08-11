@echo off
cd /d "%~dp0"

"..\utils\win32\pipe-utils\mkpipe.exe" ^
	"..\utils\win32\pipe-utils\rand.exe" "|" ^
	"..\..\bin\Win32\Release\replace.exe" -bv 0xDEADBEEF 0xCAFEBABE "|" ^
	"..\utils\win32\pipe-utils\pv.exe" -rb ">" NUL

echo "Whoops!"
pause
