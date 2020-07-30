while true; do
	len_pattern=$((($RANDOM % 5) + 3))
	len_replace=$((($RANDOM % 5) + 3))
	len_srcdata=$((($RANDOM % 256) + 256))
	
	pattern="$(tr -dc 'abc' < /dev/urandom | head -c${len_pattern})"
	replace="$(tr -dc 'abc' < /dev/urandom | head -c${len_replace})"
	srcdata="$(tr -dc 'abc' < /dev/urandom | head -c${len_srcdata})"
	
	echo "\"${pattern}\""
	echo "\"${replace}\""
	echo "\"${srcdata}\""
	
	outdata1="$(sed "s|${pattern}|${replace}|g" <<< $srcdata)"
	outdata2="$(../../bin/Win32/Release/replace.exe "${pattern}" "${replace}" <<< $srcdata)"
	
	if [ "${outdata1}" == "${outdata2}" ]; then
		[ "${outdata1}" == "${srcdata}" ] && echo "SUCCESS" || echo "NOP"
	else
		echo "MISMATCH!"
		break;
	fi
	echo "---------------------"
done
