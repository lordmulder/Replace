@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"

set "EXE_PATH=..\..\bin\Win32\Release\replace.exe"
mkdir "out" 2> NUL

REM --------------------------------------------------------------------------
REM RUN TESTS
REM --------------------------------------------------------------------------

"%EXE_PATH%" "@@JAR_PATH@@" "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\output-1a.txt"
"%EXE_PATH%" "@JAR_PATH@"   "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\output-1b.txt"

"%EXE_PATH%" "@@OUT_PATH@@" "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\output-1c.txt"
"%EXE_PATH%" "@OUT_PATH@"   "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\output-1d.txt"

"%EXE_PATH%" "@@ICO_PATH@@" "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\output-1e.txt"
"%EXE_PATH%" "@ICO_PATH@"   "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\output-1f.txt"

"%EXE_PATH%"    "ababcabab" "c3p0" "src\input-1.txt" "out\output-1g.txt"
"%EXE_PATH%" -s "ababcabab" "c3p0" "src\input-1.txt" "out\output-1h.txt"

copy /Y "src\input-1.txt" "out\output-1i.txt"
"%EXE_PATH%" "@@OUT_PATH@@" "C:\Program Files (x86)\AVI-Mux_GUI\AVIMux_GUI.exe"            "out\output-1i.txt"
"%EXE_PATH%" "@@JAR_PATH@@" "C:\Java\zulu8.48.0.51-ca-fx-jdk8.0.262-win_x64\lib\tools.jar" "out\output-1i.txt"
"%EXE_PATH%" "@@ICO_PATH@@" "C:\texlive\2017\tlpkg\tlpsv\psv.ico"                          "out\output-1i.txt"
"%EXE_PATH%" "ababcabab"    "Lorem ipsum dolor sit amet, consectetur adipisici elit sed^!" "out\output-1i.txt"

"%EXE_PATH%"    "Cheshire Cat" "White Rabbit" "src\input-2.txt" "out\output-2a.txt"
"%EXE_PATH%" -s "Cheshire Cat" "White Rabbit" "src\input-2.txt" "out\output-2b.txt"

"%EXE_PATH%"    "White Rabbit" "Cheshire Cat" "src\input-2.txt" "out\output-2c.txt"
"%EXE_PATH%" -s "White Rabbit" "Cheshire Cat" "src\input-2.txt" "out\output-2d.txt"

"%EXE_PATH%" "ttttcattct" "quux" "src\input-3.txt" "out\output-3a.txt"
"%EXE_PATH%" "ggaattcagc" "quux" "src\input-3.txt" "out\output-3b.txt"

REM --------------------------------------------------------------------------
REM VERIFY
REM --------------------------------------------------------------------------

set VERIFY_STATUS=1

for /r %%i in ("out\*.txt") do (
	echo [%%~nxi]
	fc /B "out\%%~nxi" "ref\%%~nxi"
	if "!ERRORLEVEL!"=="0" (
		echo Passed
	) else (
		set VERIFY_STATUS=0
		echo Mismatch detected ^^!^^!^^!
	)
	echo.
)

if "%VERIFY_STATUS%"=="1" (
	echo All tests have passed :-^)
) else (
	echo At least one test has failed :-^(
)

echo.
pause
