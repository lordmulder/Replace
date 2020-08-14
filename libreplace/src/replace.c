/******************************************************************************/
/* Replace, by LoRd_MuldeR <MuldeR2@GMX.de>                                   */
/* This work has been released under the CC0 1.0 Universal license!           */
/*                                                                            */
/* This program implements a variant of the "KMP" string-searching algorithm. */
/* See here for information:                                                  */
/* https://en.wikipedia.org/wiki/Knuth%E2%80%93Morris%E2%80%93Pratt_algorithm */
/******************************************************************************/

#include "libreplace/replace.h"

#define TO_UPPER(C) ((((C) >= 0x61) && ((C) <= 0x7A)) ? ((C) - 0x20) : (C))
#define COMPARE_CHAR(A, B) (ignore_case ? (TO_UPPER(A) == TO_UPPER(B)) : ((A) == (B)))

#define INCREMENT(VALUE, LIMIT) do \
{ \
	if(++(VALUE) >= (LIMIT)) \
	{ \
		(VALUE) = 0L; \
	} \
} \
while (0)

#define CHECK_ABORT_REQUEST() do \
{ \
	if(*abort_flag) \
	{ \
		libreplace_print(logger, ABORTING_MESSAGE); \
		goto finished; \
	} \
} \
while(0)

static const CHAR *const WR_ERROR_MESSAGE = "Write operation failed -> aborting!\n";
static const CHAR *const RD_ERROR_MESSAGE = "Read operation failed -> aborting!\n";
static const CHAR *const ABORTING_MESSAGE = "Process cancelled by user --> aborting!\n";

/* ======================================================================= */
/* Ring buffer                                                             */
/* ======================================================================= */

typedef struct ringbuffer_t
{ 
	LONG capacity;
	LONG valid;
	LONG index_write;
	LONG index_flush;
	BYTE buffer[];
}
ringbuffer_t;

static ringbuffer_t *ringbuffer_alloc(const LONG size)
{
	if(size > 0L)
	{
		ringbuffer_t *const ringbuffer = (ringbuffer_t*) LocalAlloc(LPTR, sizeof(ringbuffer_t) + (sizeof(BYTE) * size));
		if(ringbuffer)
		{
			ringbuffer->capacity = size;
			ringbuffer->index_write = ringbuffer->valid = ringbuffer->index_flush = 0L;
			return ringbuffer;
		}
	}
	return NULL;
}

static __inline BOOL ringbuffer_append(const BYTE in, BYTE *const out, ringbuffer_t *const ringbuffer)
{
	*out = ringbuffer->buffer[ringbuffer->index_write];
	ringbuffer->buffer[ringbuffer->index_write] = in;
	INCREMENT(ringbuffer->index_write, ringbuffer->capacity);
	if(ringbuffer->valid < ringbuffer->capacity)
	{
		++ringbuffer->valid;
		return FALSE;
	}
	return TRUE;
}

static __inline BOOL ringbuffer_compare(ringbuffer_t *const ringbuffer, const BYTE *const needle, const LONG needle_len, const BOOL ignore_case, const BYTE *const wildcard_char)
{
	LONG needle_pos, buffer_pos;
	if(ringbuffer->valid == needle_len)
	{
		for(needle_pos = 0L, buffer_pos = ringbuffer->index_write; needle_pos < needle_len; ++needle_pos)
		{
			if(((!wildcard_char) || (needle[needle_pos] != *wildcard_char)) && (!COMPARE_CHAR(ringbuffer->buffer[buffer_pos], needle[needle_pos])))
			{
				return FALSE;
			}
			INCREMENT(buffer_pos, ringbuffer->capacity);
		}
		return TRUE;
	}
	return FALSE;
}

static __inline BOOL ringbuffer_flush(BYTE *const out, ringbuffer_t *const ringbuffer)
{
	if(ringbuffer->index_flush < ringbuffer->valid)
	{
		*out = ringbuffer->buffer[ringbuffer->valid >= ringbuffer->capacity ? (ringbuffer->index_write + ringbuffer->index_flush++) % ringbuffer->capacity : ringbuffer->index_flush++];
		return TRUE;
	}
	return FALSE;
}

static __inline void ringbuffer_reset(ringbuffer_t *const ringbuffer)
{
	ringbuffer->index_write = ringbuffer->index_flush = ringbuffer->valid = 0L;
}

/* ======================================================================= */
/* Utility Functions                                                       */
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
		if(wvsprintfA(temp, format, ap) > 0L)
		{
			result = logger->logging_func(logger->context, temp);
		}
		va_end(ap);
		return result;
	}
	return TRUE;
}

static __inline BOOL libreplace_write(const BYTE *const data, const LONG data_len, const libreplace_io_t *const io_functions)
{
	LONG pos;
	for(pos = 0L; pos < data_len; ++pos)
	{
		if(!io_functions->func_wr(data[pos], io_functions->context_wr))
		{
			return FALSE;
		}
	}
	return TRUE;
}

