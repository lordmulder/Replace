/************************************************************************* */
/* Replace, by LoRd_MuldeR <MuldeR2@GMX.de>                                */
/* This work has been released under the CC0 1.0 Universal license!        */
/************************************************************************* */

#define WIN32_LEAN_AND_MEAN 1

#include <Windows.h>
#include <ShellAPI.h>
#include <wincrypt.h>

/* ======================================================================= */
/* Utilities                                                               */
/* ======================================================================= */

static __inline void increment(DWORD *const value, const DWORD limit)
{
	if((*value += 1U) >= limit)
	{
		*value = 0U;
	}
}

static __inline DWORD add(const DWORD a, const DWORD b, const DWORD limit)
{
	DWORD result = a + b;
	while(result >= limit)
	{
		result -= limit;
	}
	return result;
}

/* ======================================================================= */
/* String Routines                                                         */
/* ======================================================================= */

#define NOT_EMPTY(STR) ((STR) && ((STR)[0U]))

static BYTE *utf16_to_bytes(const WCHAR *const input, const UINT code_page)
{
	BYTE *buffer;
	DWORD buffer_size = 0U, result = 0U;

	buffer_size = WideCharToMultiByte(code_page, 0, input, -1, NULL, 0, NULL, NULL);

	buffer = (BYTE*) LocalAlloc(LPTR, sizeof(BYTE) * buffer_size);
	if(!buffer)
	{
		return NULL;
	}

	result = WideCharToMultiByte(code_page, 0, input, -1, (LPSTR)buffer, buffer_size, NULL, NULL);
	if((result > 0) && (result <= buffer_size))
	{
		return buffer;
	}

	LocalFree(buffer);
	return NULL;
}

/* ======================================================================= */
/* Random Numbers                                                          */
/* ======================================================================= */

typedef ULONG_PTR random_t;

