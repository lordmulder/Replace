/******************************************************************************/
/* Replace, by LoRd_MuldeR <MuldeR2@GMX.de>                                   */
/* This work has been released under the CC0 1.0 Universal license!           */
/*                                                                            */
/* This program implements a variant of the "KMP" string-searching algorithm. */
/* See here for information:                                                  */
/* https://en.wikipedia.org/wiki/Knuth%E2%80%93Morris%E2%80%93Pratt_algorithm */
/******************************************************************************/

#ifndef INC_LIBREPLACE_COMMON_H
#define INC_LIBREPLACE_COMMON_H

#ifndef _WINDOWS_
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#endif

#ifdef _DEBUG
#define LIBREPLACE_HAVE_TRACE 1
#define LIBREPLACE_TRACE(OUT, FMT, ...) libreplace_print_fmt((OUT), "[DEBUG] " FMT, __VA_ARGS__)
#else
#define LIBREPLACE_HAVE_TRACE 0
#define LIBREPLACE_TRACE(OUT, FMT, ...) __noop()
#endif

static __inline BOOL libreplace_print(const HANDLE output, const CHAR *const text)
{
	DWORD bytes_written;
	if(output != INVALID_HANDLE_VALUE)
	{
		return WriteFile(output, text, lstrlenA(text), &bytes_written, NULL);
	}
	return TRUE;
}

static __inline BOOL libreplace_print_fmt(const HANDLE output, const CHAR *const format, ...)
{
	CHAR temp[256U];
	BOOL result = FALSE;
	va_list ap;
	if(output != INVALID_HANDLE_VALUE)
	{
		va_start(ap, format);
		if(wvsprintfA(temp, format, ap))
		{
			result = libreplace_print(output, temp);
		}
		va_end(ap);
		return result;
	}
	return TRUE;
}

#endif /*INC_LIBREPLACE_COMMON_H*/
