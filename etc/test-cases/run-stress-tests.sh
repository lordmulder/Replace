#!/bin/bash

../fastrand/bin/Win32/Release/fastrand.exe | ../../bin/Win32/Release/replace.exe -bv 0xDEADBEEF 0xCAFEBABE | pv -rb > /dev/null

echo "Whoops!"