static BOOL random_init(random_t *const random_ctx)
{
	HCRYPTPROV provider;
	if(CryptAcquireContextW(&provider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
	{
		*random_ctx = provider;
		return TRUE;
	}
	return FALSE;
}

static __inline BOOL random_next(DWORD *const random_out, const random_t random_ctx)
{
	DWORD temp;
	if(CryptGenRandom(random_ctx, sizeof(DWORD), (BYTE*)&temp))
	{
		*random_out = temp;
		return TRUE;
	}
	return FALSE;
}

static void random_exit(random_t *const random_ctx)
{
	CryptReleaseContext(*random_ctx, 0U);
	*random_ctx = 0U;
}

/* ======================================================================= */
/* File System Routines                                                    */
/* ======================================================================= */

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
		random_t random_ctx;
		DWORD retry;
		if(random_init(&random_ctx))
		{
			for(retry = 0U; retry < 9973U; ++retry)
			{
				DWORD rndval;
				if(random_next(&rndval, random_ctx))
				{
					wsprintfW(temp, RAND_TEMPLATE, prefix, rndval & RAND_MASK);
					if(GetFileAttributesW(temp) == INVALID_FILE_ATTRIBUTES)
					{
						random_exit(&random_ctx);
						return temp;
					}
				}
			}
			random_exit(&random_ctx);
		}
		for(retry = 0U; retry <= RAND_MASK; ++retry)
		{
			wsprintfW(temp, RAND_TEMPLATE, prefix, retry);
			if(GetFileAttributesW(temp) == INVALID_FILE_ATTRIBUTES)
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
	for(retry = 0U; retry < 97; ++retry)
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

static __inline BOOL read_next_byte(BYTE *const output, const HANDLE input, input_context_t *const ctx)
{
	if(ctx->pos >= ctx->avail)
	{
		ctx->pos = 0U;
		if(!ReadFile(input, ctx->buffer, BUFF_SIZE, &ctx->avail, NULL))
		{
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

static __inline BOOL write_next_byte(const BYTE input, const HANDLE output, output_context_t *const ctx)
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
	}
	return TRUE;
}

static __inline BOOL write_byte_array(const BYTE *const input, const DWORD input_len, const HANDLE output, output_context_t *const ctx)
{
	DWORD input_pos;
	for(input_pos = 0U; input_pos < input_len; ++input_pos)
	{
		if(!write_next_byte(input[input_pos], output, ctx))
		{
			return FALSE;
		}
	}
	return TRUE;
}

static __inline BOOL flush_pending_bytes(const HANDLE output, output_context_t *const ctx)
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
	}
	return TRUE;
}

static __inline BOOL print_message(const HANDLE output, const BYTE *const text)
{
	DWORD bytes_written;
	return WriteFile(output, text, lstrlenA((LPCSTR)text), &bytes_written, NULL);
}

/* ======================================================================= */
/* Ring buffer                                                             */
/* ======================================================================= */

#define RB_INITIAL_POS(BUFF) (((BUFF)->valid >= (BUFF)->capacity) ? (BUFF)->pos : 0U)

typedef struct ringbuffer_t
{ 
	DWORD capacity;
	DWORD valid;
	DWORD flushed;
	DWORD pos;
	BYTE buffer[];
}
ringbuffer_t;

static ringbuffer_t *ringbuffer_alloc(const DWORD size)
{
	ringbuffer_t *const ringbuffer = (ringbuffer_t*) LocalAlloc(LPTR, sizeof(ringbuffer_t) + (sizeof(BYTE) * size));
	if(ringbuffer)
	{
		ringbuffer->capacity = size;
		ringbuffer->pos = ringbuffer->valid = ringbuffer->flushed = 0U;
		return ringbuffer;
	}
	return NULL;
}

static __inline void ringbuffer_reset(ringbuffer_t *const ringbuffer)
{
	ringbuffer->pos = ringbuffer->valid = ringbuffer->flushed = 0U;
}

static __inline BOOL ringbuffer_put(const BYTE in, BYTE *const out, ringbuffer_t *const ringbuffer)
{
	*out = ringbuffer->buffer[ringbuffer->pos];
	ringbuffer->buffer[ringbuffer->pos] = in;
	increment(&ringbuffer->pos, ringbuffer->capacity);
	if(ringbuffer->valid < ringbuffer->capacity)
	{
		++ringbuffer->valid;
		return FALSE;
	}
	return TRUE;
}

static __inline BOOL ringbuffer_compare(const BYTE *const needle, const DWORD needle_len, ringbuffer_t *const ringbuffer)
{
	DWORD idx_needle, idx_buffer;
	if(ringbuffer->valid != needle_len)
	{
		return FALSE;
	}
	for(idx_needle = 0U, idx_buffer = RB_INITIAL_POS(ringbuffer); idx_needle < needle_len; ++idx_needle, increment(&idx_buffer, ringbuffer->capacity))
	{
		if(ringbuffer->buffer[idx_buffer] != needle[idx_needle])
		{
			return FALSE;
		}
	}
	return TRUE;
}

static __inline BOOL ringbuffer_flush(BYTE *const out, ringbuffer_t *const ringbuffer)
{
	if(ringbuffer->flushed < ringbuffer->valid)
	{
		*out = ringbuffer->buffer[add(RB_INITIAL_POS(ringbuffer), ringbuffer->flushed++, ringbuffer->capacity)];
		return TRUE;
	}
	return FALSE;
}

/* ======================================================================= */
/* Search & Replace                                                        */
/* ======================================================================= */

static BOOL search_and_replace(const HANDLE input, const HANDLE output, const BYTE *const needle, const BYTE *const replacement)
{
	const DWORD needle_len = lstrlenA((LPCSTR)needle);
	const DWORD replacement_len = lstrlenA((LPCSTR)replacement);

	BOOL success = FALSE;
	input_context_t *input_ctx = NULL; output_context_t *output_ctx  = NULL;
	ringbuffer_t *ringbuffer;
	BYTE char_input, char_output;

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

	ringbuffer = ringbuffer_alloc(needle_len);
	if(!ringbuffer)
	{
		goto finished;
	}

	while(read_next_byte(&char_input, input, input_ctx))
	{
		if(ringbuffer_put(char_input, &char_output, ringbuffer))
		{
			if(!write_next_byte(char_output, output, output_ctx))
			{
				goto finished; 
			}
		}
		if(ringbuffer_compare(needle, needle_len, ringbuffer))
		{
			if(!write_byte_array(replacement, replacement_len, output, output_ctx))
			{
				goto finished; 
			}
			ringbuffer_reset(ringbuffer);
		}
	}

	while(ringbuffer_flush(&char_output, ringbuffer))
	{
		if(!write_next_byte(char_output, output, output_ctx))
		{
			goto finished; 
		}
	}

	success = flush_pending_bytes(output, output_ctx);

finished:

	if(input_ctx)
	{
		LocalFree(input_ctx);
	}

	if(output_ctx)
	{
		LocalFree(output_ctx);
	}

	if(ringbuffer)
	{
		LocalFree(ringbuffer);
	}

	return success;
}

/* ======================================================================= */
/* Main                                                                    */
/* ======================================================================= */

static int _main(const int argc, const LPWSTR *const argv)
{
	int result = 1;
	const BYTE *needle = NULL, *replacement = NULL;
	HANDLE std_in = GetStdHandle(STD_INPUT_HANDLE), std_out = GetStdHandle(STD_OUTPUT_HANDLE), std_err = GetStdHandle(STD_ERROR_HANDLE);
	UINT previous_output_cp = 0U;
	HANDLE input = INVALID_HANDLE_VALUE, output = INVALID_HANDLE_VALUE;
	const WCHAR *temp_path = NULL, *temp_file = NULL;

	if(argc < 3U)
	{
		print_message(std_err, "Replace [" __DATE__ "], by LoRd_MuldeR <MuldeR2@GMX.de>\n\n");
		print_message(std_err, "Replaces all occurences of '<needle>' in '<input_file>' with '<replacement>'.\n");
		print_message(std_err, "The modified contents are then written to '<output_file>'.\n\n");
		print_message(std_err, "Usage:\n");
		print_message(std_err, "   replace.exe <needle> <replacement> [<input_file>] [<output_file>]\n\n");
		print_message(std_err, "Please note:\n");
		print_message(std_err, "1. If *only* '<input_file>' is specified, the file will be modified in-place!\n");
		print_message(std_err, "2. If both file names are omitted, reads from STDIN and writes to STDOUT.\n");
		print_message(std_err, "3. Either file can be specified as \"-\" to read from STDIN or write to STDOUT.\n\n");
		goto cleanup;
	}

	if(lstrlenW(argv[1U]) < 1)
	{
		print_message(std_err, "Error: Search string (needle) must not be empty!\n");
		goto cleanup;
	}

	if((argc > 4) && (lstrcmpiW(argv[3U], argv[4U]) == 0))
	{
		print_message(std_err, "Error: Input and output file must not be the same!\n");
		goto cleanup;
	}

	needle = utf16_to_bytes(argv[1U], CP_UTF8);
	if(!needle)
	{
		goto cleanup;
	}

	replacement = utf16_to_bytes(argv[2U], CP_UTF8);
	if(!replacement)
	{
		goto cleanup;
	}

	if((argc > 3U) && (lstrcmpiW(argv[3U], L"-") != 0))
	{
		input = CreateFileW(argv[3U], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	}
	else
	{
		input = std_in;
	}

	if(input == INVALID_HANDLE_VALUE)
	{
		print_message(std_err, "Error: Failed to open input file for reading!\n");
		goto cleanup;
	}

	if((argc == 4U) && (lstrcmpiW(argv[3U], L"-") != 0) && (input != std_in))
	{
		if(get_readonly_attribute(input))
		{
			print_message(std_err, "Error: The read-only file cannot be modified in-place!\n");
			goto cleanup;
		}
		if(temp_path = get_directory_part(argv[3U]))
		{
			if(!(temp_file = generate_temp_file(temp_path)))
			{
				print_message(std_err, "Error: Failed to generate temp file name!\n");
				goto cleanup;
			}
		}
		else
		{
			print_message(std_err, "Error: Failed to determine temp directory!\n");
			goto cleanup;
		}
	}

	if(NOT_EMPTY(temp_file) || ((argc > 4U) && (lstrcmpiW(argv[4U], L"-") != 0)))
	{
		output = CreateFileW(NOT_EMPTY(temp_file) ? temp_file : argv[4U], GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	}
	else
	{
		output = std_out;
	}

	if(output == INVALID_HANDLE_VALUE)
	{
		print_message(std_err, "Error: Failed to open output file for writing!\n");
		goto cleanup;
	}

	if(GetFileType(output) == FILE_TYPE_CHAR)
	{
		previous_output_cp = GetConsoleOutputCP();
		if(previous_output_cp != CP_UTF8)
		{
			SetConsoleOutputCP(CP_UTF8);
		}
	}

	if(!search_and_replace(input, output, needle, replacement))
	{
		print_message(std_err, "Error: An I/O error was encountered. Output probably is incomplete!\n");
		goto cleanup;
	}

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
		if(!move_file(temp_file, argv[3U]))
		{
			print_message(std_err, "Error: Failed to replace the original file!\n");
			goto cleanup;
		}
	}

	result = 0;

cleanup:

	if((input != INVALID_HANDLE_VALUE) && (input != std_in))
	{
		CloseHandle(input);
	}

	if((output != INVALID_HANDLE_VALUE) && (output != std_out))
	{
		CloseHandle(output);
	}

	if(NOT_EMPTY(temp_file))
	{
		if(GetFileAttributesW(temp_file) != INVALID_FILE_ATTRIBUTES)
		{
			DeleteFileW(temp_file);
		}
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

	if(previous_output_cp && (previous_output_cp != CP_UTF8))
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
	UINT result = -1;
	LPWSTR *argv;

	SetErrorMode(SetErrorMode(0x3) | 0x3);

	argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if(!argv)
	{
		ExitProcess(result);
	}

	result = _main(argc, argv);
	LocalFree(argv);
	ExitProcess(result);
}