static __inline BOOL libreplace_flush_pending(ringbuffer_t *const ringbuffer, const libreplace_io_t *const io_functions)
{
	BYTE char_out;
	while(ringbuffer_flush(&char_out, ringbuffer))
	{
		if(!io_functions->func_wr(char_out, io_functions->context_wr))
		{
			return FALSE;
		}
	}
	ringbuffer_reset(ringbuffer);
	return TRUE;
}

/* ======================================================================= */
/* Search & Replace                                                        */
/* ======================================================================= */

BOOL libreplace_search_and_replace(const libreplace_io_t *const io_functions, const libreplace_logger_t *const logger, const BYTE *const needle, const LONG needle_len, const BYTE *const replacement, const LONG replacement_len, const libreplace_flags_t *const options, volatile BOOL *const abort_flag)
{
	BOOL success = FALSE, pending_input = FALSE, error_flag = FALSE;
	BYTE char_in, char_out;
	DWORD replacement_count = 0U;
	ULARGE_INTEGER position = { 0U, 0U };
	ringbuffer_t *ringbuffer = NULL;

	/* check parameters */
	if(!(io_functions && needle && replacement && (needle_len > 0L) && (replacement_len >= 0L) && options && abort_flag))
	{
		libreplace_print(logger, "Invalid function parameters!\n");
		goto finished;
	}

	/* check length limitation */
	if((needle_len > LIBREPLACE_MAXLEN) || (replacement_len > LIBREPLACE_MAXLEN))
	{
		libreplace_print(logger, "Needle and/or replacement length exceeds the allowable limit!\n");
		goto finished;
	}

	/* allocate ring buffer */
	ringbuffer = ringbuffer_alloc(needle_len);
	if(!ringbuffer)
	{
		libreplace_print(logger, "Memory allocation has failed!\n");
		goto finished;
	}

	/* process all available input data */
	while(pending_input = io_functions->func_rd(&char_in, io_functions->context_rd, &error_flag))
	{
		/* add next character to buffer*/
		if(ringbuffer_append(char_in, &char_out, ringbuffer))
		{
			if(!io_functions->func_wr(char_out, io_functions->context_wr))
			{
				libreplace_print(logger, WR_ERROR_MESSAGE);
				goto finished;
			}
		}

		/* if a match has been found, write the replacement */
		if(ringbuffer_compare(ringbuffer, needle, needle_len, options->case_insensitive, options->wildcard))
		{
			++replacement_count;
			if(options->verbose || options->dry_run)
			{
				libreplace_print_fmt(logger, options->dry_run ? "Found occurence at offset: 0x%08lX%08lX\n" : "Replaced occurence at offset: 0x%08lX%08lX\n", position.HighPart, position.LowPart);
			}
			if(!options->dry_run)
			{
				if(!libreplace_write(replacement, replacement_len, io_functions))
				{
					libreplace_print(logger, WR_ERROR_MESSAGE);
					goto finished;
				}
				ringbuffer_reset(ringbuffer);
			}
			else
			{
				if(!libreplace_flush_pending(ringbuffer, io_functions))
				{
					libreplace_print(logger, WR_ERROR_MESSAGE);
					goto finished;
				}
			}
			if(options->replace_once)
			{
				break;
			}
		}

		/*incremet the file position*/
		++position.QuadPart;

		/* check if abort was requested */
		CHECK_ABORT_REQUEST();
	}

	/* write any pending data */
	if(!libreplace_flush_pending(ringbuffer, io_functions))
	{
		libreplace_print(logger, WR_ERROR_MESSAGE);
		goto finished;
	}

	/* check if abort was requested */
	CHECK_ABORT_REQUEST();

	/* transfer any input data not processed yet */
	if(pending_input)
	{
		while(io_functions->func_rd(&char_in, io_functions->context_rd, &error_flag))
		{
			if(!io_functions->func_wr(char_in, io_functions->context_wr))
			{
				libreplace_print(logger, WR_ERROR_MESSAGE);
				goto finished;
			}
		}
	}

	/* check if abort was requested */
	CHECK_ABORT_REQUEST();

	/* check for any previous read errors */
	if(error_flag)
	{
		libreplace_print(logger, RD_ERROR_MESSAGE);
		goto finished;
	}

	/* flush output buffers*/
	success = io_functions->func_wr(LIBREPLACE_FLUSH, io_functions->context_wr);

	if(options->verbose)
	{
		libreplace_print_fmt(logger, options->dry_run ? "Total occurences found: %lu\n" : "Total occurences replaced: %lu\n", replacement_count);
	}

finished:

	if(ringbuffer)
	{
		LocalFree(ringbuffer);
	}

	return success;
}
