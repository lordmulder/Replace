/******************************************************************************/
/* Replace, by LoRd_MuldeR <MuldeR2@GMX.de>                                   */
/* This work has been released under the CC0 1.0 Universal license!           */
/*                                                                            */
/* This program implements a variant of the "KMP" string-searching algorithm. */
/* See here for information:                                                  */
/* https://en.wikipedia.org/wiki/Knuth%E2%80%93Morris%E2%80%93Pratt_algorithm */
/******************************************************************************/

#include "libreplace/replace.h"

#ifdef _DEBUG
#define LIBREPLACE_HAVE_TRACE 1
#define LIBREPLACE_TRACE(OUT, FMT, ...) libreplace_print_fmt((OUT), "[DEBUG] " FMT, __VA_ARGS__)
#else
#define LIBREPLACE_HAVE_TRACE 0
#define LIBREPLACE_TRACE(OUT, FMT, ...) __noop()
#endif

#define CHECK_ABORT_REQUEST() do \
{ \
	if(*abort_flag) \
	{ \
		libreplace_print(logger, "Process cancelled by user --> aborting!\n"); \
		goto finished; \
	} \
} \
while(0)

static const char *const WR_ERROR_MESSAGE = "Write operation failed -> aborting!\n";
static const char *const RD_ERROR_MESSAGE = "Read operation failed -> aborting!\n";

/* ======================================================================= */
/* Helper Functions                                                        */
/* ======================================================================= */

static __inline BOOL libreplace_print(const libreplace_logger_t *const logger, const CHAR *const text)
{
	return (logger) ? logger->logging_func(logger->context, text) : TRUE;
}

static __inline BOOL libreplace_print_fmt(const libreplace_logger_t *const logger, const CHAR *const format, ...)
{
	if(logger)
	{
		CHAR temp[256U];
		BOOL result = FALSE;
		va_list ap;
		va_start(ap, format);
		if(wvsprintfA(temp, format, ap))
		{
			result = logger->logging_func(logger->context, temp);
		}
		va_end(ap);
		return result;
	}
	return TRUE;
}

static __inline ULARGE_INTEGER libreplace_uint64(const ULONGLONG value)
{
	ULARGE_INTEGER result;
	result.QuadPart = value;
	return result;
}

#define TO_UPPER(C) ((((C) >= 0x61) && ((C) <= 0x7A)) ? ((C) - 0x20) : (C))

static __inline BOOL libreplace_compare(const BYTE char_a, const BYTE char_b, const BOOL ignore_case)
{
	return ignore_case ? (TO_UPPER(char_a) == TO_UPPER(char_b)) : (char_a == char_b);
}

static __inline BOOL libreplace_write(const BYTE *const data, const DWORD data_len, const libreplace_io_t *const io_functions, const BOOL sync)
{
	DWORD pos;
	for(pos = 0U; pos < data_len; ++pos)
	{
		if(!io_functions->func_wr(data[pos], io_functions->context_wr, sync))
		{
			return FALSE;
		}
	}
	return TRUE;
}

/* ======================================================================= */
/* Search & Replace                                                        */
/* ======================================================================= */

static LONG *libreplace_compute_prefixes(const libreplace_logger_t *const logger, const BYTE *const needle, const LONG needle_len)
{
	LONG *prefix, needle_pos = 0L, prefix_len = -1L;
	if(!(prefix = (LONG*) LocalAlloc(LPTR, sizeof(LONG) * (needle_len + 1L))))
	{
		return NULL;
	}

	prefix[0U] = -1L;
	while (needle_pos < needle_len)
	{
		while ((prefix_len >= 0) && (needle[needle_pos] != needle[prefix_len]))
		{
			prefix_len = prefix[prefix_len];
		}
		prefix[++needle_pos] = ++prefix_len;
	}

	if(LIBREPLACE_HAVE_TRACE)
	{
		libreplace_print(logger, "[DEBUG] prefix: ");
		for(needle_pos = 0U; needle_pos < needle_len; ++needle_pos)
		{
			libreplace_print_fmt(logger, (needle_pos > 0L) ? ", %ld" : "%ld", prefix[needle_pos]);
		}
		libreplace_print(logger, "\n");
	}

	return prefix;
}

