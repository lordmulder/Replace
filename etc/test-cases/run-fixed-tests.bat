@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"

rmdir /Q /S "out" 2> NUL
mkdir "out" 2> NUL

REM --------------------------------------------------------------------------
REM RUN TESTS
REM --------------------------------------------------------------------------

for %%p in (Win32,x64) do (
	for %%c in (Release,Debug) do (
		echo %%p/%%c
		set "EXE_PATH=..\..\bin\%%~p\%%~c\replace.exe"
		mkdir "out\%%~p" 2> NUL
		mkdir "out\%%~p\%%~c" 2> NUL
		
		"!EXE_PATH!" "@@JAR_PATH@@" "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\%%~p\%%~c\output-1a.txt"
		"!EXE_PATH!" "@JAR_PATH@"   "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\%%~p\%%~c\output-1b.txt"

		"!EXE_PATH!" "@@OUT_PATH@@" "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\%%~p\%%~c\output-1c.txt"
		"!EXE_PATH!" "@OUT_PATH@"   "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\%%~p\%%~c\output-1d.txt"

		"!EXE_PATH!" "@@ICO_PATH@@" "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\%%~p\%%~c\output-1e.txt"
		"!EXE_PATH!" "@ICO_PATH@"   "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\%%~p\%%~c\output-1f.txt"

		"!EXE_PATH!"    "ababcabab" "c3p0" "src\input-1.txt" "out\%%~p\%%~c\output-1g.txt"
		"!EXE_PATH!" -s "ababcabab" "c3p0" "src\input-1.txt" "out\%%~p\%%~c\output-1h.txt"

		copy /Y "src\input-1.txt" "out\%%~p\%%~c\output-1i.txt"
		"!EXE_PATH!" "@@OUT_PATH@@" "C:\Program Files (x86)\AVI-Mux_GUI\AVIMux_GUI.exe"            "out\%%~p\%%~c\output-1i.txt"
		"!EXE_PATH!" "@@JAR_PATH@@" "C:\Java\zulu8.48.0.51-ca-fx-jdk8.0.262-win_x64\lib\tools.jar" "out\%%~p\%%~c\output-1i.txt"
		"!EXE_PATH!" "@@ICO_PATH@@" "C:\texlive\2017\tlpkg\tlpsv\psv.ico"                          "out\%%~p\%%~c\output-1i.txt"
		"!EXE_PATH!" "ababcabab"    "Lorem ipsum dolor sit amet, consectetur adipisici elit sed^!" "out\%%~p\%%~c\output-1i.txt"

		"..\utils\win32\pipe-utils\mkpipe.exe" "<" "src\input-1.txt" "!EXE_PATH!" "@@JAR_PATH@@" "C:\Windows\System32\drivers\tcpip.sys" ">" out\%%~p\%%~c\output-1j.txt"
		"..\utils\win32\pipe-utils\mkpipe.exe" "<" "src\input-1.txt" "!EXE_PATH!" "@JAR_PATH@"   "C:\Windows\System32\drivers\tcpip.sys" ">" out\%%~p\%%~c\output-1k.txt"

		"!EXE_PATH!"     "@jar_path@" "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\%%~p\%%~c\output-1l.txt"
		"!EXE_PATH!" -i  "@jar_path@" "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\%%~p\%%~c\output-1m.txt"
		"!EXE_PATH!" -id "@jar_path@" "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\%%~p\%%~c\output-1n.txt"

		"!EXE_PATH!"    "@???_PATH@" "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\%%~p\%%~c\output-1o.txt"
		"!EXE_PATH!" -g "@???_PATH@" "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\%%~p\%%~c\output-1p.txt"

		"!EXE_PATH!"    "Cheshire Cat" "White Rabbit" "src\input-2.txt" "out\%%~p\%%~c\output-2a.txt"
		"!EXE_PATH!" -s "Cheshire Cat" "White Rabbit" "src\input-2.txt" "out\%%~p\%%~c\output-2b.txt"

		"!EXE_PATH!"    "White Rabbit" "Cheshire Cat" "src\input-2.txt" "out\%%~p\%%~c\output-2c.txt"
		"!EXE_PATH!" -s "White Rabbit" "Cheshire Cat" "src\input-2.txt" "out\%%~p\%%~c\output-2d.txt"

		"!EXE_PATH!" "ttttcattct" "quux" "src\input-3.txt" "out\%%~p\%%~c\output-3a.txt"
		"!EXE_PATH!" "ggaattcagc" "quux" "src\input-3.txt" "out\%%~p\%%~c\output-3b.txt"
	)
)

REM --------------------------------------------------------------------------
REM VERIFY
REM --------------------------------------------------------------------------

set VERIFY_STATUS=1

for %%p in (Win32,x64) do (
	for %%c in (Release,Debug) do (
		for %%i in ("out\%%~p\%%~c\*.txt") do (
			echo [%%~nxi]
			fc /B "out\%%~p\%%~c\%%~nxi" "ref\%%~nxi"
			if "!ERRORLEVEL!"=="0" (
				echo Passed
			) else (
				set VERIFY_STATUS=0
				echo Mismatch detected ^^!^^!^^!
			)
			echo.
		)
	)
)

for %%i in ("ref\*.txt") do (
	for %%p in (Win32,x64) do (
		for %%c in (Release,Debug) do (
			echo [%%~nxi]
			fc /B "ref\%%~nxi" "out\%%~p\%%~c\%%~nxi"
			if "!ERRORLEVEL!"=="0" (
				echo Passed
			) else (
				set VERIFY_STATUS=0
				echo Mismatch detected ^^!^^!^^!
			)
			echo.
		)
	)
)

if "%VERIFY_STATUS%"=="1" (
	echo All tests have passed :-^)
) else (
	echo At least one test has failed :-^(
)

echo.
pause
