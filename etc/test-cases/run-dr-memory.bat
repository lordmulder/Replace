@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"

set "DR_MEMORY_PATH=C:\Program Files (x86)\Dr. Memory"

rmdir /Q /S "out" 2> NUL
mkdir "out" 2> NUL

REM --------------------------------------------------------------------------
REM RUN TESTS
REM --------------------------------------------------------------------------

for %%p in (Win32,x64) do (
	set "EXE_PATH=..\..\bin\%%~p\Debug\replace.exe"
	mkdir "out\%%~p" 2> NUL
	
	if "%%~p"=="x64" (
		set "DR_MEMORY_EXE=%DR_MEMORY_PATH%\bin64\drmemory.exe"
	) else (
		set "DR_MEMORY_EXE=%DR_MEMORY_PATH%\bin\drmemory.exe"
	)
	
	"!DR_MEMORY_EXE!" "!EXE_PATH!" "@@JAR_PATH@@" "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\%%~p\output-1a.txt"
	"!DR_MEMORY_EXE!" "!EXE_PATH!" "@JAR_PATH@"   "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\%%~p\output-1b.txt"
	
	"!DR_MEMORY_EXE!" "!EXE_PATH!" "@@OUT_PATH@@" "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\%%~p\output-1c.txt"
	"!DR_MEMORY_EXE!" "!EXE_PATH!" "@OUT_PATH@"   "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\%%~p\output-1d.txt"
	
	"!DR_MEMORY_EXE!" "!EXE_PATH!" "@@ICO_PATH@@" "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\%%~p\output-1e.txt"
	"!DR_MEMORY_EXE!" "!EXE_PATH!" "@ICO_PATH@"   "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\%%~p\output-1f.txt"
	
	"!DR_MEMORY_EXE!" "!EXE_PATH!"    "ababcabab" "c3p0" "src\input-1.txt" "out\%%~p\output-1g.txt"
	"!DR_MEMORY_EXE!" "!EXE_PATH!" -s "ababcabab" "c3p0" "src\input-1.txt" "out\%%~p\output-1h.txt"
	
	copy /Y "src\input-1.txt" "out\%%~p\output-1i.txt"
	"!DR_MEMORY_EXE!" "!EXE_PATH!" "@@OUT_PATH@@" "C:\Program Files (x86)\AVI-Mux_GUI\AVIMux_GUI.exe"            "out\%%~p\output-1i.txt"
	"!DR_MEMORY_EXE!" "!EXE_PATH!" "@@JAR_PATH@@" "C:\Java\zulu8.48.0.51-ca-fx-jdk8.0.262-win_x64\lib\tools.jar" "out\%%~p\output-1i.txt"
	"!DR_MEMORY_EXE!" "!EXE_PATH!" "@@ICO_PATH@@" "C:\texlive\2017\tlpkg\tlpsv\psv.ico"                          "out\%%~p\output-1i.txt"
	"!DR_MEMORY_EXE!" "!EXE_PATH!" "ababcabab"    "Lorem ipsum dolor sit amet, consectetur adipisici elit sed^!" "out\%%~p\output-1i.txt"
	
	type "src\input-1.txt" | "!DR_MEMORY_EXE!" "!EXE_PATH!" "@@JAR_PATH@@" "C:\Windows\System32\drivers\tcpip.sys" "-" "out\%%~p\output-1j.txt"
	type "src\input-1.txt" | "!DR_MEMORY_EXE!" "!EXE_PATH!" "@JAR_PATH@"   "C:\Windows\System32\drivers\tcpip.sys" "-" "out\%%~p\output-1k.txt"
	
	"!DR_MEMORY_EXE!" "!EXE_PATH!"     "@jar_path@" "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\%%~p\output-1l.txt"
	"!DR_MEMORY_EXE!" "!EXE_PATH!" -i  "@jar_path@" "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\%%~p\output-1m.txt"
	"!DR_MEMORY_EXE!" "!EXE_PATH!" -id "@jar_path@" "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\%%~p\output-1n.txt"
	
	"!DR_MEMORY_EXE!" "!EXE_PATH!"    "@???_PATH@" "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\%%~p\output-1o.txt"
	"!DR_MEMORY_EXE!" "!EXE_PATH!" -g "@???_PATH@" "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\%%~p\output-1p.txt"
	
	"!DR_MEMORY_EXE!" "!EXE_PATH!"    "Cheshire Cat" "White Rabbit" "src\input-2.txt" "out\%%~p\output-2a.txt"
	"!DR_MEMORY_EXE!" "!EXE_PATH!" -s "Cheshire Cat" "White Rabbit" "src\input-2.txt" "out\%%~p\output-2b.txt"
	
	"!DR_MEMORY_EXE!" "!EXE_PATH!"    "White Rabbit" "Cheshire Cat" "src\input-2.txt" "out\%%~p\output-2c.txt"
	"!DR_MEMORY_EXE!" "!EXE_PATH!" -s "White Rabbit" "Cheshire Cat" "src\input-2.txt" "out\%%~p\output-2d.txt"
	
	if not "%%~p"=="x64" (
		"!DR_MEMORY_EXE!" "!EXE_PATH!" "ttttcattct" "quux" "src\input-3.txt" "out\%%~p\output-3a.txt"
		"!DR_MEMORY_EXE!" "!EXE_PATH!" "ggaattcagc" "quux" "src\input-3.txt" "out\%%~p\output-3b.txt"
	)
)

REM --------------------------------------------------------------------------
REM VERIFY
REM --------------------------------------------------------------------------

set VERIFY_STATUS=1

for %%p in (Win32,x64) do (
	for %%i in ("out\%%~p\*.txt") do (
		echo [%%~nxi]
		fc /B "out\%%~p\%%~nxi" "ref\%%~nxi"
		if "!ERRORLEVEL!"=="0" (
			echo Passed
		) else (
			set VERIFY_STATUS=0
			echo Mismatch detected ^^!^^!^^!
		)
		echo.
	)
)

if "%VERIFY_STATUS%"=="1" (
	echo All tests have passed :-^)
) else (
	echo At least one test has failed :-^(
)

echo.
pause
