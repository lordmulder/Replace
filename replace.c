/************************************************************************* */
/* Replace, by LoRd_MuldeR <MuldeR2@GMX.de>                                */
/* This work has been released under the CC0 1.0 Universal license!        */
/************************************************************************* */

#define WIN32_LEAN_AND_MEAN 1

#include <Windows.h>
#include <ShellAPI.h>
#include <wincrypt.h>

#define VERSION_MAJOR 1
#define VERSION_MINOR 1
#define VERSION_PATCH 0

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

static __inline DWORD add_mod(const DWORD value_a, const DWORD value_b, const DWORD limit)
{
	DWORD result = value_a + value_b;
	while(result >= limit)
	{
		result -= limit;
	}
	return result;
}

/* ======================================================================= */
/* String Routines                                                         */
/* ======================================================================= */

#define CP_1252 1252
#define TO_UPPER(C) ((((C) >= 0x61) && ((C) <= 0x7A)) ? ((C) - 0x20) : (C))

#define NOT_EMPTY(STR) ((STR) && ((STR)[0U]))
#define EMPTY(STR) (!NOT_EMPTY(STR))

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

static __inline BOOL compare_char(const BYTE char_a, const BYTE char_b, const BOOL ignore_case)
{
	return ignore_case ? (TO_UPPER(char_a) == TO_UPPER(char_b)) : (char_a == char_b);
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
					if(!path_exists(temp))
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
			if(!path_exists(temp))
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
	*error_flag = FALSE;
	if(ctx->pos >= ctx->avail)
	{
		ctx->pos = 0U;
		if(!ReadFile(input, ctx->buffer, BUFF_SIZE, &ctx->avail, NULL))
		{
			const DWORD error_code = GetLastError();
			*error_flag = (error_code != ERROR_HANDLE_EOF) && (error_code != ERROR_BROKEN_PIPE);
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
		FlushFileBuffers(output);
	}
	return TRUE;
}

static __inline BOOL print_message(const HANDLE output, const CHAR *const text)
{
	DWORD bytes_written;
	return WriteFile(output, text, lstrlenA(text), &bytes_written, NULL);
}

static BOOL print_message_fmt(const HANDLE output, const CHAR *const format, ...)
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

static __inline BOOL ringbuffer_compare(const BYTE *const needle, const DWORD needle_len, ringbuffer_t *const ringbuffer, const BOOL ignore_case)
{
	DWORD idx_needle, idx_buffer;
	if(ringbuffer->valid != needle_len)
	{
		return FALSE;
	}
	for(idx_needle = 0U, idx_buffer = RB_INITIAL_POS(ringbuffer); idx_needle < needle_len; ++idx_needle, increment(&idx_buffer, ringbuffer->capacity))
	{
		if(!compare_char(ringbuffer->buffer[idx_buffer], needle[idx_needle], ignore_case))
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
		*out = ringbuffer->buffer[add_mod(RB_INITIAL_POS(ringbuffer), ringbuffer->flushed++, ringbuffer->capacity)];
		return TRUE;
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
	BOOL force_flush;
	BOOL verbose;
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
					options->force_flush = TRUE;
					break;
				case L'v':
					options->verbose = TRUE;
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

static const char *const WRITE_ERROR_MESSAGE = "Failed to write output data -> aborting!\n";
static const char *const READ_ERROR_MESSAGE  = "Warning: Read operation failed -> input may be incomplete!\n";

static BOOL search_and_replace(const HANDLE input, const HANDLE output, const HANDLE std_err, const BYTE *const needle, const BYTE *const replacement, const options_t *const options)
{

	const DWORD needle_len = lstrlenA((LPCSTR)needle);
	const DWORD replacement_len = lstrlenA((LPCSTR)replacement);

	BOOL success = FALSE, pending_input = FALSE, error_flag = FALSE;
	ringbuffer_t *ringbuffer = NULL;
	input_context_t *input_ctx = NULL; output_context_t *output_ctx  = NULL;
	BYTE char_input, char_output;
	ULARGE_INTEGER position = { 0U, 0U };
	DWORD replace_counter = 0U;

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

	while(pending_input = read_next_byte(&char_input, input, input_ctx, &error_flag))
	{
		if(ringbuffer_put(char_input, &char_output, ringbuffer))
		{
			if(!write_next_byte(char_output, output, output_ctx, options->force_flush))
			{
				if(options->verbose)
				{
					print_message(std_err, WRITE_ERROR_MESSAGE);
				}
				goto finished; 
			}
		}
		if(ringbuffer_compare(needle, needle_len, ringbuffer, options->case_insensitive))
		{
			++replace_counter;
			ringbuffer_reset(ringbuffer);
			if(options->verbose)
			{
				print_message_fmt(std_err, "Replaced occurence at: 0x%08lX%08lX\n", position.HighPart, position.LowPart);
			}
			if(!write_byte_array(replacement, replacement_len, output, output_ctx, options->force_flush))
			{
				if(options->verbose)
				{
					print_message(std_err, WRITE_ERROR_MESSAGE);
				}
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
		++position.QuadPart;
	}

	if(error_flag)
	{
		print_message(std_err, READ_ERROR_MESSAGE);
	}

	while(ringbuffer_flush(&char_output, ringbuffer))
	{
		if(!write_next_byte(char_output, output, output_ctx, options->force_flush))
		{
			if(options->verbose)
			{
				print_message(std_err, WRITE_ERROR_MESSAGE);
			}
			goto finished; 
		}
	}

	if(pending_input)
	{
		while(read_next_byte(&char_input, input, input_ctx, &error_flag))
		{
			if(!write_next_byte(char_input, output, output_ctx, options->force_flush))
			{
				if(options->verbose)
				{
					print_message(std_err, WRITE_ERROR_MESSAGE);
				}
				goto finished;
			}
		}
		if(error_flag)
		{
			print_message(std_err, READ_ERROR_MESSAGE);
		}
	}

	success = flush_pending_bytes(output, output_ctx);

	if(options->verbose)
	{
		print_message_fmt(std_err, "Total occurences replaced: %lu\n", replace_counter);
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

	if(ringbuffer)
	{
		LocalFree(ringbuffer);
	}

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
	print_message(std_err, "Replaces all occurences of '<needle>' in '<input_file>' with '<replacement>'.\n");
	print_message(std_err, "The modified contents are then written to '<output_file>'.\n\n");
	print_message(std_err, "Usage:\n");
	print_message(std_err, "  replace.exe [options] <needle> <replacement> [<input_file>] [<output_file>]\n\n");
	print_message(std_err, "Options:\n");
	print_message(std_err, "  -i  Perform case-insensitive matching for the characters 'A' to 'Z'\n");
	print_message(std_err, "  -s  Single replacement; replace only the *first* occurence instead of all\n");
	print_message(std_err, "  -a  Process input using ANSI codepage (CP-1252) instead of UTF-8\n");
	print_message(std_err, "  -f  Force immediate flushing of output buffers (may degrade performance)\n");
	print_message(std_err, "  -v  Enable verbose mode; prints additional diagnostic information\n");
	print_message(std_err, "  -h  Display this help and exit\n\n");
	print_message(std_err, "Notes:\n");
	print_message(std_err, "  1. If *only* an '<input_file>' is specified, the file is modified in-place!\n");
	print_message(std_err, "  2. If file names are omitted, reads from STDIN and writes to STDOUT.\n");
	print_message(std_err, "  3. File name can be specified as \"-\" to read from STDIN or write to STDOUT.\n\n");
	print_message(std_err, "Examples:\n");
	print_message(std_err, "  replace.exe \"foo\" \"bar\" \"input.txt\" \"output.txt\"\n");
	print_message(std_err, "  replace.exe \"foo\" \"bar\" \"modify-me.txt\"\n");
	print_message(std_err, "  type \"input.txt\" | replace.exe \"foo\" \"bar\" > \"output.txt\"\n\n");
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

	if(argc - param_offset < 2U)
	{
		print_message(std_err, "Error: Required parameter is missing. Type \"replace -h\" for details!\n");
		goto cleanup;
	}

	if(lstrlenW(argv[param_offset]) < 1)
	{
		print_message(std_err, "Error: Search string (needle) must not be empty!\n");
		goto cleanup;
	}

	if((argc - param_offset > 2) && lstrlenW(argv[param_offset + 2]) < 1)
	{
		print_message(std_err, "Error: If input file is specified, it must not be an empty string!\n");
		goto cleanup;
	}

	if((argc - param_offset > 3) && lstrlenW(argv[param_offset + 3]) < 1)
	{
		print_message(std_err, "Error: If output file is specified, it must not be an empty string!\n");
		goto cleanup;
	}

	/* -------------------------------------------------------- */
	/* Initialize search parameters and file names              */
	/* -------------------------------------------------------- */

	needle = utf16_to_bytes(argv[param_offset], options.ansi_cp ? CP_1252 : CP_UTF8);
	if(!needle)
	{
		goto cleanup;
	}

	replacement = utf16_to_bytes(argv[param_offset + 1], options.ansi_cp ? CP_1252 : CP_UTF8);
	if(!replacement)
	{
		goto cleanup;
	}

	source_file = (argc - param_offset > 2) ? argv[param_offset + 2] : NULL;
	output_file = (argc - param_offset > 3) ? argv[param_offset + 3] : NULL;

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

	if(!search_and_replace(input, output, std_err, needle, replacement, &options))
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
