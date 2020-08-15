/******************************************************************************/
/* Replace, by LoRd_MuldeR <MuldeR2@GMX.de>                                   */
/* This work has been released under the CC0 1.0 Universal license!           */
/******************************************************************************/

#ifndef INC_LIBREPLACE_H
#define INC_LIBREPLACE_H

#ifndef _WINDOWS_
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#endif

#define LIBREPLACE_VERSION_MAJOR 1
#define LIBREPLACE_VERSION_MINOR 7
#define LIBREPLACE_VERSION_PATCH 0

#define LIBREPLACE_FLUSH ((WORD)-1)
#define LIBREPLACE_MAXLEN ((DWORD)(MAXDWORD >> 1))

typedef BOOL (*libreplace_rd_func_t)(BYTE *const data, const DWORD_PTR context, BOOL *const error_flag);
typedef BOOL (*libreplace_wr_func_t)(const WORD data, const DWORD_PTR context);

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
	BOOL normalize;
	BOOL replace_once;
	BOOL dry_run;
	BOOL match_crlf;
	BOOL verbose;
}
libreplace_flags_t;

BOOL libreplace_search_and_replace(const libreplace_io_t *const io_functions, const libreplace_logger_t *const logger, const BYTE *const needle, const BOOL *const wildcard_map, const DWORD needle_len, const BYTE *const replacement, const DWORD replacement_len, const libreplace_flags_t *const options, volatile BOOL *const abort_flag);

#endif /*INC_LIBREPLACE_H*/
