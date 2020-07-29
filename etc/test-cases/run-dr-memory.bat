@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"

set "EXE_PATH=..\..\bin\Win32\Debug\replace.exe"
set "DR_MEMORY_PATH=C:\Program Files (x86)\Dr. Memory\bin\drmemory.exe"
mkdir "out" 2> NUL

REM --------------------------------------------------------------------------
REM RUN TESTS
REM --------------------------------------------------------------------------

"%DR_MEMORY_PATH%" "%EXE_PATH%" "@@JAR_PATH@@" "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\output-1a.txt"
"%DR_MEMORY_PATH%" "%EXE_PATH%" "@JAR_PATH@"   "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\output-1b.txt"

"%DR_MEMORY_PATH%" "%EXE_PATH%" "@@OUT_PATH@@" "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\output-1c.txt"
"%DR_MEMORY_PATH%" "%EXE_PATH%" "@OUT_PATH@"   "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\output-1d.txt"

"%DR_MEMORY_PATH%" "%EXE_PATH%" "@@ICO_PATH@@" "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\output-1e.txt"
"%DR_MEMORY_PATH%" "%EXE_PATH%" "@ICO_PATH@"   "C:\Windows\System32\drivers\tcpip.sys" "src\input-1.txt" "out\output-1f.txt"

"%DR_MEMORY_PATH%" "%EXE_PATH%"    "ababcabab" "c3p0" "src\input-1.txt" "out\output-1g.txt"
"%DR_MEMORY_PATH%" "%EXE_PATH%" -s "ababcabab" "c3p0" "src\input-1.txt" "out\output-1h.txt"

copy /Y "src\input-1.txt" "out\output-1i.txt"
"%DR_MEMORY_PATH%" "%EXE_PATH%" "@@OUT_PATH@@" "C:\Program Files (x86)\AVI-Mux_GUI\AVIMux_GUI.exe"            "out\output-1i.txt"
"%DR_MEMORY_PATH%" "%EXE_PATH%" "@@JAR_PATH@@" "C:\Java\zulu8.48.0.51-ca-fx-jdk8.0.262-win_x64\lib\tools.jar" "out\output-1i.txt"
"%DR_MEMORY_PATH%" "%EXE_PATH%" "@@ICO_PATH@@" "C:\texlive\2017\tlpkg\tlpsv\psv.ico"                          "out\output-1i.txt"
"%DR_MEMORY_PATH%" "%EXE_PATH%" "ababcabab"    "Lorem ipsum dolor sit amet, consectetur adipisici elit sed^!" "out\output-1i.txt"

"%DR_MEMORY_PATH%" "%EXE_PATH%"    "Cheshire Cat" "White Rabbit" "src\input-2.txt" "out\output-2a.txt"
"%DR_MEMORY_PATH%" "%EXE_PATH%" -s "Cheshire Cat" "White Rabbit" "src\input-2.txt" "out\output-2b.txt"

"%DR_MEMORY_PATH%" "%EXE_PATH%"    "White Rabbit" "Cheshire Cat" "src\input-2.txt" "out\output-2c.txt"
"%DR_MEMORY_PATH%" "%EXE_PATH%" -s "White Rabbit" "Cheshire Cat" "src\input-2.txt" "out\output-2d.txt"

echo.
pause
