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

static BOOL run_test(const HANDLE logging, const CHAR *const needle, const CHAR *const replacement, const CHAR *const haystack, const CHAR *const expected)
{
	BOOL success = FALSE;
	memory_input_t input_context;
	memory_output_t *output_context = NULL;
	io_functions_t io_functions;
	options_t options;
	const DWORD expected_len = lstrlenA(expected);

	init_memory_input(&input_context, (const BYTE*)haystack, lstrlenA(haystack));
	SecureZeroMemory(&options, sizeof(options_t));

	if(!(output_context = alloc_memory_output(expected_len + 2U)))
	{
		goto cleanup;
	}

	init_io_functions(&io_functions, memory_read_byte, memory_write_byte, (DWORD_PTR)&input_context, (DWORD_PTR)output_context);

	if(!search_and_replace(&io_functions, logging, (const BYTE*)needle, lstrlenA(needle), (const BYTE*)replacement, lstrlenA(replacement), &options))
	{
		goto cleanup;
	}
	
	if(output_context->flushed == expected_len)
	{
		success = (CompareStringA(LOCALE_INVARIANT, 0, (const CHAR*)output_context->buffer, output_context->flushed, expected, lstrlenA(expected)) == CSTR_EQUAL);
	}

cleanup:

	if(output_context)
	{
		LocalFree((HLOCAL)output_context);
	}

	return success;
}

static BOOL self_test(const HANDLE std_err)
{
	BOOL success = TRUE;

	RUN_TEST(0, "LTs3kx", "XJbf3A",
		"KJnsbsniWReHocwWghHKmtwue7zLXvT9Ai3twkgmHRahFxTV3EggbHptv7toJhdKWCyJ93vPmUqXVtwCuJvpvY9Avu4cojuRwknv7HCYpyNvzJWtdwvEEpsNNyq9JAay",
		"KJnsbsniWReHocwWghHKmtwue7zLXvT9Ai3twkgmHRahFxTV3EggbHptv7toJhdKWCyJ93vPmUqXVtwCuJvpvY9Avu4cojuRwknv7HCYpyNvzJWtdwvEEpsNNyq9JAay");

	RUN_TEST(1, "LTs3kx", "XJbf3B",
		"KJnsbsniWReHocwWghHKmtwue7zLXvT9Ai3twkgmHRahFxTV3EggbHpLTs3kxhdKWCyJ93vPmUqXVtwCuJvpvY9Avu4cojuRwknv7HCYpyNvzJWtdwvEEpsNNyq9JAay",
		"KJnsbsniWReHocwWghHKmtwue7zLXvT9Ai3twkgmHRahFxTV3EggbHpXJbf3BhdKWCyJ93vPmUqXVtwCuJvpvY9Avu4cojuRwknv7HCYpyNvzJWtdwvEEpsNNyq9JAay");

	RUN_TEST(2, "LTs3kx", "",
		"KJnsbsniWReHocwWghHKmtwue7zLXvT9Ai3twkgmHRahFxTV3EggbHpLTs3kxhdKWCyJ93vPmUqXVtwCuJvpvY9Avu4cojuRwknv7HCYpyNvzJWtdwvEEpsNNyq9JAay",
		"KJnsbsniWReHocwWghHKmtwue7zLXvT9Ai3twkgmHRahFxTV3EggbHphdKWCyJ93vPmUqXVtwCuJvpvY9Avu4cojuRwknv7HCYpyNvzJWtdwvEEpsNNyq9JAay");

	RUN_TEST(3, "LTs3kx", "H4n3zWoHKfbX",
		"KJnsbsniWReHocwWghHKmtwue7zLXvT9Ai3twkgmHRahFxTV3EggbHpLTs3kxhdKWCyJ93vPmUqXVtwCuJvpvY9Avu4cojuRwknv7HCYpyNvzJWtdwvEEpsNNyq9JAay",
		"KJnsbsniWReHocwWghHKmtwue7zLXvT9Ai3twkgmHRahFxTV3EggbHpH4n3zWoHKfbXhdKWCyJ93vPmUqXVtwCuJvpvY9Avu4cojuRwknv7HCYpyNvzJWtdwvEEpsNNyq9JAay");

	RUN_TEST(4, "ababaa", "YJbg3A",
		"aaabaaaabbbaaabbaaaabaaaaabbabbaaaa3aabbbabbaabbbbabbabbbbbbbaabaaaabbaaabbbaaabbbaaaaababaaaaabaaabababaabbabbbabaabaAabaaabbaa",
		"aaabaaaabbbaaabbaaaabaaaaabbabbaaaa3aabbbabbaabbbbabbabbbbbbbaabaaaabbaaabbbaaabbbaaaaYJbg3AaaabaaabYJbg3AbbabbbabaabaAabaaabbaa");

	RUN_TEST(5, "abcabd", "XJcf3A",
		"abbaccddbcccacbddabcabcdacdbcabcdcccbcdcadbdddcabbcadcdccbabaabacccccabcababcabddbcbbcaadccab4dbaddbdccbdcdbcXccbbbcabbaabdcadccd",
		"abbaccddbcccacbddabcabcdacdbcabcdcccbcdcadbdddcabbcadcdccbabaabacccccabcabXJcf3Adbcbbcaadccab4dbaddbdccbdcdbcXccbbbcabbaabdcadccd");
	
	RUN_TEST(6, "bcbbab", "XIbf3A",
		"cbcaccbcaaaacccbaacaaccc3cbccbbbcaacbbcbabEaabaacccccbccbcbabacabbbcbbcbacccbabcabaccaabaaabbabcaababaabacbccbbccbaccccaccbcbbab",
		"cbcaccbcaaaacccbaacaaccc3cbccbbbcaacbbcbabEaabaacccccbccbcbabacabbbcbbcbacccbabcabaccaabaaabbabcaababaabacbccbbccbaccccaccXIbf3A");

	RUN_TEST(7, "bcbbab", "WJbf3A",
		"cbcaccbcaaaacccbaacaaccc3cbccbbbcaacbbcbabEaabaacccccbccbcbabacabbbcbbcbacccbabcabaccaabaaabbabcaababaabacbccbbccbaccccaccbbcbba",
		"cbcaccbcaaaacccbaacaaccc3cbccbbbcaacbbcbabEaabaacccccbccbcbabacabbbcbbcbacccbabcabaccaabaaabbabcaababaabacbccbbccbaccccaccbbcbba");

	RUN_TEST(8, "bcbbab", "XJbf2A",
		"bcbbacbcaccbcaaaacccbaacaaccc3cbccbbbcaacbbcbabEaabaacccccbccbcbabacabbbcbbcbacccbabcabaccaabaaabbabcaababaabacbccbbccbaccccaccb",
		"bcbbacbcaccbcaaaacccbaacaaccc3cbccbbbcaacbbcbabEaabaacccccbccbcbabacabbbcbbcbacccbabcabaccaabaaabbabcaababaabacbccbbccbaccccaccb");

	RUN_TEST(9, "kokos", "XJbf4",
		"xxxkokofxxxkokosnussxxxkokokoxxxxxxkokofxxxkokosnussxxxkokokoxxxxxxkokofxxxkokosnussxxxkokokoxxxxxxkokofxxxkokosnussxxxkokokoxxx",
		"xxxkokofxxxXJbf4nussxxxkokokoxxxxxxkokofxxxXJbf4nussxxxkokokoxxxxxxkokofxxxXJbf4nussxxxkokokoxxxxxxkokofxxxXJbf4nussxxxkokokoxxx");

	return success;
}

#endif /*INC_SELFTEST_H*/