BOOL libreplace_search_and_replace(const libreplace_io_t *const io_functions, const libreplace_logger_t *const logger, const BYTE *const needle, const LONG needle_len, const BYTE *const replacement, const LONG replacement_len, const libreplace_flags_t *const options, volatile BOOL *const abort_flag)
{
	BOOL success = FALSE, pending_input = FALSE, error_flag = FALSE;
	BYTE char_input;
	LONG *prefix = NULL, needle_pos = 0L;
	DWORD replacement_count = 0U;
	ULARGE_INTEGER position = { 0U, 0U };

	/* pre-compute prefixes */
	prefix = libreplace_compute_prefixes(logger, needle, needle_len);
	if(!prefix)
	{
		goto finished;
	}

	/* process all available input data */
	while(pending_input = io_functions->func_rd(&char_input, io_functions->context_rd, &error_flag))
	{
		/* dump the initial status */
		LIBREPLACE_TRACE(logger, "position: 0x%08lX%08lX\n", position.HighPart, position.LowPart);
		LIBREPLACE_TRACE(logger, "needle_pos[old]: %ld\n", needle_pos);

		/* update input byte counter */
		++position.QuadPart;

		/* if prefix cannot be extended, search for a shorter prefix */
		while ((needle_pos >= 0L) && (!libreplace_compare(char_input, needle[needle_pos], options->case_insensitive)))
		{
			LIBREPLACE_TRACE(logger, "mismatch: %ld --> %ld\n", needle_pos, prefix[needle_pos]);
			/* write discarded part of old prefix */
			if((needle_pos > 0L) && (prefix[needle_pos] < needle_pos))
			{
				if(!libreplace_write(needle, needle_pos - prefix[needle_pos], io_functions, options->force_sync))
				{
					libreplace_print(logger, WR_ERROR_MESSAGE);
					goto finished;
				}
			}
			needle_pos = prefix[needle_pos];
		}

		/* write the input character, if it did *not* match */
		if(needle_pos < 0L)
		{
			if(!io_functions->func_wr(char_input, io_functions->context_wr, options->force_sync))
			{
				libreplace_print(logger, WR_ERROR_MESSAGE);
				goto finished;
			}
		}

		/* dump the updated status */
		LIBREPLACE_TRACE(logger, "needle_pos[new]: %ld\n", needle_pos);

		/* if a full match has been found, write the replacement instead */
		if (++needle_pos >= needle_len)
		{
			++replacement_count;
			needle_pos = 0U;
			if(options->verbose)
			{
				const ULARGE_INTEGER match_start = libreplace_uint64(position.QuadPart - needle_len);
				libreplace_print_fmt(logger, "Replaced occurence at offset: 0x%08lX%08lX\n", match_start.HighPart, match_start.LowPart);
			}
			if(!libreplace_write(replacement, replacement_len, io_functions, options->force_sync))
			{
				libreplace_print(logger, WR_ERROR_MESSAGE);
				goto finished;
			}
			if(options->replace_once)
			{
				if(options->verbose)
				{
					libreplace_print(logger, "Stopping search & replace after *first* match.\n");
				}
				break;
			}
		}

		/*check if abort was request*/
		CHECK_ABORT_REQUEST();
	}

	/* dump the final status */
	LIBREPLACE_TRACE(logger, "needle_pos[fin]: %ld\n", needle_pos);

	/* write any pending data */
	if(needle_pos > 0L)
	{	
		if(!libreplace_write(needle, needle_pos, io_functions, options->force_sync))
		{
			libreplace_print(logger, WR_ERROR_MESSAGE);
			goto finished;
		}
	}

	/*check if abort was request*/
	CHECK_ABORT_REQUEST();

	/* transfer any input data not processed yet */
	if(pending_input)
	{
		while(io_functions->func_rd(&char_input, io_functions->context_rd, &error_flag))
		{
			if(!io_functions->func_wr(char_input, io_functions->context_wr, options->force_sync))
			{
				libreplace_print(logger, WR_ERROR_MESSAGE);
				goto finished;
			}
		}
	}

	/*check if abort was request*/
	CHECK_ABORT_REQUEST();

	/* check for any previous read errors */
	if(error_flag)
	{
		libreplace_print(logger, RD_ERROR_MESSAGE);
		goto finished;
	}

	/* flush output buffers*/
	success = io_functions->func_wr(LIBREPLACE_FLUSH, io_functions->context_wr, options->force_sync);

	if(options->verbose)
	{
		libreplace_print_fmt(logger, "Total occurences replaced: %lu\n", replacement_count);
	}

finished:

	if(prefix)
	{
		LocalFree(prefix);
	}

	return success;
}
