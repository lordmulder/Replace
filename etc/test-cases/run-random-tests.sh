#!/bin/bash
cd "$(dirname "${BASH_SOURCE[0]}")"

while true; do
	pattern="$(tr -dc 'abc' < /dev/urandom | head -c$((($RANDOM % 12) + 4)))"
	replace="$(tr -dc 'abc' < /dev/urandom | head -c$((($RANDOM % 12) + 4)))"
	srcdata="$(tr -dc 'abcABC' < /dev/urandom | head -c$(($RANDOM % 293)))${pattern}$(tr -dc 'abcABC' < /dev/urandom | head -c$(($RANDOM % 293)))"
	globber="?"
	while [ "${globber}" == "?" ]; do
		globber="$(tr -dc 'abc' < /dev/urandom | head -c$(($RANDOM % 3)))?$(tr -dc 'abc' < /dev/urandom | head -c$(($RANDOM % 3)))"
	done;
	
	echo "\"${pattern}\", \"${globber}\" --> \"${replace}\""
	echo "\"${srcdata}\""
	
	refdata1="$(printf '%s' "${srcdata}" | sed "s|${pattern}|${replace}|g")"
	refdata2="$(printf '%s' "${srcdata}" | sed "s|${pattern}|${replace}|gi")"
	refdata3="$(printf '%s' "${srcdata}" | sed "s|$(printf '%s' "${globber}" | tr "?" ".")|${replace}|g")"
	
	outdata1="$(printf '%s' "${srcdata}" | ../../bin/Win32/Release/replace.exe "${pattern}" "${replace}")"
	if [ "${outdata1}" != "${refdata1}" ]; then
		echo -e "\033[1;31mMISMATCH! (#1)\033[0m\nout: \"${outdata1}\"\nref: \"${refdata1}\""
		break;
	fi
	
	outdata2="$(printf '%s' "${srcdata}" | ../../bin/Win32/Release/replace.exe -i "${pattern}" "${replace}")"
	if [ "${outdata2}" != "${refdata2}" ]; then
		echo -e "\033[1;31mMISMATCH! (#2)\033[0m\nout: \"${outdata2}\"\nref: \"${refdata2}\""
		break;
	fi
	
	outdata3="$(printf '%s' "${srcdata}" | ../../bin/Win32/Release/replace.exe -g "${globber}" "${replace}")"
	if [ "${outdata3}" != "${refdata3}" ]; then
		echo -e "\033[1;31mMISMATCH! (#3)\033[0m\nout: \"${outdata3}\"\nref: \"${refdata3}\""
		break;
	fi
	
	outdata4="$(printf '%s' "${srcdata}" | ../../bin/Win32/Release/replace.exe -id "${pattern}" "${replace}")"
	if [ "${outdata4}" != "${srcdata}" ]; then
		echo -e "\033[1;31mMISMATCH! (#4)\033[0m\nout: \"${outdata4}\"\nref: \"${srcdata}\""
		break;
	fi
	
	echo -e "\033[1;32mSUCCESS\033[0m";
	echo "------------------------------------------------------------------------------"
done
