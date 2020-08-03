/******************************************************************************/
/* Replace, by LoRd_MuldeR <MuldeR2@GMX.de>                                   */
/* This work has been released under the CC0 1.0 Universal license!           */
/*                                                                            */
/* This program implements a variant of the "KMP" string-searching algorithm. */
/* See here for information:                                                  */
/* https://en.wikipedia.org/wiki/Knuth%E2%80%93Morris%E2%80%93Pratt_algorithm */
/******************************************************************************/

#include "libreplace/replace.h"
#include "utils.h"
#include "selftest.h"

#include <ShellAPI.h> /*CommandLineToArgvW*/

/* ======================================================================= */
/* Manpage                                                                 */
/* ======================================================================= */

#define __VERSION_STR__(X, Y, Z) #X "." #Y "." #Z
#define _VERSION_STR_(X, Y, Z) __VERSION_STR__(X, Y, Z)
#define VERSION_STR _VERSION_STR_(LIBREPLACE_VERSION_MAJOR, LIBREPLACE_VERSION_MINOR, LIBREPLACE_VERSION_PATCH)

static void print_manpage(const HANDLE std_err)
{
	print_text(std_err, "Replace v" VERSION_STR " [" __DATE__ "], by LoRd_MuldeR <MuldeR2@GMX.de>\n\n");
	print_text(std_err, "Replaces any occurrence of '<needle>' in '<input_file>' with '<replacement>'.\n");
	print_text(std_err, "The modified contents are then written to '<output_file>'.\n\n");
	print_text(std_err, "Usage:\n");
	print_text(std_err, "  replace.exe [options] <needle> <replacement> [<input_file>] [<output_file>]\n\n");
	print_text(std_err, "Options:\n");
	print_text(std_err, "  -i  Perform case-insensitive matching for the characters 'A' to 'Z'\n");
	print_text(std_err, "  -s  Single replacement; replace only the *first* occurrence instead of all\n");
	print_text(std_err, "  -a  Process input using ANSI codepage (CP-1252) instead of UTF-8\n");
	print_text(std_err, "  -e  Enable interpretation of backslash escape sequences in all parameters\n");
	print_text(std_err, "  -f  Force immediate flushing of file buffers (may degrade performance)\n");
	print_text(std_err, "  -b  Binary mode; parameters '<needle>' and '<replacement>' are Hex strings\n");
	print_text(std_err, "  -y  Try to overwrite read-only files; i.e. clears the read-only flag\n");
	print_text(std_err, "  -v  Enable verbose mode; print additional diagnostic information to STDERR\n");
	print_text(std_err, "  -t  Run self-test and exit\n");
	print_text(std_err, "  -h  Display this help text and exit\n\n");
	print_text(std_err, "Notes:\n");
	print_text(std_err, "  1. If *only* an '<input_file>' is specified, the file is modified in-place!\n");
	print_text(std_err, "  2. If file names are omitted, reads from STDIN and writes to STDOUT.\n");
	print_text(std_err, "  3. File name can be specified as \"-\" to read from STDIN or write to STDOUT.\n");
	print_text(std_err, "  4. The length of a Hex string must be *even*; with optional '0x' prefix.\n\n");
	print_text(std_err, "Examples:\n");
	print_text(std_err, "  replace.exe \"foobar\" \"quux\" \"input.txt\" \"output.txt\"\n");
	print_text(std_err, "  replace.exe -e \"foo\\nbar\" \"qu\\tux\" \"input.txt\" \"output.txt\"\n");
	print_text(std_err, "  replace.exe \"foobar\" \"quux\" \"modified.txt\"\n");
	print_text(std_err, "  replace.exe -b 0xDEADBEEF 0xCAFEBABE \"input.bin\" \"output.bin\"\n");
	print_text(std_err, "  type \"from.txt\" | replace.exe \"foo\" \"bar\" > \"to.txt\"\n\n");
}

/* ======================================================================= */
/* Ctrl+C handler routine                                                  */
/* ======================================================================= */

BOOL WINAPI ctrl_handler_routine(const DWORD type)
{
	switch(type)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		g_abort_requested = TRUE;
		return TRUE;
	}
	return FALSE;
}

