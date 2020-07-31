/******************************************************************************/
/* Replace, by LoRd_MuldeR <MuldeR2@GMX.de>                                   */
/* This work has been released under the CC0 1.0 Universal license!           */
/*                                                                            */
/* This program implements a variant of the "KMP" string-searching algorithm. */
/* See here for information:                                                  */
/* https://en.wikipedia.org/wiki/Knuth%E2%80%93Morris%E2%80%93Pratt_algorithm */
/******************************************************************************/

#ifndef INC_SELFTEST_H
#define INC_SELFTEST_H

#include "replace.h"

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
	if(g_abort_requested || g_process_aborted) \
	{ \
		return FALSE; \
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

#endif /*INC_SELFTEST_H*/
