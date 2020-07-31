/******************************************************************************/
/* Replace, by LoRd_MuldeR <MuldeR2@GMX.de>                                   */
/* This work has been released under the CC0 1.0 Universal license!           */
/*                                                                            */
/* This program implements a variant of the "KMP" string-searching algorithm. */
/* See here for information:                                                  */
/* https://en.wikipedia.org/wiki/Knuth%E2%80%93Morris%E2%80%93Pratt_algorithm */
/******************************************************************************/

#define WIN32_LEAN_AND_MEAN 1

#include <Windows.h>
#include <ShellAPI.h>
#include <wincrypt.h>

#define VERSION_MAJOR 1
#define VERSION_MINOR 3
#define VERSION_PATCH 0

#ifdef _DEBUG
#define HAVE_TRACE 1
#define TRACE(OUT, FMT, ...) print_message_fmt((OUT), "[DEBUG] " FMT, __VA_ARGS__)
#else
#define HAVE_TRACE 0
#define TRACE(OUT, FMT, ...) __noop()
#endif

/* ======================================================================= */
/* Utilities                                                               */
/* ======================================================================= */

static __inline void increment(LONG *const value, const LONG limit)
{
	*value += 1U;
	if(*value >= limit)
	{
		*value = 0U;
	}
}

static __inline ULARGE_INTEGER make_uint64(const ULONGLONG value)
{
	ULARGE_INTEGER result;
	result.QuadPart = value;
	return result;
}

/* ======================================================================= */
/* String Routines                                                         */
/* ======================================================================= */

#define CP_1252 1252
#define TO_UPPER(C) ((((C) >= 0x61) && ((C) <= 0x7A)) ? ((C) - 0x20) : (C))

#define NOT_EMPTY(STR) ((STR) && ((STR)[0U]))
#define EMPTY(STR) (!NOT_EMPTY(STR))

#define INVALID_CHAR 0xFF

static BYTE *utf16_to_bytes(const WCHAR *const input, LONG *const length_out, const UINT code_page)
{
	BYTE *buffer;
	DWORD buffer_size = 0U, result = 0U;

	buffer_size = WideCharToMultiByte(code_page, 0, input, -1, NULL, 0, NULL, NULL);
	if(buffer_size < 1U)
	{
		return NULL;
	}

	buffer = (BYTE*) LocalAlloc(LPTR, sizeof(BYTE) * buffer_size);
	if(!buffer)
	{
		return NULL;
	}

	result = WideCharToMultiByte(code_page, 0, input, -1, (LPSTR)buffer, buffer_size, NULL, NULL);
	if((result > 0U) && (result <= buffer_size))
	{
		*length_out = result - 1U;
		return buffer;
	}

	LocalFree(buffer);
	return NULL;
}

static __inline BOOL compare_char(const BYTE char_a, const BYTE char_b, const BOOL ignore_case)
{
	return ignore_case ? (TO_UPPER(char_a) == TO_UPPER(char_b)) : (char_a == char_b);
}

static __inline BYTE decode_hex_char(const WCHAR c)
{
	if ((c >= L'0') && (c <= L'9'))
	{
		return (BYTE)(c - L'0');
	}
	else if ((c >= L'A') && (c <= L'F'))
	{
		return (BYTE)(c - L'A' + 10);
	}
	else if ((c >= L'a') && (c <= L'f'))
	{
		return (BYTE)(c - L'a' + 10);
	}
	else
	{
		return INVALID_CHAR;
	}
}

static BYTE *decode_hex_string(const WCHAR *input, LONG *const length_out)
{
	DWORD len, pos;
	BYTE *result;

	if((input[0U] == L'0') && (input[1U] == L'x'))
	{
		input += 2U;
	}

	len = lstrlenW(input);
	if((len < 1U) || ((len % 2U) != 0U))
	{
		return NULL;
	}

	if(!(result = (BYTE*) LocalAlloc(LPTR, sizeof(BYTE) * (len /= 2U))))
	{
		return NULL;
	}

	for(pos = 0U; pos < len; ++pos, input += 2U)
	{
		const BYTE val[] = { decode_hex_char(input[0U]), decode_hex_char(input[1U]) };
		if((val[0U] == INVALID_CHAR) || (val[1U] == INVALID_CHAR))
		{
			LocalFree(result);
			return NULL;
		}
		result[pos] = (val[0U] << 4U) | val[1U];
	}

	*length_out = len;
	return result;
}

/* ======================================================================= */
/* Random Numbers                                                          */
/* ======================================================================= */

static __inline DWORD random_seed(void)
{
	LARGE_INTEGER count;
	if(QueryPerformanceCounter(&count))
	{
		return 31U * GetCurrentProcessId() + count.LowPart;
	}
	else
	{
		return 31U * GetCurrentProcessId() + GetTickCount();
	}
}

static __inline DWORD random_next(DWORD *const seed)
{
	*seed *= 1664525U;
	*seed += 1013904223U;
	return *seed;
}

/* ======================================================================= */
/* File System Routines                                                    */
/* ======================================================================= */

