/******************************************************************************/
/* Replace, by LoRd_MuldeR <MuldeR2@GMX.de>                                   */
/* This work has been released under the CC0 1.0 Universal license!           */
/******************************************************************************/

#include "libreplace/replace.h"

#ifdef _DEBUG
#define MY_ASSERT(X) do { if(!((X))) FatalExit(-1); } while(0)
#else
#define MY_ASSERT(X) __noop((X))
#endif

static const CHAR  CHAR_LF = 0x0A;
static const CHAR  CHAR_CR = 0x0D;

#define TO_UPPER(X) ((((X) >= 0x61) && ((X) <= 0x7A)) ? ((X) - 0x20) : (X))
#define IS_WILDCARD(X,Y) (wildcard_map && wildcard_map[(X)] && (options->match_crlf || (((Y) != CHAR_LF) && ((Y) != CHAR_CR))))
#define COMPARE_CHAR(X,Y) (options->case_insensitive ? (TO_UPPER(X) == TO_UPPER(Y)) : ((X) == (Y)))

#define INCREMENT(VALUE, LIMIT) do \
{ \
	if(++(VALUE) >= (LIMIT)) { (VALUE) = 0U; } \
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
	DWORD capacity;
	DWORD valid;
	DWORD index_write;
	DWORD index_flush;
	BYTE buffer[];
}
ringbuffer_t;

static ringbuffer_t *ringbuffer_alloc(const DWORD capacity)
{
	if((capacity > 0U) && (capacity < MAXDWORD))
	{
		ringbuffer_t *const ringbuffer = (ringbuffer_t*) LocalAlloc(LPTR, sizeof(ringbuffer_t) + (sizeof(BYTE) * capacity));
		if(ringbuffer)
		{
			ringbuffer->capacity = capacity;
			ringbuffer->index_write = ringbuffer->valid = 0U;
			ringbuffer->index_flush = MAXDWORD;
			return ringbuffer;
		}
	}
	return NULL;
}

static __inline BOOL ringbuffer_append(const BYTE in, BYTE *const ptr_out, ringbuffer_t *const ringbuffer)
{
	MY_ASSERT(ringbuffer->index_flush == MAXDWORD);
	*ptr_out = ringbuffer->buffer[ringbuffer->index_write];
	ringbuffer->buffer[ringbuffer->index_write] = in;
	INCREMENT(ringbuffer->index_write, ringbuffer->capacity);
	if(ringbuffer->valid < ringbuffer->capacity)
	{
		++ringbuffer->valid;
		return FALSE;
	}
	return TRUE;
}

static __forceinline BYTE ringbuffer_peek(const ringbuffer_t *const ringbuffer)
{
	return ringbuffer->buffer[ringbuffer->index_write];
}

static __inline BOOL ringbuffer_compare(const ringbuffer_t *const ringbuffer, const BYTE *needle, const BOOL *const wildcard_map, const libreplace_flags_t *const options)
{
	if(ringbuffer->index_write == 0U)
	{
		DWORD needle_pos;
		for(needle_pos = 1U; needle_pos < ringbuffer->capacity; ++needle_pos) /*first element is skipped intentionally!*/
		{
			if((!IS_WILDCARD(needle_pos, ringbuffer->buffer[needle_pos])) && (!COMPARE_CHAR(ringbuffer->buffer[needle_pos], needle[needle_pos])))
			{
				return FALSE;
			}
		}
	}
	else
	{
		DWORD needle_pos, buffer_pos = ringbuffer->index_write;
		INCREMENT(buffer_pos, ringbuffer->capacity);
		for(needle_pos = 1U; needle_pos < ringbuffer->capacity; ++needle_pos) /*first element is skipped intentionally!*/
		{
			if((!IS_WILDCARD(needle_pos, ringbuffer->buffer[buffer_pos])) && (!COMPARE_CHAR(ringbuffer->buffer[buffer_pos], needle[needle_pos])))
			{
				return FALSE;
			}
			INCREMENT(buffer_pos, ringbuffer->capacity);
		}
	}
	return TRUE;
}

static __inline BOOL ringbuffer_flush(BYTE *const out, ringbuffer_t *const ringbuffer)
{
	if(ringbuffer->index_flush == MAXDWORD)
	{
		ringbuffer->index_flush = (ringbuffer->valid >= ringbuffer->capacity) ? ringbuffer->index_write : 0U;
	}
	if(ringbuffer->valid > 0U)
	{
		*out = ringbuffer->buffer[ringbuffer->index_flush];
		--ringbuffer->valid;
		INCREMENT(ringbuffer->index_flush, ringbuffer->capacity);
		return TRUE;
	}
	return FALSE;
}