/* ======================================================================= */
/* Command-line Options                                                    */
/* ======================================================================= */

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
				case L'?':
				case L'h':
					options->show_help = TRUE;
					break;
				case L'a':
					options->ansi_cp = TRUE;
					break;
				case L'b':
					options->binary_mode = TRUE;
					break;
				case L'e':
					options->escpae_chars = TRUE;
					break;
				case L'f':
					options->flags.force_sync = TRUE;
					break;
				case L'i':
					options->flags.case_insensitive = TRUE;
					break;
				case L's':
					options->flags.replace_once = TRUE;
					break;
				case L't':
					options->self_test = TRUE;
					break;
				case L'v':
					options->flags.verbose = TRUE;
					break;
				case L'y':
					options->force_overwrite = TRUE;
					break;
				default:
					print_text(std_err, "Error: Invalid command-line option encountered!\n");
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
/* Main                                                                    */
/* ======================================================================= */

const CHAR *const ABORTED_MESSAGE = "Process was aborted.\n";

static int _main(const int argc, const LPWSTR *const argv)
{
	int result = 1, param_offset = 1;
	BYTE *needle = NULL, *replacement = NULL;
	options_t options;
	HANDLE std_in = GetStdHandle(STD_INPUT_HANDLE), std_out = GetStdHandle(STD_OUTPUT_HANDLE), std_err = GetStdHandle(STD_ERROR_HANDLE);
	HANDLE input = INVALID_HANDLE_VALUE, output = INVALID_HANDLE_VALUE;
	libreplace_logger_t logger;
	libreplace_io_t io_functions;
	file_input_t *file_input_context = NULL;
	file_output_t *file_output_context = NULL;
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

	if(options.binary_mode && (options.ansi_cp || options.escpae_chars || options.flags.case_insensitive))
	{
		print_text(std_err, "Error: Options '-a', '-e' and '-i' are incompatible with '-b' option!\n");
		goto cleanup;
	}

	if((!options.self_test) && (argc - param_offset < 2U))
	{
		print_text(std_err, "Error: Required parameter is missing. Type \"replace -h\" for details!\n");
		goto cleanup;
	}

	if((!options.self_test) && (!argv[param_offset][0U]))
	{
		print_text(std_err, "Error: Search string (needle) must not be empty!\n");
		goto cleanup;
	}

	if((!options.self_test) && (lstrlenW(argv[param_offset]) > MAXWORD))
	{
		print_text(std_err, "Error: Search string (needle) must not exceed 65535 characters!\n");
		goto cleanup;
	}

	if((!options.self_test) && (lstrlenW(argv[param_offset + 1L]) > MAXWORD))
	{
		print_text(std_err, "Error: Replacement string must not exceed 65535 characters!\n");
		goto cleanup;
	}

	if((argc - param_offset > 2) && (!argv[param_offset + 2L][0U]))
	{
		print_text(std_err, "Error: If input file is specified, it must not be an empty string!\n");
		goto cleanup;
	}

	if((argc - param_offset > 3) && (!argv[param_offset + 3L][0U]))
	{
		print_text(std_err, "Error: If output file is specified, it must not be an empty string!\n");
		goto cleanup;
	}

	/* -------------------------------------------------------- */
	/* Self-test mode */
	/* -------------------------------------------------------- */

	if(options.self_test)
	{
		print_text(std_err, "Running self-test...\n");
		if(!self_test(std_err))
		{
			print_text(std_err, g_abort_requested ? ABORTED_MESSAGE : "Error: Self-test failed!\n");
			goto cleanup;
		}
		print_text(std_err, "Self-test completed.\n");
		result = 0;
		goto cleanup;
	}

	/* -------------------------------------------------------- */
	/* Initialize search parameters and file names              */
	/* -------------------------------------------------------- */

	needle = options.binary_mode ? decode_hex_string(argv[param_offset], &needle_len) : utf16_to_bytes(argv[param_offset], &needle_len, SELECTED_CP);
	if(!needle)
	{
		print_text(std_err, "Error: Failed to decode 'needle' string!\n");
		goto cleanup;
	}

	replacement = options.binary_mode ? decode_hex_string(argv[param_offset + 1U], &replacement_len) : utf16_to_bytes(argv[param_offset + 1U], &replacement_len, SELECTED_CP);
	if(!replacement)
	{
		print_text(std_err, "Error: Failed to decode 'replacement' string!\n");
		goto cleanup;
	}

	if(options.escpae_chars)
	{
		if(!(expand_escape_chars(needle, &needle_len) && expand_escape_chars(replacement, &replacement_len)))
		{
			print_text(std_err, "Error: Parameter contains invalid escape sequence!\n");
			goto cleanup;
		}
	}

	source_file = (argc - param_offset > 2U) ? argv[param_offset + 2U] : NULL;
	output_file = (argc - param_offset > 3U) ? argv[param_offset + 3U] : NULL;

	if(NOT_EMPTY(source_file) && NOT_EMPTY(output_file) && (lstrcmpiW(source_file, L"-") != 0) && (lstrcmpiW(output_file, L"-") != 0))
	{
		if(lstrcmpiW(source_file, output_file) == 0)
		{
			print_text(std_err, "Error: Input and output file must not be the same!\n");
			goto cleanup;
		}
	}

	/* -------------------------------------------------------- */
	/* Open input and output files                              */
	/* -------------------------------------------------------- */

	if(NOT_EMPTY(source_file) && (lstrcmpiW(source_file, L"-") != 0))
	{
		input = open_file(source_file, FALSE);
	}
	else
	{
		if(options.flags.verbose)
		{
			print_text(std_err, "Reading input from STDIN stream.\n");
		}
		input = std_in;
	}

	if(input == INVALID_HANDLE_VALUE)
	{
		print_text(std_err, "Error: Failed to open input file for reading!\n");
		goto cleanup;
	}

	if(!(file_input_context = alloc_file_input(input)))
	{
		print_text(std_err, "Error: Failed to allocate file input context!\n");
		goto cleanup;
	}

	if(EMPTY(output_file) && NOT_EMPTY(source_file) && (lstrcmpiW(source_file, L"-") != 0))
	{
		if(options.flags.verbose)
		{
			print_text(std_err, "Using in-place file processing mode this time.\n");
		}
		if((!options.force_overwrite) && has_readonly_attribute(input))
		{
			print_text(std_err, "Error: Sorry, the write-protected file cannot be modified in-place!\n");
			goto cleanup;
		}
		temp_file = generate_temp_file(temp_path = get_directory_part(source_file), &output);
		if(EMPTY(temp_file))
		{
			print_text(std_err, g_abort_requested ? ABORTED_MESSAGE : "Error: Failed to create temporary file!\n");
			goto cleanup;
		}
	}

	if(EMPTY(temp_file))
	{
		if(NOT_EMPTY(output_file) && (lstrcmpiW(output_file, L"-") != 0))
		{
			if(options.force_overwrite)
			{
				clear_readonly_attribute(output_file);
			}
			output = open_file(output_file, TRUE);
		}
		else
		{
			if(options.flags.verbose)
			{
				print_text(std_err, "Writing output to STDOUT stream.\n");
			}
			output = std_out;
		}
	}

	if(output == INVALID_HANDLE_VALUE)
	{
		print_text(std_err, "Error: Failed to open output file for writing!\n");
		goto cleanup;
	}

	if(!(file_output_context = alloc_file_output(output)))
	{
		print_text(std_err, "Error: Failed to allocate file output context!\n");
		goto cleanup;
	}

	/* -------------------------------------------------------- */
	/* Set up terminal                                          */
	/* -------------------------------------------------------- */

	if(GetFileType(output) == FILE_TYPE_CHAR)
	{
		const UINT current_cp = GetConsoleOutputCP();
		if((current_cp != 0U) && (current_cp != SELECTED_CP))
		{
			if(SetConsoleOutputCP(SELECTED_CP))
			{
				previous_output_cp = current_cp;
			}
		}
	}

	/* -------------------------------------------------------- */
	/* Search & replace!                                        */
	/* -------------------------------------------------------- */

	init_logging_functions(&logger, print_text_ptr, (DWORD_PTR)std_err);
	init_io_functions(&io_functions, file_read_byte, file_write_byte, (DWORD_PTR)file_input_context, (DWORD_PTR)file_output_context);

	if(!libreplace_search_and_replace(&io_functions, &logger, needle, needle_len, replacement, replacement_len, &options.flags, &g_abort_requested))
	{
		print_text(std_err, g_abort_requested ? ABORTED_MESSAGE : "Error: Something went wrong. Output probably is incomplete!\n");
		goto cleanup;
	}

	if(g_abort_requested)
	{
		print_text(std_err, ABORTED_MESSAGE);
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
		if(options.force_overwrite)
		{
			clear_readonly_attribute(source_file);
		}
		if(!move_file(temp_file, source_file))
		{
			print_text(std_err, "Error: Failed to replace the original file with modified one!\n");
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

	if(file_input_context)
	{
		LocalFree((HLOCAL)file_input_context);
	}

	if(file_output_context)
	{
		LocalFree((HLOCAL)file_output_context);
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
	SetConsoleCtrlHandler(ctrl_handler_routine, TRUE);

	if(argv = CommandLineToArgvW(GetCommandLineW(), &argc))
	{
		result = _main(argc, argv);
		LocalFree(argv);
	}

	ExitProcess(result);
}
