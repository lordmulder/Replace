/******************************************************************************/
/* Replace, by LoRd_MuldeR <MuldeR2@GMX.de>                                   */
/* This work has been released under the CC0 1.0 Universal license!           */
/*                                                                            */
/* This program implements a variant of the "KMP" string-searching algorithm. */
/* See here for information:                                                  */
/* https://en.wikipedia.org/wiki/Knuth%E2%80%93Morris%E2%80%93Pratt_algorithm */
/******************************************************************************/

#ifndef INC_LIBREPLACE_H
#define INC_LIBREPLACE_H

#ifndef _WINDOWS_
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#endif

#define LIBREPLACE_VERSION_MAJOR 1
#define LIBREPLACE_VERSION_MINOR 4
#define LIBREPLACE_VERSION_PATCH 0

#define LIBREPLACE_FLUSH ((WORD)-1)

typedef BOOL (*libreplace_rd_func_t)(BYTE *const data, const DWORD_PTR context, BOOL *const error_flag);
typedef BOOL (*libreplace_wr_func_t)(const WORD data, const DWORD_PTR context, const BOOL sync);

typedef struct libreplace_io_t
{
	libreplace_rd_func_t func_rd;
	libreplace_wr_func_t func_wr;
	DWORD_PTR context_rd;
	DWORD_PTR context_wr;
}
libreplace_io_t;

typedef BOOL (*libreplace_logging_func_t)(const DWORD_PTR context, const CHAR *const text);

typedef struct libreplace_logger_t
{
	libreplace_logging_func_t logging_func;
	DWORD_PTR context;
}
libreplace_logger_t;

typedef struct libreplace_flags_t
{ 
	BOOL case_insensitive;
	BOOL replace_once;
	BOOL verbose;
	BOOL force_sync;
}
libreplace_flags_t;

BOOL libreplace_search_and_replace(const libreplace_io_t *const io_functions, const libreplace_logger_t *const logger, const BYTE *const needle, const LONG needle_len, const BYTE *const replacement, const LONG replacement_len, const libreplace_flags_t *const options, volatile BOOL *const abort_fag);

#endif /*INC_LIBREPLACE_H*/
