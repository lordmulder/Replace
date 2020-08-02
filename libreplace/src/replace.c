/******************************************************************************/
/* Replace, by LoRd_MuldeR <MuldeR2@GMX.de>                                   */
/* This work has been released under the CC0 1.0 Universal license!           */
/*                                                                            */
/* This program implements a variant of the "KMP" string-searching algorithm. */
/* See here for information:                                                  */
/* https://en.wikipedia.org/wiki/Knuth%E2%80%93Morris%E2%80%93Pratt_algorithm */
/******************************************************************************/

#include "libreplace/replace.h"
#include "libreplace/common.h"

#define CHECK_ABORT_REQUEST() do \
{ \
	if(*abort_fag) \
	{ \
		libreplace_print(logging, "Process cancelled by user --> aborting!\n"); \
		goto finished; \
	} \
} \
while(0)

static const char *const WR_ERROR_MESSAGE = "Write operation failed -> aborting!\n";
static const char *const RD_ERROR_MESSAGE = "Read operation failed -> aborting!\n";

/* ======================================================================= */
/* Helper Functions                                                        */
/* ======================================================================= */

static __inline ULARGE_INTEGER make_uint64(const ULONGLONG value)
{
	ULARGE_INTEGER result;
	result.QuadPart = value;
	return result;
}

#define TO_UPPER(C) ((((C) >= 0x61) && ((C) <= 0x7A)) ? ((C) - 0x20) : (C))

static __inline BOOL compare_char(const BYTE char_a, const BYTE char_b, const BOOL ignore_case)
{
	return ignore_case ? (TO_UPPER(char_a) == TO_UPPER(char_b)) : (char_a == char_b);
}

static __inline BOOL write_array(const BYTE *const data, const DWORD data_len, const libreplace_io_t *const io_functions, const BOOL sync)
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

static LONG *libreplace_compute_prefixes(const HANDLE logging, const BYTE *const needle, const LONG needle_len)
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
		libreplace_print(logging, "[DEBUG] prefix: ");
		for(needle_pos = 0U; needle_pos < needle_len; ++needle_pos)
		{
			libreplace_print_fmt(logging, (needle_pos > 0L) ? ", %ld" : "%ld", prefix[needle_pos]);
		}
		libreplace_print(logging, "\n");
	}

	return prefix;
}

BOOL libreplace_search_and_replace(const libreplace_io_t *const io_functions, const HANDLE logging, const BYTE *const needle, const LONG needle_len, const BYTE *const replacement, const LONG replacement_len, const libreplace_flags_t *const options, volatile BOOL *const abort_fag)
{
	BOOL success = FALSE, pending_input = FALSE, error_flag = FALSE;
	BYTE char_input;
	LONG *prefix = NULL, needle_pos = 0L;
	DWORD replacement_count = 0U;
	ULARGE_INTEGER position = { 0U, 0U };

	/* pre-compute prefixes */
	prefix = libreplace_compute_prefixes(logging, needle, needle_len);
	if(!prefix)
	{
		goto finished;
	}

	/* process all available input data */
	while(pending_input = io_functions->func_rd(&char_input, io_functions->context_rd, &error_flag))
	{
		/* dump the initial status */
		LIBREPLACE_TRACE(logging, "position: 0x%08lX%08lX\n", position.HighPart, position.LowPart);
		LIBREPLACE_TRACE(logging, "needle_pos[old]: %ld\n", needle_pos);

		/* update input byte counter */
		++position.QuadPart;

		/* if prefix cannot be extended, search for a shorter prefix */
		while ((needle_pos >= 0L) && (!compare_char(char_input, needle[needle_pos], options->case_insensitive)))
		{
			LIBREPLACE_TRACE(logging, "mismatch: %ld --> %ld\n", needle_pos, prefix[needle_pos]);
			/* write discarded part of old prefix */
			if((needle_pos > 0L) && (prefix[needle_pos] < needle_pos))
			{
				if(!write_array(needle, needle_pos - prefix[needle_pos], io_functions, options->force_sync))
				{
					libreplace_print(logging, WR_ERROR_MESSAGE);
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
				libreplace_print(logging, WR_ERROR_MESSAGE);
				goto finished;
			}
		}

		/* dump the updated status */
		LIBREPLACE_TRACE(logging, "needle_pos[new]: %ld\n", needle_pos);

		/* if a full match has been found, write the replacement instead */
		if (++needle_pos >= needle_len)
		{
			++replacement_count;
			needle_pos = 0U;
			if(options->verbose)
			{
				const ULARGE_INTEGER match_start = make_uint64(position.QuadPart - needle_len);
				libreplace_print_fmt(logging, "Replaced occurence at offset: 0x%08lX%08lX\n", match_start.HighPart, match_start.LowPart);
			}
			if(!write_array(replacement, replacement_len, io_functions, options->force_sync))
			{
				libreplace_print(logging, WR_ERROR_MESSAGE);
				goto finished;
			}
			if(options->replace_once)
			{
				if(options->verbose)
				{
					libreplace_print(logging, "Stopping search & replace after *first* match.\n");
				}
				break;
			}
		}

		/*check if abort was request*/
		CHECK_ABORT_REQUEST();
	}

	/* dump the final status */
	LIBREPLACE_TRACE(logging, "needle_pos[fin]: %ld\n", needle_pos);

	/* write any pending data */
	if(needle_pos > 0L)
	{	
		if(!write_array(needle, needle_pos, io_functions, options->force_sync))
		{
			libreplace_print(logging, WR_ERROR_MESSAGE);
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
				libreplace_print(logging, WR_ERROR_MESSAGE);
				goto finished;
			}
		}
	}

	/*check if abort was request*/
	CHECK_ABORT_REQUEST();

	/* check for any previous read errors */
	if(error_flag)
	{
		libreplace_print(logging, RD_ERROR_MESSAGE);
		goto finished;
	}

	/* flush output buffers*/
	success = io_functions->func_wr(LIBREPLACE_FLUSH, io_functions->context_wr, options->force_sync);

	if(options->verbose)
	{
		libreplace_print_fmt(logging, "Total occurences replaced: %lu\n", replacement_count);
	}

finished:

	if(prefix)
	{
		LocalFree(prefix);
	}

	return success;
}
