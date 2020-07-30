#!/bin/bash

while true; do
	len_pattern=$((($RANDOM % 12) + 4))
	len_replace=$((($RANDOM % 12) + 4))
	len_indata1=$((($RANDOM % 128) + 128))
	len_indata2=$((($RANDOM % 128) + 128))
	
	pattern="$(tr -dc 'abc' < /dev/urandom | head -c${len_pattern})"
	replace="$(tr -dc 'abc' < /dev/urandom | head -c${len_replace})"
	
	indata1="$(tr -dc 'abc' < /dev/urandom | head -c${len_indata1})"
	indata2="$(tr -dc 'abc' < /dev/urandom | head -c${len_indata2})"
	srcdata="${indata1}${pattern}${indata2}"
	
	echo "\"${pattern}\""
	echo "\"${replace}\""
	echo "\"${srcdata}\""
	
	outdata1="$(sed "s|${pattern}|${replace}|g" <<< $srcdata)"
	outdata2="$(../../bin/Win32/Release/replace.exe "${pattern}" "${replace}" <<< $srcdata)"
	
	if [ "${outdata1}" != "${outdata2}" ]; then
		echo -e "\033[1;31mMISMATCH!\033[0m\n#1: \"${outdata1}\n#2: \"${outdata2}\""
		break;
	fi
	echo -e "\033[1;32mSUCCESS\033[0m";
	echo "------------------------------------------------------------------------------"
done
