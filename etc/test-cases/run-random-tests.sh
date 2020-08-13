#!/bin/bash
cd "$(dirname "${BASH_SOURCE[0]}")"

while true; do
	len_pattern=$((($RANDOM % 12) + 4))
	len_replace=$((($RANDOM % 12) + 4))
	len_indata1=$((($RANDOM % 127) + 251))
	len_indata2=$((($RANDOM % 127) + 251))
	
	pattern="$(tr -dc 'abc' < /dev/urandom | head -c${len_pattern})"
	replace="$(tr -dc 'abc' < /dev/urandom | head -c${len_replace})"
	srcdata="$(tr -dc 'abcABC' < /dev/urandom | head -c${len_indata1})${pattern}$(tr -dc 'abcABC' < /dev/urandom | head -c${len_indata2})"
	
	echo "\"${pattern}\" --> \"${replace}\""
	echo "\"${srcdata}\""
	
	refdata1="$(sed "s|${pattern}|${replace}|g"  <<< $srcdata)"
	refdata2="$(sed "s|${pattern}|${replace}|gi" <<< $srcdata)"
	
	outdata1="$(../../bin/Win32/Release/replace.exe "${pattern}" "${replace}" <<< $srcdata)"
	if [ "${outdata1}" != "${refdata1}" ]; then
		echo -e "\033[1;31mMISMATCH!\033[0m\n#1: \"${outdata1}\"\n#2: \"${refdata1}\""
		break;
	fi
	
	outdata2="$(../../bin/Win32/Release/replace.exe -i "${pattern}" "${replace}" <<< $srcdata)"
	if [ "${outdata2}" != "${refdata2}" ]; then
		echo -e "\033[1;31mMISMATCH!\033[0m\n#1: \"${outdata2}\"\n#2: \"${refdata2}\""
		break;
	fi
	
	outdata3="$(../../bin/Win32/Release/replace.exe -id "${pattern}" "${replace}" <<< $srcdata)"
	if [ "${outdata3}" != "${srcdata}" ]; then
		echo -e "\033[1;31mMISMATCH!\033[0m\n#1: \"${outdata3}\"\n#2: \"${srcdata}\""
		break;
	fi
	
	echo -e "\033[1;32mSUCCESS\033[0m";
	echo "------------------------------------------------------------------------------"
done
