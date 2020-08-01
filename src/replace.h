/******************************************************************************/
/* Replace, by LoRd_MuldeR <MuldeR2@GMX.de>                                   */
/* This work has been released under the CC0 1.0 Universal license!           */
/*                                                                            */
/* This program implements a variant of the "KMP" string-searching algorithm. */
/* See here for information:                                                  */
/* https://en.wikipedia.org/wiki/Knuth%E2%80%93Morris%E2%80%93Pratt_algorithm */
/******************************************************************************/

#ifndef INC_REPLACE_H
#define INC_REPLACE_H

#include "utils.h"

typedef BOOL (*rd_func_t)(BYTE *const data, const DWORD_PTR, BOOL *const error_flag);
typedef BOOL (*wr_func_t)(const WORD data, const DWORD_PTR, const BOOL sync);

#define CHECK_ABORT_REQUEST() do \
{ \
	if(g_abort_requested) \
	{ \
		print_message(logging, "Process cancelled by user --> aborting!\n"); \
		g_process_aborted = TRUE; \
		goto finished; \
	} \
} \
while(0)

/* ======================================================================= */
/* Search & Replace                                                        */
/* ======================================================================= */

static const char *const WR_ERROR_MESSAGE = "Write operation failed -> aborting!\n";
static const char *const RD_ERROR_MESSAGE = "Read operation failed -> aborting!\n";

typedef struct io_functions_t
{
	rd_func_t rd_func;
	wr_func_t wr_func;
	DWORD_PTR context_rd;
	DWORD_PTR context_wr;
}
io_functions_t;

static __inline void init_io_functions(io_functions_t *const io_functions, const rd_func_t rd_func, const wr_func_t wr_func, const DWORD_PTR context_rd, const DWORD_PTR context_wr)
{
	SecureZeroMemory(io_functions, sizeof(io_functions_t));
	io_functions->rd_func = rd_func;
	io_functions->wr_func = wr_func;
	io_functions->context_rd = context_rd;
	io_functions->context_wr = context_wr;
}

static __inline BOOL write_array(const BYTE *const data, const DWORD data_len, const io_functions_t *const io_functions, const BOOL sync)
{
	DWORD pos;
	for(pos = 0U; pos < data_len; ++pos)
	{
		if(!io_functions->wr_func(data[pos], io_functions->context_wr, sync))
		{
			return FALSE;
		}
	}
	return TRUE;
}

static LONG *compute_prefixes(const HANDLE logging, const BYTE *const needle, const LONG needle_len)
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

	if(HAVE_TRACE)
	{
		print_message(logging, "[DEBUG] prefix: ");
		for(needle_pos = 0U; needle_pos < needle_len; ++needle_pos)
		{
			print_message_fmt(logging, (needle_pos > 0L) ? ", %ld" : "%ld", prefix[needle_pos]);
		}
		print_message(logging, "\n");
	}

	return prefix;
}

static BOOL search_and_replace(const io_functions_t *const io_functions, const HANDLE logging, const BYTE *const needle, const LONG needle_len, const BYTE *const replacement, const LONG replacement_len, const options_t *const options)
{
	BOOL success = FALSE, pending_input = FALSE, error_flag = FALSE;
	BYTE char_input;
	LONG *prefix = NULL, needle_pos = 0L;
	DWORD replacement_count = 0U;
	ULARGE_INTEGER position = { 0U, 0U };

	/* pre-compute prefixes */
	prefix = compute_prefixes(logging, needle, needle_len);
	if(!prefix)
	{
		goto finished;
	}

	/* process all available input data */
	while(pending_input = io_functions->rd_func(&char_input, io_functions->context_rd, &error_flag))
	{
		/* dump the initial status */
		TRACE(logging, "position: 0x%08lX%08lX\n", position.HighPart, position.LowPart);
		TRACE(logging, "needle_pos[old]: %ld\n", needle_pos);

		/* update input byte counter */
		++position.QuadPart;

		/* if prefix cannot be extended, search for a shorter prefix */
		while ((needle_pos >= 0L) && (!compare_char(char_input, needle[needle_pos], options->case_insensitive)))
		{
			TRACE(logging, "mismatch: %ld --> %ld\n", needle_pos, prefix[needle_pos]);
			/* write discarded part of old prefix */
			if((needle_pos > 0L) && (prefix[needle_pos] < needle_pos))
			{
				if(!write_array(needle, needle_pos - prefix[needle_pos], io_functions, options->force_sync))
				{
					print_message(logging, WR_ERROR_MESSAGE);
					goto finished;
				}
			}
			needle_pos = prefix[needle_pos];
		}

		/* write the input character, if it did *not* match */
		if(needle_pos < 0L)
		{
			if(!io_functions->wr_func(char_input, io_functions->context_wr, options->force_sync))
			{
				print_message(logging, WR_ERROR_MESSAGE);
				goto finished;
			}
		}

		/* dump the updated status */
		TRACE(logging, "needle_pos[new]: %ld\n", needle_pos);

		/* if a full match has been found, write the replacement instead */
		if (++needle_pos >= needle_len)
		{
			++replacement_count;
			needle_pos = 0U;
			if(options->verbose)
			{
				const ULARGE_INTEGER match_start = make_uint64(position.QuadPart - needle_len);
				print_message_fmt(logging, "Replaced occurence at offset: 0x%08lX%08lX\n", match_start.HighPart, match_start.LowPart);
			}
			if(!write_array(replacement, replacement_len, io_functions, options->force_sync))
			{
				print_message(logging, WR_ERROR_MESSAGE);
				goto finished;
			}
			if(options->replace_once)
			{
				if(options->verbose)
				{
					print_message(logging, "Stopping search & replace after *first* match.\n");
				}
				break;
			}
		}

		/*check if abort was request*/
		CHECK_ABORT_REQUEST();
	}

	/* dump the final status */
	TRACE(logging, "needle_pos[fin]: %ld\n", needle_pos);

	/* write any pending data */
	if(needle_pos > 0L)
	{	
		if(!write_array(needle, needle_pos, io_functions, options->force_sync))
		{
			print_message(logging, WR_ERROR_MESSAGE);
			goto finished;
		}
	}

	/*check if abort was request*/
	CHECK_ABORT_REQUEST();

	/* transfer any input data not processed yet */
	if(pending_input)
	{
		while(io_functions->rd_func(&char_input, io_functions->context_rd, &error_flag))
		{
			if(!io_functions->wr_func(char_input, io_functions->context_wr, options->force_sync))
			{
				print_message(logging, WR_ERROR_MESSAGE);
				goto finished;
			}
		}
	}

	/*check if abort was request*/
	CHECK_ABORT_REQUEST();

	/* check for any previous read errors */
	if(error_flag)
	{
		print_message(logging, RD_ERROR_MESSAGE);
		goto finished;
	}

	/* flush output buffers*/
	success = io_functions->wr_func(IO_FLUSH, io_functions->context_wr, options->force_sync);

	if(options->verbose)
	{
		print_message_fmt(logging, "Total occurences replaced: %lu\n", replacement_count);
	}

finished:

	if(prefix)
	{
		LocalFree(prefix);
	}

	return success;
}

#endif /*INC_REPLACE_H*/