static BOOL path_exists(const WCHAR *const path)
{
	return (GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES);
}

static BOOL file_exists(const WCHAR *const path)
{
	const DWORD attribs = GetFileAttributesW(path);
	return (attribs != INVALID_FILE_ATTRIBUTES) && (!(attribs & FILE_ATTRIBUTE_DIRECTORY));
}

static BOOL get_readonly_attribute(const HANDLE handle)
{
	BY_HANDLE_FILE_INFORMATION file_info;
	SecureZeroMemory(&file_info, sizeof(BY_HANDLE_FILE_INFORMATION));
	if(GetFileInformationByHandle(handle, &file_info))
	{
		if((file_info.dwFileAttributes != INVALID_FILE_ATTRIBUTES) && (file_info.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
		{
			return TRUE;
		}
	}
	return FALSE;
}

static const WCHAR *get_directory_part(const WCHAR *const file_path)
{
	WCHAR *buffer = NULL;
	DWORD pos, last_sep = 0U;
	BOOL flag = FALSE;

	for(pos = 0U; file_path[pos]; ++pos)
	{
		if((file_path[pos] == L'\\') || (file_path[pos] == L'/'))
		{
			if(flag)
			{
				last_sep = pos;
				flag = FALSE;
			}
		}
		else
		{
			flag = TRUE;
		}
	}

	if(buffer = (WCHAR*) LocalAlloc(LPTR, sizeof(WCHAR) * (last_sep + 1U)))
	{
		if(last_sep > 0U)
		{
			lstrcpynW(buffer, file_path, last_sep + 1U);
		}
		return buffer;
	}

	return NULL;
}

static const WCHAR *generate_temp_file(const WCHAR *const directory)
{
	static const WCHAR *const RAND_TEMPLATE = L"%s\\~%07X.tmp";
	static const DWORD RAND_MASK = 0xFFFFFFF;
	const WCHAR *const prefix = NOT_EMPTY(directory) ? directory : L".";

	WCHAR *const temp = (WCHAR*) LocalAlloc(LPTR, sizeof(WCHAR) * (lstrlenW(prefix) + 14U));
	if(temp)
	{
		DWORD round;
		for(round = 0U; round < 997U; ++round)
		{
			DWORD rseed = random_seed();
			DWORD retry;
			for(retry = 0U; retry <= 9973U; ++retry)
			{
				wsprintfW(temp, RAND_TEMPLATE, prefix, random_next(&rseed) & RAND_MASK);
				if(!path_exists(temp))
				{
					return temp;
				}
			}
		}
		LocalFree(temp);
	}

	return NULL;
}

static const BOOL move_file(const WCHAR *const file_src, const WCHAR *const file_dst)
{
	DWORD retry;
	for(retry = 0U; retry < 128; ++retry)
	{
		if(retry > 0U)
		{
			Sleep(retry); /*delay before retry*/
		}
		if(MoveFileEx(file_src, file_dst, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH))
		{
			return TRUE;
		}
	}
	return FALSE;
}

static const BOOL delete_file(const WCHAR *const file_path)
{
	DWORD retry;
	for(retry = 0U; retry < 128; ++retry)
	{
		if(retry > 0U)
		{
			Sleep(retry); /*delay before retry*/
		}
		if(DeleteFileW(file_path))
		{
			return TRUE;
		}
		else
		{
			if(GetLastError() == ERROR_FILE_NOT_FOUND)
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

/* ======================================================================= */
/* I/O Routines                                                            */
/* ======================================================================= */

#define BUFF_SIZE 4096U

typedef struct input_context_t
{ 
	DWORD avail;
	DWORD pos;
	BYTE buffer[BUFF_SIZE];
}
input_context_t;

typedef struct output_context_t
{ 
	DWORD pos;
	BYTE buffer[BUFF_SIZE];
}
output_context_t;

static __inline BOOL read_next_byte(BYTE *const output, const HANDLE input, input_context_t *const ctx, BOOL *const error_flag)
{
	if(ctx->pos >= ctx->avail)
	{
		ctx->pos = 0U;
		if(!ReadFile(input, ctx->buffer, BUFF_SIZE, &ctx->avail, NULL))
		{
			const DWORD error_code = GetLastError();
			if((error_code != ERROR_HANDLE_EOF) && (error_code != ERROR_BROKEN_PIPE))
			{
				*error_flag = TRUE;
			}
			return FALSE;
		}
		if(ctx->avail < 1U)
		{
			return FALSE;
		}
	}
	*output = ctx->buffer[ctx->pos++];
	return TRUE;
}

static __inline BOOL write_next_byte(const BYTE input, const HANDLE output, output_context_t *const ctx, const BOOL sync)
{
	ctx->buffer[ctx->pos++] = input;
	if(ctx->pos >= BUFF_SIZE)
	{
		DWORD bytes_written;
		ctx->pos = 0U;
		if(!WriteFile(output, ctx->buffer, BUFF_SIZE, &bytes_written, NULL))
		{
			return FALSE;
		}
		if(bytes_written < BUFF_SIZE)
		{
			return FALSE;
		}
		if(sync)
		{
			FlushFileBuffers(output);
		}
	}
	return TRUE;
}

static __inline BOOL write_byte_array(const BYTE *const input, const DWORD input_len, const HANDLE output, output_context_t *const ctx, const BOOL sync)
{
	DWORD input_pos;
	for(input_pos = 0U; input_pos < input_len; ++input_pos)
	{
		if(!write_next_byte(input[input_pos], output, ctx, FALSE))
		{
			return FALSE;
		}
		if(sync)
		{
			FlushFileBuffers(output);
		}
	}
	return TRUE;
}

static __inline BOOL flush_pending_bytes(const HANDLE output, output_context_t *const ctx, const BOOL sync)
{
	if(ctx->pos > 0)
	{
		DWORD bytes_written;
		if(!WriteFile(output, ctx->buffer, ctx->pos, &bytes_written, NULL))
		{
			return FALSE;
		}
		if(bytes_written < ctx->pos)
		{
			return FALSE;
		}
		if(sync)
		{
			FlushFileBuffers(output);
		}
	}
	return TRUE;
}

static __inline BOOL print_message(const HANDLE output, const CHAR *const text)
{
	DWORD bytes_written;
	return WriteFile(output, text, lstrlenA(text), &bytes_written, NULL);
}

static __inline BOOL print_message_fmt(const HANDLE output, const CHAR *const format, ...)
{
	CHAR temp[256U];
	va_list ap;
	int result;
	va_start(ap, format);
	result = wvsprintfA(temp, format, ap);
	va_end(ap);
	if(result > 0)
	{
		return print_message(output, temp);
	}
	return FALSE;
}

/* ======================================================================= */
/* Command-line Options                                                    */
/* ======================================================================= */

typedef struct options_t
{ 
	BOOL show_help;
	BOOL case_insensitive;
	BOOL replace_once;
	BOOL ansi_cp;
	BOOL force_sync;
	BOOL verbose;
	BOOL binary_mode;
	BOOL self_test;
}
options_t;

static int parse_options(const HANDLE std_err, const int argc, const LPWSTR *const argv, int *const index, options_t *const options)
{
	DWORD flag_pos;
	SecureZeroMemory(options, sizeof(options_t));
	while(*index < argc)
	{
		const WCHAR *const value = argv[*index];
		if((value[0U] == L'-') && (value[1U] != L'\0'))
		{
			*index += 1U;
			if((value[1U] == L'-') && (value[2U] == L'\0'))
			{
				return TRUE; /*stop here!*/
			}
			for(flag_pos = 1U; value[flag_pos]; ++flag_pos)
			{
				switch(value[flag_pos])
				{
				case L'h':
				case L'?':
					options->show_help = TRUE;
					break;
				case L'i':
					options->case_insensitive = TRUE;
					break;
				case L's':
					options->replace_once = TRUE;
					break;
				case L'a':
					options->ansi_cp = TRUE;
					break;
				case L'f':
					options->force_sync = TRUE;
					break;
				case L'v':
					options->verbose = TRUE;
					break;
				case L'b':
					options->binary_mode = TRUE;
					break;
				case L't':
					options->self_test = TRUE;
					break;
				default:
					print_message(std_err, "Error: Invalid command-line option encountered!\n");
					return FALSE;
				}
			}
		}
		else
		{
			break; /*no more options*/
		}
	}
	return TRUE;
}

/* ======================================================================= */
/* Search & Replace                                                        */
/* ======================================================================= */

static const char *const WR_ERROR_MESSAGE = "Write operation failed -> aborting!\n";
static const char *const RD_ERROR_MESSAGE = "Read operation failed -> aborting!\n";

static LONG *compute_prefixes(const HANDLE std_err, const BYTE *const needle, const LONG needle_len)
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
		print_message(std_err, "[DEBUG] prefix: ");
		for(needle_pos = 0U; needle_pos < needle_len; ++needle_pos)
		{
			print_message_fmt(std_err, (needle_pos > 0L) ? ", %ld" : "%ld", prefix[needle_pos]);
		}
		print_message(std_err, "\n");
	}

	return prefix;
}

static BOOL search_and_replace(const HANDLE input, const HANDLE output, const HANDLE std_err, const BYTE *const needle, const LONG needle_len, const BYTE *const replacement, const LONG replacement_len, const options_t *const options)
{
	BOOL success = FALSE, pending_input = FALSE, error_flag = FALSE;
	input_context_t *input_ctx = NULL; output_context_t *output_ctx  = NULL;
	BYTE char_input;
	LONG *prefix = NULL, needle_pos = 0L;
	DWORD replacement_count = 0U;
	ULARGE_INTEGER position = { 0U, 0U };

	input_ctx = (input_context_t*) LocalAlloc(LPTR, sizeof(input_context_t));
	if(!input_ctx)
	{
		goto finished;
	}

	output_ctx = (output_context_t*) LocalAlloc(LPTR, sizeof(output_context_t));
	if(!output_ctx)
	{
		goto finished;
	}

	prefix = compute_prefixes(std_err, needle, needle_len);
	if(!prefix)
	{
		goto finished;
	}

	/* process all available input data */
	while(pending_input = read_next_byte(&char_input, input, input_ctx, &error_flag))
	{
		/* dump the initial status */
		TRACE(std_err, "position: 0x%08lX%08lX\n", position.HighPart, position.LowPart);
		TRACE(std_err, "needle_pos[old]: %ld\n", needle_pos);

		/* update input byte counter */
		++position.QuadPart;

		/* if prefix cannot be extended, search for a shorter prefix */
		while ((needle_pos >= 0L) && (!compare_char(char_input, needle[needle_pos], options->case_insensitive)))
		{
			TRACE(std_err, "mismatch: %ld --> %ld\n", needle_pos, prefix[needle_pos]);
			/* write discarded part of old prefix */
			if((needle_pos > 0L) && (prefix[needle_pos] < needle_pos))
			{
				if(!write_byte_array(needle, needle_pos - prefix[needle_pos], output, output_ctx, options->force_sync))
				{
					print_message(std_err, WR_ERROR_MESSAGE);
					goto finished;
				}
			}
			needle_pos = prefix[needle_pos];
		}

		/* write the input character, if it did *not* match */
		if(needle_pos < 0L)
		{
			if(!write_next_byte(char_input, output, output_ctx, options->force_sync))
			{
				print_message(std_err, WR_ERROR_MESSAGE);
				goto finished;
			}
		}

		/* dump the updated status */
		TRACE(std_err, "needle_pos[new]: %ld\n", needle_pos);

		/* if a full match has been found, write the replacement instead */
		if (++needle_pos >= needle_len)
		{
			++replacement_count;
			needle_pos = 0U;
			if(options->verbose)
			{
				const ULARGE_INTEGER match_start = make_uint64(position.QuadPart - needle_len);
				print_message_fmt(std_err, "Replaced occurence at offset: 0x%08lX%08lX\n", match_start.HighPart, match_start.LowPart);
			}
			if(!write_byte_array(replacement, replacement_len, output, output_ctx, options->force_sync))
			{
				print_message(std_err, WR_ERROR_MESSAGE);
				goto finished;
			}
			if(options->replace_once)
			{
				if(options->verbose)
				{
					print_message(std_err, "Stopping search & replace after *first* match.\n");
				}
				break;
			}
		}
	}

	/* dump the final status */
	TRACE(std_err, "needle_pos[fin]: %ld\n", needle_pos);

	/* write any pending data */
	if(needle_pos > 0L)
	{	
		if(!write_byte_array(needle, needle_pos, output, output_ctx, options->force_sync))
		{
			print_message(std_err, WR_ERROR_MESSAGE);
			goto finished;
		}
	}

	/* transfer any input data not processed yet */
	if(pending_input)
	{
		while(read_next_byte(&char_input, input, input_ctx, &error_flag))
		{
			if(!write_next_byte(char_input, output, output_ctx, options->force_sync))
			{
				print_message(std_err, WR_ERROR_MESSAGE);
				goto finished;
			}
		}
	}

	/* check for any previous read errors */
	if(error_flag)
	{
		print_message(std_err, RD_ERROR_MESSAGE);
		goto finished;
	}

	/* flush output buffers*/
	success = flush_pending_bytes(output, output_ctx, options->force_sync);

	if(options->verbose)
	{
		print_message_fmt(std_err, "Total occurences replaced: %lu\n", replacement_count);
	}

finished:

	if(input_ctx)
	{
		LocalFree(input_ctx);
	}

	if(output_ctx)
	{
		LocalFree(output_ctx);
	}

	if(prefix)
	{
		LocalFree(prefix);
	}

	return success;
}

/* ======================================================================= */
/* Self-test                                                               */
/* ======================================================================= */

#define RUN_TEST(X, ...) do \
{ \
	if(run_test(std_err, __VA_ARGS__)) \
	{ \
		print_message(std_err, "[Self-Test] Test case #" #X " succeeded.\n"); \
	} \
	else \
	{ \
		print_message(std_err, "[Self-Test] Test case #" #X " failed !!!\n"); \
		success = FALSE; \
	} \
} \
while(0)

static BOOL write_test_file(const WCHAR *const file_name, const BYTE *const data, const DWORD data_len)
{
	BOOL success = FALSE;
	const HANDLE handle = CreateFileW(file_name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if(handle != NULL)
	{
		DWORD bytes_written = 0U;
		if(WriteFile(handle, data, data_len, &bytes_written, NULL))
		{
			success = (bytes_written >= data_len);
		}
		CloseHandle(handle);
	}
	return success;
}

static BYTE *read_test_file(const WCHAR *const file_name, DWORD *const length)
{
	BYTE *result = NULL;
	const HANDLE handle = CreateFileW(file_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if(handle != NULL)
	{
		LARGE_INTEGER size;
		if(GetFileSizeEx(handle, &size))
		{
			if(!size.HighPart)
			{
				if(result = (BYTE*) LocalAlloc(LPTR, sizeof(BYTE) * size.LowPart))
				{
					BOOL okay = FALSE;
					if(ReadFile(handle, result, size.LowPart, length, NULL))
					{
						okay = (*length >= size.LowPart);
					}
					if(!okay)
					{
						LocalFree(result);
						result = NULL;
					}
				}
			}
		}
		CloseHandle(handle);
	}
	return result;
}

static BOOL run_test(const HANDLE std_err, const CHAR *const needle, const CHAR *const replacement, const CHAR *const haystack, const CHAR *const expected)
{
	BOOL success = FALSE;
	options_t options;
	const WCHAR *source_file = NULL, *output_file = NULL;
	HANDLE input = INVALID_HANDLE_VALUE, output = INVALID_HANDLE_VALUE;
	DWORD result_len = 0U;
	const CHAR *result = NULL;

	if(!(source_file = generate_temp_file(NULL)))
	{
		goto cleanup;
	}

	if(!write_test_file(source_file, (const BYTE*)haystack, lstrlenA(haystack)))
	{
		goto cleanup;
	}

	input = CreateFileW(source_file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if(input == INVALID_HANDLE_VALUE)
	{
		goto cleanup;
	}

	if(!(output_file = generate_temp_file(NULL)))
	{
		goto cleanup;
	}

	output = CreateFileW(output_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if(output == INVALID_HANDLE_VALUE)
	{
		goto cleanup;
	}

	SecureZeroMemory(&options, sizeof(options_t));
	if(!search_and_replace(input, output, std_err, (const BYTE*)needle, lstrlenA(needle), (const BYTE*)replacement, lstrlenA(replacement), &options))
	{
		goto cleanup;
	}

	CloseHandle(input);
	CloseHandle(output);

	input = output = INVALID_HANDLE_VALUE;
	
	if(result = (const CHAR*)read_test_file(output_file, &result_len))
	{
		success = (CompareStringA(LOCALE_INVARIANT, 0, result, result_len, expected, lstrlenA(expected)) == CSTR_EQUAL);
	}

cleanup:

	if(input != INVALID_HANDLE_VALUE)
	{
		CloseHandle(input);
	}

	if(output != INVALID_HANDLE_VALUE)
	{
		CloseHandle(output);
	}

	if(source_file)
	{
		delete_file(source_file);
		LocalFree((HLOCAL)source_file);
	}

	if(output_file)
	{
		delete_file(output_file);
		LocalFree((HLOCAL)output_file);
	}

	if(result)
	{
		LocalFree((HLOCAL)result);
	}

	return success;
}

static BOOL self_test(const HANDLE std_err)
{
	BOOL success = TRUE;

	RUN_TEST(1, "LTs3kx", "XJbf3A",
		"KJnsbsniWReHocwWghHKmtwue7zLXvT9Ai3twkgmHRahFxTV3EggbHptv7toJhdKWCyJ93vPmUqXVtwCuJvpvY9Avu4cojuRwknv7HCYpyNvzJWtdwvEEpsNNyq9JAay",
		"KJnsbsniWReHocwWghHKmtwue7zLXvT9Ai3twkgmHRahFxTV3EggbHptv7toJhdKWCyJ93vPmUqXVtwCuJvpvY9Avu4cojuRwknv7HCYpyNvzJWtdwvEEpsNNyq9JAay");

	RUN_TEST(2, "LTs3kx", "XJbf3A",
		"KJnsbsniWReHocwWghHKmtwue7zLXvT9Ai3twkgmHRahFxTV3EggbHpLTs3kxhdKWCyJ93vPmUqXVtwCuJvpvY9Avu4cojuRwknv7HCYpyNvzJWtdwvEEpsNNyq9JAay",
		"KJnsbsniWReHocwWghHKmtwue7zLXvT9Ai3twkgmHRahFxTV3EggbHpXJbf3AhdKWCyJ93vPmUqXVtwCuJvpvY9Avu4cojuRwknv7HCYpyNvzJWtdwvEEpsNNyq9JAay");

	RUN_TEST(3, "LTs3kx", "",
		"KJnsbsniWReHocwWghHKmtwue7zLXvT9Ai3twkgmHRahFxTV3EggbHpLTs3kxhdKWCyJ93vPmUqXVtwCuJvpvY9Avu4cojuRwknv7HCYpyNvzJWtdwvEEpsNNyq9JAay",
		"KJnsbsniWReHocwWghHKmtwue7zLXvT9Ai3twkgmHRahFxTV3EggbHphdKWCyJ93vPmUqXVtwCuJvpvY9Avu4cojuRwknv7HCYpyNvzJWtdwvEEpsNNyq9JAay");

	RUN_TEST(4, "LTs3kx", "H4n3zWoHKfbX",
		"KJnsbsniWReHocwWghHKmtwue7zLXvT9Ai3twkgmHRahFxTV3EggbHpLTs3kxhdKWCyJ93vPmUqXVtwCuJvpvY9Avu4cojuRwknv7HCYpyNvzJWtdwvEEpsNNyq9JAay",
		"KJnsbsniWReHocwWghHKmtwue7zLXvT9Ai3twkgmHRahFxTV3EggbHpH4n3zWoHKfbXhdKWCyJ93vPmUqXVtwCuJvpvY9Avu4cojuRwknv7HCYpyNvzJWtdwvEEpsNNyq9JAay");

	RUN_TEST(5, "ababaa", "XJbf3A",
		"aaabaaaabbbaaabbaaaabaaaaabbabbaaaa3aabbbabbaabbbbabbabbbbbbbaabaaaabbaaabbbaaabbbaaaaababaaaaabaaabababaabbabbbabaabaAabaaabbaa",
		"aaabaaaabbbaaabbaaaabaaaaabbabbaaaa3aabbbabbaabbbbabbabbbbbbbaabaaaabbaaabbbaaabbbaaaaXJbf3AaaabaaabXJbf3AbbabbbabaabaAabaaabbaa");

	RUN_TEST(6, "abcabd", "XJbf3A",
		"abbaccddbcccacbddabcabcdacdbcabcdcccbcdcadbdddcabbcadcdccbabaabacccccabcababcabddbcbbcaadccab4dbaddbdccbdcdbcXccbbbcabbaabdcadccd",
		"abbaccddbcccacbddabcabcdacdbcabcdcccbcdcadbdddcabbcadcdccbabaabacccccabcabXJbf3Adbcbbcaadccab4dbaddbdccbdcdbcXccbbbcabbaabdcadccd");
	
	RUN_TEST(7, "bcbbab", "XJbf3A",
		"cbcaccbcaaaacccbaacaaccc3cbccbbbcaacbbcbabEaabaacccccbccbcbabacabbbcbbcbacccbabcabaccaabaaabbabcaababaabacbccbbccbaccccaccbcbbab",
		"cbcaccbcaaaacccbaacaaccc3cbccbbbcaacbbcbabEaabaacccccbccbcbabacabbbcbbcbacccbabcabaccaabaaabbabcaababaabacbccbbccbaccccaccXJbf3A");

	RUN_TEST(8, "bcbbab", "XJbf3A",
		"cbcaccbcaaaacccbaacaaccc3cbccbbbcaacbbcbabEaabaacccccbccbcbabacabbbcbbcbacccbabcabaccaabaaabbabcaababaabacbccbbccbaccccaccbbcbba",
		"cbcaccbcaaaacccbaacaaccc3cbccbbbcaacbbcbabEaabaacccccbccbcbabacabbbcbbcbacccbabcabaccaabaaabbabcaababaabacbccbbccbaccccaccbbcbba");

	RUN_TEST(9, "bcbbab", "XJbf3A",
		"bcbbacbcaccbcaaaacccbaacaaccc3cbccbbbcaacbbcbabEaabaacccccbccbcbabacabbbcbbcbacccbabcabaccaabaaabbabcaababaabacbccbbccbaccccaccb",
		"bcbbacbcaccbcaaaacccbaacaaccc3cbccbbbcaacbbcbabEaabaacccccbccbcbabacabbbcbbcbacccbabcabaccaabaaabbabcaababaabacbccbbccbaccccaccb");

	return success;
}

/* ======================================================================= */
/* Manpage                                                                 */
/* ======================================================================= */

#define __VERSION_STR__(X, Y, Z) #X "." #Y "." #Z
#define _VERSION_STR_(X, Y, Z) __VERSION_STR__(X, Y, Z)
#define VERSION_STR _VERSION_STR_(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH)

static void print_manpage(const HANDLE std_err)
{
	print_message(std_err, "Replace v" VERSION_STR " [" __DATE__ "], by LoRd_MuldeR <MuldeR2@GMX.de>\n\n");
	print_message(std_err, "Replaces any occurrence of '<needle>' in '<input_file>' with '<replacement>'.\n");
	print_message(std_err, "The modified contents are then written to '<output_file>'.\n\n");
	print_message(std_err, "Usage:\n");
	print_message(std_err, "  replace.exe [options] <needle> <replacement> [<input_file>] [<output_file>]\n\n");
	print_message(std_err, "Options:\n");
	print_message(std_err, "  -i  Perform case-insensitive matching for the characters 'A' to 'Z'\n");
	print_message(std_err, "  -s  Single replacement; replace only the *first* occurrence instead of all\n");
	print_message(std_err, "  -a  Process input using ANSI codepage (CP-1252) instead of UTF-8\n");
	print_message(std_err, "  -b  Binary mode; parameters '<needle>' and '<replacement>' are Hex strings\n");
	print_message(std_err, "  -v  Enable verbose mode; prints additional diagnostic information\n");
	print_message(std_err, "  -f  Force immediate flushing of output buffers (may degrade performance)\n");
	print_message(std_err, "  -t  Run self-test and exit\n");
	print_message(std_err, "  -h  Display this help text and exit\n\n");
	print_message(std_err, "Notes:\n");
	print_message(std_err, "  1. If *only* an '<input_file>' is specified, the file is modified in-place!\n");
	print_message(std_err, "  2. If file names are omitted, reads from STDIN and writes to STDOUT.\n");
	print_message(std_err, "  3. File name can be specified as \"-\" to read from STDIN or write to STDOUT.\n");
	print_message(std_err, "  4. The length of a Hex string must be *even*; with optional '0x' prefix.\n\n");
	print_message(std_err, "Examples:\n");
	print_message(std_err, "  replace.exe \"foobar\" \"quux\" \"input.txt\" \"output.txt\"\n");
	print_message(std_err, "  replace.exe \"foobar\" \"quux\" \"modified.txt\"\n");
	print_message(std_err, "  replace.exe -b 0xDEADBEEF 0xCAFEBABE \"input.bin\" \"output.bin\"\n");
	print_message(std_err, "  type \"from.txt\" | replace.exe \"foo\" \"bar\" > \"to.txt\"\n\n");
}

/* ======================================================================= */
/* Main                                                                    */
/* ======================================================================= */

static int _main(const int argc, const LPWSTR *const argv)
{
	int result = 1, param_offset = 1;
	const BYTE *needle = NULL, *replacement = NULL;
	options_t options;
	HANDLE std_in = GetStdHandle(STD_INPUT_HANDLE), std_out = GetStdHandle(STD_OUTPUT_HANDLE), std_err = GetStdHandle(STD_ERROR_HANDLE);
	HANDLE input = INVALID_HANDLE_VALUE, output = INVALID_HANDLE_VALUE;
	UINT previous_output_cp = 0U;
	LONG needle_len = 0L, replacement_len = 0L;
	const WCHAR *source_file = NULL, *output_file = NULL, *temp_path = NULL, *temp_file = NULL;

	/* -------------------------------------------------------- */
	/* Parse options                                            */
	/* -------------------------------------------------------- */

	if(!parse_options(std_err, argc, argv, &param_offset, &options))
	{
		goto cleanup;
	}

	if(options.show_help)
	{
		print_manpage(std_err);
		result = 0;
		goto cleanup;
	}

	/* -------------------------------------------------------- */
	/* Parameter validation                                     */
	/* -------------------------------------------------------- */

	if(options.binary_mode && (options.ansi_cp || options.case_insensitive))
	{
		print_message(std_err, "Error: Options '-i' and '-a' are incompatible with '-b' option!\n");
		goto cleanup;
	}

	if((!options.self_test) && (argc - param_offset < 2U))
	{
		print_message(std_err, "Error: Required parameter is missing. Type \"replace -h\" for details!\n");
		goto cleanup;
	}

	if((!options.self_test) && (!argv[param_offset][0U]))
	{
		print_message(std_err, "Error: Search string (needle) must not be empty!\n");
		goto cleanup;
	}

	if((!options.self_test) && (lstrlenW(argv[param_offset]) > MAXWORD))
	{
		print_message(std_err, "Error: Search string (needle) must not exceed 65535 characters!\n");
		goto cleanup;
	}

	if((!options.self_test) && (lstrlenW(argv[param_offset + 1L]) > MAXWORD))
	{
		print_message(std_err, "Error: Replacement string must not exceed 65535 characters!\n");
		goto cleanup;
	}

	if((argc - param_offset > 2) && (!argv[param_offset + 2L][0U]))
	{
		print_message(std_err, "Error: If input file is specified, it must not be an empty string!\n");
		goto cleanup;
	}

	if((argc - param_offset > 3) && (!argv[param_offset + 3L][0U]))
	{
		print_message(std_err, "Error: If output file is specified, it must not be an empty string!\n");
		goto cleanup;
	}

	/* -------------------------------------------------------- */
	/* Self-test mode */
	/* -------------------------------------------------------- */

	if(options.self_test)
	{
		print_message(std_err, "Begin self-test...\n");
		if(self_test(std_err))
		{
			print_message(std_err, "Self-test completed.\n");
			result = 0;
		}
		else
		{
			print_message(std_err, "Error: Self-test failed!\n");
		}
		goto cleanup;
	}

	/* -------------------------------------------------------- */
	/* Initialize search parameters and file names              */
	/* -------------------------------------------------------- */

	needle = options.binary_mode ? decode_hex_string(argv[param_offset], &needle_len) : utf16_to_bytes(argv[param_offset], &needle_len, options.ansi_cp ? CP_1252 : CP_UTF8);
	if(!needle)
	{
		print_message(std_err, "Error: Failed to decode 'needle' string!\n");
		goto cleanup;
	}

	replacement = options.binary_mode ? decode_hex_string(argv[param_offset + 1U], &replacement_len) : utf16_to_bytes(argv[param_offset + 1U], &replacement_len, options.ansi_cp ? CP_1252 : CP_UTF8);
	if(!replacement)
	{
		print_message(std_err, "Error: Failed to decode 'replacement' string!\n");
		goto cleanup;
	}

	source_file = (argc - param_offset > 2U) ? argv[param_offset + 2U] : NULL;
	output_file = (argc - param_offset > 3U) ? argv[param_offset + 3U] : NULL;

	if(NOT_EMPTY(source_file) && NOT_EMPTY(output_file) && (lstrcmpiW(source_file, L"-") != 0) && (lstrcmpiW(output_file, L"-") != 0))
	{
		if(lstrcmpiW(source_file, output_file) == 0)
		{
			print_message(std_err, "Error: Input and output file must not be the same!\n");
			goto cleanup;
		}
	}

	/* -------------------------------------------------------- */
	/* Open input and output files                              */
	/* -------------------------------------------------------- */

	if(NOT_EMPTY(source_file) && (lstrcmpiW(source_file, L"-") != 0))
	{
		input = CreateFileW(source_file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	}
	else
	{
		if(options.verbose)
		{
			print_message(std_err, "Reading input from STDIN stream.\n");
		}
		input = std_in;
	}

	if(input == INVALID_HANDLE_VALUE)
	{
		print_message(std_err, "Error: Failed to open input file for reading!\n");
		goto cleanup;
	}

	if(EMPTY(output_file) && NOT_EMPTY(source_file) && (lstrcmpiW(source_file, L"-") != 0))
	{
		if(options.verbose)
		{
			print_message(std_err, "Using in-place processing mode this time.\n");
		}
		if(get_readonly_attribute(input))
		{
			print_message(std_err, "Error: The write-protected file cannot be modified in-place!\n");
			goto cleanup;
		}
		temp_file = generate_temp_file(temp_path = get_directory_part(source_file));
		if(EMPTY(temp_file))
		{
			print_message(std_err, "Error: Failed to generate file name for temporary data!\n");
			goto cleanup;
		}
	}

	if(NOT_EMPTY(temp_file) || (NOT_EMPTY(output_file) && (lstrcmpiW(output_file, L"-") != 0)))
	{
		output = CreateFileW(NOT_EMPTY(temp_file) ? temp_file : output_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	}
	else
	{
		if(options.verbose)
		{
			print_message(std_err, "Writing output to STDOUT stream.\n");
		}
		output = std_out;
	}

	if(output == INVALID_HANDLE_VALUE)
	{
		print_message(std_err, "Error: Failed to open output file for writing!\n");
		goto cleanup;
	}

	/* -------------------------------------------------------- */
	/* Search & replace!                                        */
	/* -------------------------------------------------------- */

	if(GetFileType(output) == FILE_TYPE_CHAR)
	{
		previous_output_cp = GetConsoleOutputCP();
		SetConsoleOutputCP(options.ansi_cp ? CP_1252 : CP_UTF8);
	}

	if(!search_and_replace(input, output, std_err, needle, needle_len, replacement, replacement_len, &options))
	{
		print_message(std_err, "Error: An I/O error was encountered. Output probably is incomplete!\n");
		goto cleanup;
	}

	/* -------------------------------------------------------- */
	/* Finishing touch                                          */
	/* -------------------------------------------------------- */

	if(input != std_in)
	{
		CloseHandle(input);
		input = INVALID_HANDLE_VALUE;
	}

	if(output != std_out)
	{
		CloseHandle(output);
		output = INVALID_HANDLE_VALUE;
	}

	if(NOT_EMPTY(temp_file))
	{
		if(!move_file(temp_file, source_file))
		{
			print_message(std_err, "Error: Failed to replace the original file with modified one!\n");
			goto cleanup;
		}
	}

	result = 0;

	/* -------------------------------------------------------- */
	/* Final clean-up                                           */
	/* -------------------------------------------------------- */

cleanup:

	if((input != INVALID_HANDLE_VALUE) && (input != std_in))
	{
		CloseHandle(input);
	}

	if((output != INVALID_HANDLE_VALUE) && (output != std_out))
	{
		CloseHandle(output);
	}

	if(NOT_EMPTY(temp_file) && file_exists(temp_file))
	{
		delete_file(temp_file);
	}

	if(needle)
	{
		LocalFree((HLOCAL)needle);
	}

	if(replacement)
	{
		LocalFree((HLOCAL)replacement);
	}

	if(temp_file)
	{
		LocalFree((HLOCAL)temp_file);
	}

	if(temp_path)
	{
		LocalFree((HLOCAL)temp_path);
	}

	if(previous_output_cp)
	{
		SetConsoleOutputCP(previous_output_cp);
	}

	return result;
}

/* ======================================================================= */
/* Entry point                                                             */
/* ======================================================================= */

void startup(void)
{
	int argc;
	UINT result = (UINT)(-1);
	LPWSTR *argv;

	SetErrorMode(SetErrorMode(0x3) | 0x3);
	if(argv = CommandLineToArgvW(GetCommandLineW(), &argc))
	{
		result = _main(argc, argv);
		LocalFree(argv);
	}

	ExitProcess(result);
}