static __inline void ringbuffer_reset(ringbuffer_t *const ringbuffer)
{
	ringbuffer->index_write = ringbuffer->valid = 0U;
	ringbuffer->index_flush = MAXDWORD;
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
	CHAR temp[256U];
	BOOL result = FALSE;
	va_list ap;
	va_start(ap, format);
	if(wvsprintfA(temp, format, ap) > 0L)
	{
		result = libreplace_print(logger, temp);
	}
	va_end(ap);
	return result;
}

static __inline BOOL libreplace_normalize(BYTE *const char_ptr, BYTE *const last_linbreak)
{
	if((*char_ptr == CHAR_LF) || (*char_ptr == CHAR_CR))
	{
		if((*last_linbreak != 0U) && (*char_ptr != *last_linbreak))
		{
			*last_linbreak = 0U;
			return FALSE;
		}
		*last_linbreak = *char_ptr;
		*char_ptr = CHAR_LF;
	}
	else
	{
		*last_linbreak = 0U; /*not a line-break*/
	}
	return TRUE;
}

static __inline BOOL libreplace_write(const BYTE *const data, const DWORD data_len, const libreplace_io_t *const io_functions)
{
	DWORD data_pos;
	for(data_pos = 0U; data_pos < data_len; ++data_pos)
	{
		if(!io_functions->func_wr(data[data_pos], io_functions->context_wr))
		{
			return FALSE;
		}
	}
	return TRUE;
}

static __inline BOOL libreplace_flush_pending(ringbuffer_t *const ringbuffer, const libreplace_io_t *const io_functions)
{
	BYTE temp;
	while(ringbuffer_flush(&temp, ringbuffer))
	{
		if(!io_functions->func_wr(temp, io_functions->context_wr))
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

BOOL libreplace_search_and_replace(const libreplace_io_t *const io_functions, const libreplace_logger_t *const logger, const BYTE *const needle, const BOOL *const wildcard_map, const DWORD needle_len, const BYTE *const replacement, const DWORD replacement_len, const libreplace_flags_t *const options, volatile BOOL *const abort_flag)
{
	BYTE char_in, char_out, last_linbreak = 0U;
	BOOL success = FALSE, pending_input = FALSE, error_flag = FALSE;
	DWORD replacement_count = 0U;
	ULARGE_INTEGER position = { 0U, 0U };
	ringbuffer_t *ringbuffer = NULL;

	/* check parameters */
	if(!(io_functions && needle && replacement && (needle_len > 0U) && (replacement_len >= 0U) && options && abort_flag))
	{
		libreplace_print(logger, "Invalid function parameters detected!\n");
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
		libreplace_print(logger, "Failed to allocate ring buffer!\n");
		goto finished;
	}

	/* process all available input data */
	while(pending_input = io_functions->func_rd(&char_in, io_functions->context_rd, &error_flag))
	{
		/* fix up CRLF/LFCR line-breaks, if normalization is enabled */
		if(options->normalize)
		{
			if(!libreplace_normalize(&char_in, &last_linbreak))
			{
				continue; /*discard char*/
			}
		}

		/* add next character to buffer*/
		if(ringbuffer_append(char_in, &char_out, ringbuffer))
		{
			if(!io_functions->func_wr(char_out, io_functions->context_wr))
			{
				libreplace_print(logger, WR_ERROR_MESSAGE);
				goto finished;
			}
		}

		/* make sure enough input data is in the buffer */
		if(ringbuffer->valid < needle_len)
		{
			goto skip_check;
		}

		/* perfrom quick pre-test on the first character in the buffer */
		if(IS_WILDCARD(0U, ringbuffer_peek(ringbuffer)) || COMPARE_CHAR(ringbuffer_peek(ringbuffer), needle[0U]))
		{
			/* if a full match is found, write the replacement */
			if(ringbuffer_compare(ringbuffer, needle, wildcard_map, options))
			{
				++replacement_count;
				if(options->verbose || options->dry_run)
				{
					libreplace_print_fmt(logger, "%s occurence at offset: 0x%08lX%08lX\n", options->dry_run ? "Found" : "Replaced", position.HighPart, position.LowPart);
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
		}

	skip_check:

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
