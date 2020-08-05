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

#include "libreplace/replace.h"

/* CLI options*/
typedef struct options_t
{ 
	BOOL show_help;
	BOOL ansi_cp;
	BOOL escpae_chars;
	BOOL binary_mode;
	BOOL force_overwrite;
	BOOL self_test;
	libreplace_flags_t flags;
}
options_t;

/* Abort flag */
volatile BOOL g_abort_requested = FALSE;

/* ======================================================================= */
/* String Routines                                                         */
/* ======================================================================= */

#define CP_1252 1252
#define SELECTED_CP ((UINT)(options.ansi_cp ? CP_1252 : CP_UTF8))

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

static BOOL expand_escape_chars(BYTE *const string, LONG *const len)
{
	LONG pos_in, pos_out;
	BOOL flag = FALSE;
	for(pos_in = pos_out = 0U; pos_in < *len; ++pos_in)
	{
		if(flag)
		{
			flag = FALSE;
			switch(string[pos_in])
			{
			case '0':
				string[pos_out++] = '\0';
				continue;
			case 'a':
				string[pos_out++] = '\a';
				continue;
			case 'b':
				string[pos_out++] = '\b';
				continue;
			case 't':
				string[pos_out++] = '\t';
				continue;
			case 'n':
				string[pos_out++] = '\n';
				continue;
			case 'v':
				string[pos_out++] = '\v';
				continue;
			case 'f':
				string[pos_out++] = '\f';
				continue;
			case 'r':
				string[pos_out++] = '\r';
				continue;
			case '\\':
				string[pos_out++] = '\\';
				continue;
			default:
				return FALSE;
			}
		}
		if(!(flag = (string[pos_in] == '\\')))
		{
			if(pos_in != pos_out)
			{
				string[pos_out] = string[pos_in];
			}
			++pos_out;
		}
	}
	if(!flag)
	{
		if(*len > pos_out)
		{
			string[*len = pos_out] = '\0';
		}
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/* ======================================================================= */
/* Random Numbers                                                          */
/* ======================================================================= */

typedef struct random_t
{
	DWORD a, b, c, d;
	DWORD counter;
}
random_t;

static void random_seed(random_t *const state)
{
	LARGE_INTEGER counter;
	FILETIME time;
	state->a = 65599U * GetCurrentThreadId() + GetCurrentProcessId();
	do
	{
		GetSystemTimeAsFileTime(&time);
		QueryPerformanceCounter(&counter);
		state->b = GetTickCount();
		state->c = 65599U * time.dwHighDateTime + time.dwLowDateTime;
		state->d = 65599U * counter.HighPart + counter.LowPart;
	}
	while((!state->a) && (!state->b) && (!state->c) && (!state->d));
	state->counter = 0U;
}

static __inline DWORD random_next(random_t *const state)
{
	DWORD t = state->d;
	const DWORD s = state->a;
	state->d = state->c;
	state->c = state->b;
	state->b = s;
	t ^= t >> 2;
	t ^= t << 1;
	t ^= s ^ (s << 4);
	state->a = t;
	return t + (state->counter += 362437U);
}

/* ======================================================================= */
/* File System Routines                                                    */
/* ======================================================================= */

static BOOL file_exists(const WCHAR *const path)
{
	const DWORD attribs = GetFileAttributesW(path);
	return (attribs != INVALID_FILE_ATTRIBUTES) && (!(attribs & FILE_ATTRIBUTE_DIRECTORY));
}

static BOOL has_readonly_attribute(const HANDLE handle)
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

static BOOL clear_readonly_attribute(const WCHAR *const path)
{
	DWORD attribs = GetFileAttributesW(path);
	if(attribs != INVALID_FILE_ATTRIBUTES)
	{
		if(attribs & FILE_ATTRIBUTE_READONLY)
		{
			attribs &= (~FILE_ATTRIBUTE_READONLY);
			return SetFileAttributesW(path, attribs ? attribs : FILE_ATTRIBUTE_NORMAL);
		}
		else
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

static const HANDLE open_file(const WCHAR *const file_name, const BOOL write_mode)
{
	HANDLE handle = INVALID_HANDLE_VALUE;
	DWORD retry;
	for(retry = 0U; retry < 32U; ++retry)
	{
		if(retry > 0U)
		{
			Sleep(retry); /*delay before retry*/
		}
		if((handle = CreateFileW(file_name, write_mode ? GENERIC_WRITE : GENERIC_READ, write_mode ? 0U: FILE_SHARE_READ, NULL, write_mode ? CREATE_ALWAYS : OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
		{
			const DWORD error = GetLastError();
			if(((!write_mode) && (error == ERROR_FILE_NOT_FOUND)) || (error == ERROR_PATH_NOT_FOUND) || (error == ERROR_INVALID_NAME))
			{
				break;
			}
		}
		else
		{
			return handle;
		}
	}
	return INVALID_HANDLE_VALUE;
}

static const WCHAR *generate_temp_file(const WCHAR *const directory, HANDLE *const handle)
{
	static const WCHAR *const RAND_TEMPLATE = L"%s\\~%07X.tmp";
	static const DWORD RAND_MASK = 0xFFFFFFF;
	const WCHAR *const prefix = NOT_EMPTY(directory) ? directory : L".";

	WCHAR *const temp = (WCHAR*) LocalAlloc(LPTR, sizeof(WCHAR) * (lstrlenW(prefix) + 14U));
	if(temp)
	{
		DWORD round, fail_count = 0U;
		random_t random_state;
		random_seed(&random_state);
		for(round = 0U; (round < 16777213U) && (!g_abort_requested); ++round)
		{
			wsprintfW(temp, RAND_TEMPLATE, prefix, random_next(&random_state) & RAND_MASK);
			if((*handle = CreateFileW(temp, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, 0, NULL)) == INVALID_HANDLE_VALUE)
			{
				if(GetLastError() != ERROR_FILE_EXISTS)
				{
					if(++fail_count >= 128U)
					{
						break;
					}
				}
			}
			else
			{
				return temp;
			}
		}
		LocalFree(temp);
	}

	return NULL;
}

static const BOOL move_file(const WCHAR *const file_src, const WCHAR *const file_dst)
{
	DWORD retry;
	for(retry = 0U; retry < 128U; ++retry)
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
	if(input != LIBREPLACE_FLUSH)
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
	BOOL pipe;
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
		ctx->pipe = (GetFileType(ctx->handle_in) == FILE_TYPE_PIPE);
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
		ctx->pos = ctx->avail = 0U;
		for(;;)
		{
			if(!ReadFile(ctx->handle_in, ctx->buffer, IO_BUFF_SIZE, &ctx->avail, NULL))
			{
				const DWORD error_code = GetLastError();
				if(ctx->pipe && (error_code == ERROR_NO_DATA))
				{
					Sleep(0U); /*wait before retry*/
					continue;
				}
				else
				{
					if((error_code != ERROR_HANDLE_EOF) && (error_code != ERROR_BROKEN_PIPE))
					{
						*error_flag = TRUE;
					}
					return FALSE;
				}
			}
			if(ctx->avail > 0U)
			{
				break;
			}
			if(!ctx->pipe)
			{
				return FALSE;
			}
		}
	}
	*output = ctx->buffer[ctx->pos++];
	return TRUE;
}

static __inline BOOL file_write_byte(const WORD input, const DWORD_PTR output, const BOOL sync)
{
	file_output_t *const ctx = (file_output_t*) output;
	if(input != LIBREPLACE_FLUSH)
	{
		ctx->buffer[ctx->pos++] = input & 0xFF;
	}
	if((ctx->pos >= IO_BUFF_SIZE) || (input == LIBREPLACE_FLUSH))
	{
		BOOL fail_flag = FALSE;
		DWORD bytes_written = 0U, offset = 0U;
		while(offset < ctx->pos)
		{
			if(!WriteFile(ctx->handle_out, ctx->buffer + offset, ctx->pos - offset, &bytes_written, NULL))
			{
				return FALSE;
			}
			if(bytes_written < 1U)
			{
				if(!fail_flag)
				{
					fail_flag = TRUE;
					continue;
				}
				else
				{
					return FALSE;
				}
			}
			offset += bytes_written;
			fail_flag = FALSE;
		}
		if(sync)
		{
			FlushFileBuffers(ctx->handle_out);
		}
		ctx->pos = 0U;
	}
	return TRUE;
}

/* ======================================================================= */
/* Logging                                                                 */
/* ======================================================================= */

static __inline BOOL print_text(const HANDLE output, const CHAR *const text)
{
	DWORD bytes_written;
	if(output != INVALID_HANDLE_VALUE)
	{
		return WriteFile(output, text, lstrlenA(text), &bytes_written, NULL);
	}
	return TRUE;
}

static __inline BOOL print_text_ptr(const DWORD_PTR context, const CHAR *const text)
{
	return print_text((HANDLE)context, text);
}

/* ======================================================================= */
/* Miscellaneous                                                           */
/* ======================================================================= */

static __inline void init_logging_functions(libreplace_logger_t *const logger, const libreplace_logging_func_t logging_func, const DWORD_PTR context)
{
	SecureZeroMemory(logger, sizeof(libreplace_logger_t));
	logger->logging_func = logging_func;
	logger->context = context;
}

static __inline void init_io_functions(libreplace_io_t *const io_functions, const libreplace_rd_func_t rd_func, const libreplace_wr_func_t wr_func, const DWORD_PTR context_rd, const DWORD_PTR context_wr)
{
	SecureZeroMemory(io_functions, sizeof(libreplace_io_t));
	io_functions->func_rd = rd_func;
	io_functions->func_wr = wr_func;
	io_functions->context_rd = context_rd;
	io_functions->context_wr = context_wr;
}

#endif /*INC_UTILS_H*/
