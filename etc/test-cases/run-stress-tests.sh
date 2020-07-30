#!/bin/bash

dd if=/dev/urandom | ../../bin/Win32/Release/replace.exe -bv 0xDEADBEEF 0xCAFEBABE | pv -rb > /dev/null
echo "Whoops!"
