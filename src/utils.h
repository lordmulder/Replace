/******************************************************************************/
/* Replace, by LoRd_MuldeR <MuldeR2@GMX.de>                                   */
/* This work has been released under the CC0 1.0 Universal license!           */
/*                                                                            */
/* This program implements a variant of the "KMP" string-searching algorithm. */
/* See here for information:                                                  */
/* https://en.wikipedia.org/wiki/Knuth%E2%80%93Morris%E2%80%93Pratt_algorithm */
/******************************************************************************/

#ifndef INC_UTILS_H
#define INC_UTILS_H

#define WIN32_LEAN_AND_MEAN 1

#include <Windows.h>
#include <ShellAPI.h>

#ifdef _DEBUG
#define HAVE_TRACE 1
#define TRACE(OUT, FMT, ...) print_message_fmt((OUT), "[DEBUG] " FMT, __VA_ARGS__)
#else
#define HAVE_TRACE 0
#define TRACE(OUT, FMT, ...) __noop()
#endif

/* CLI options*/
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

/* Abort flag */
static volatile BOOL g_abort_requested = FALSE;
static volatile BOOL g_process_aborted = FALSE;

/* Flush flag */
#define IO_FLUSH ((WORD)-1)

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
#define SELECTED_CP ((UINT)(options.ansi_cp ? CP_1252 : CP_UTF8))

#define NOT_EMPTY(STR) ((STR) && ((STR)[0U]))
#define EMPTY(STR) (!NOT_EMPTY(STR))

#define TO_UPPER(C) ((((C) >= 0x61) && ((C) <= 0x7A)) ? ((C) - 0x20) : (C))

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
		
		return (31U * ((31U * GetCurrentProcessId()) + count.HighPart)) + count.LowPart;
	}
	else
	{
		return (31U * GetCurrentProcessId()) + GetTickCount();
	}
}

static __inline DWORD random_next(DWORD *const seed)
{
	const DWORD rand = *seed;
	*seed = (*seed) * 134775813U + 1U;
	return rand;
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
		DWORD round, iter, seed;
		for(round = 0U; round < 4099U; ++round)
		{
			seed = random_seed();
			for(iter = 0U; iter < 4099U; ++iter)
			{
				wsprintfW(temp, RAND_TEMPLATE, prefix, random_next(&seed) & RAND_MASK);
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
/* Memory I/O Routines                                                     */
/* ======================================================================= */

#pragma warning(push)
#pragma warning(disable: 4100)

typedef struct memory_input_t
{ 
	const BYTE *data_in;
	DWORD len;
	DWORD pos;
}
memory_input_t;

typedef struct memory_output_t
{ 
	DWORD capacity;
	DWORD pos;
	DWORD flushed;
	BYTE buffer[];
}
memory_output_t;

static void init_memory_input(memory_input_t *const input, const BYTE *const data, const DWORD data_len)
{
	input->data_in = data;
	input->len = data_len;
	input->pos = 0U;
}

static memory_output_t *alloc_memory_output(const DWORD capacity)
{
	memory_output_t *const output = (memory_output_t*) LocalAlloc(LPTR, sizeof(memory_output_t) + (sizeof(BYTE) * capacity));
	if(output)
	{
		output->capacity = capacity;
		output->pos = output->flushed = 0U;
		return output;
	}
	return NULL;
}

static __inline BOOL memory_read_byte(BYTE *const output, const DWORD_PTR input, BOOL *const error_flag)
{
	memory_input_t *const ctx = (memory_input_t*) input;
	if(ctx->pos < ctx->len)
	{
		*output = ctx->data_in[ctx->pos++];
		return TRUE;
	}
	return FALSE;
}

static __inline BOOL memory_write_byte(const WORD input, const DWORD_PTR output, const BOOL sync)
{
	memory_output_t *const ctx = (memory_output_t*) output;
	if(input != IO_FLUSH)
	{
		if(ctx->pos < ctx->capacity)
		{
			ctx->buffer[ctx->pos++] = input & 0xFF;
			return TRUE;
		}
		return FALSE;
	}
	else
	{
		ctx->flushed = ctx->pos;
		return TRUE;
	}
}

#pragma warning(pop)

/* ======================================================================= */
/* File I/O Routines                                                       */
/* ======================================================================= */

#define IO_BUFF_SIZE 4096U

typedef struct file_input_t
{ 
	HANDLE handle_in;
	DWORD avail;
	DWORD pos;
	BYTE buffer[IO_BUFF_SIZE];
}
file_input_t;

typedef struct file_output_t
{ 
	HANDLE handle_out;
	DWORD pos;
	BYTE buffer[IO_BUFF_SIZE];
}
file_output_t;

static file_input_t *alloc_file_input(const HANDLE handle)
{
	file_input_t *const ctx = (file_input_t*) LocalAlloc(LPTR, sizeof(file_input_t));
	if(ctx)
	{
		ctx->handle_in = handle;
		ctx->avail = ctx->pos = 0U;
	}
	return ctx;
}

static file_output_t *alloc_file_output(const HANDLE handle)
{
	file_output_t *const ctx = (file_output_t*) LocalAlloc(LPTR, sizeof(file_output_t));
	if(ctx)
	{
		ctx->handle_out = handle;
		ctx->pos = 0U;
	}
	return ctx;
}

static __inline BOOL file_read_byte(BYTE *const output, const DWORD_PTR input, BOOL *const error_flag)
{
	file_input_t *const ctx = (file_input_t*) input;
	if(ctx->pos >= ctx->avail)
	{
		ctx->pos = 0U;
		if(!ReadFile(ctx->handle_in, ctx->buffer, IO_BUFF_SIZE, &ctx->avail, NULL))
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

static __inline BOOL file_write_byte(const WORD input, const DWORD_PTR output, const BOOL sync)
{
	file_output_t *const ctx = (file_output_t*) output;
	if(input != IO_FLUSH)
	{
		ctx->buffer[ctx->pos++] = input & 0xFF;
		if(ctx->pos >= IO_BUFF_SIZE)
		{
			DWORD bytes_written;
			if(!WriteFile(ctx->handle_out, ctx->buffer, IO_BUFF_SIZE, &bytes_written, NULL))
			{
				return FALSE;
			}
			if(bytes_written < IO_BUFF_SIZE)
			{
				return FALSE;
			}
			if(sync)
			{
				FlushFileBuffers(ctx->handle_out);
			}
			ctx->pos = 0U;
		}
	}
	else
	{
		if(ctx->pos > 0)
		{
			DWORD bytes_written;
			if(!WriteFile(ctx->handle_out, ctx->buffer, ctx->pos, &bytes_written, NULL))
			{
				return FALSE;
			}
			if(bytes_written < ctx->pos)
			{
				return FALSE;
			}
			if(sync)
			{
				FlushFileBuffers(ctx->handle_out);
			}
			ctx->pos = 0U;
		}
	}
	return TRUE;
}

/* ======================================================================= */
/* Logging                                                                 */
/* ======================================================================= */

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

#endif /*INC_UTILS_H*/