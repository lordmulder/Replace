/******************************************************************************/
/* Replace, by LoRd_MuldeR <MuldeR2@GMX.de>                                   */
/* This work has been released under the CC0 1.0 Universal license!           */
/******************************************************************************/

#ifndef INC_SELFTEST_H
#define INC_SELFTEST_H

#include "libreplace/replace.h"
#include "utils.h"

/* ======================================================================= */
/* Run a single test                                                       */
/* ======================================================================= */

#define RUN_TEST(X, ...) do \
{ \
	if(run_test(__VA_ARGS__)) \
	{ \
		print_text_fmt(log_output, "[Self-Test] Test case #%02ld succeeded.\n", (LONG)(X)); \
	} \
	else \
	{ \
		print_text_fmt(log_output, "[Self-Test] Test case #%02ld failed !!!\n", (LONG)(X)); \
		success = FALSE; \
	} \
	if(g_abort_requested) \
	{ \
		return FALSE; /*aborted by user*/ \
	} \
} \
while(0)

static BOOL run_test(const BOOL dry_run, const BOOL case_insensitive, const BOOL globbing, const CHAR *const needle, const CHAR *const replacement, const CHAR *const haystack, const CHAR *const expected)
{
	BOOL success = FALSE;
	memory_input_t input_context;
	memory_output_t *output_context = NULL;
	libreplace_io_t io_functions;
	libreplace_flags_t options;
	WORD *needle_expanded = NULL;
	DWORD replacement_count = 0U;

	const DWORD needle_len      = lstrlenA(needle);
	const DWORD replacement_len = lstrlenA(replacement);
	const DWORD expected_len    = lstrlenA(expected);

	init_memory_input(&input_context, (const BYTE*)haystack, lstrlenA(haystack));
	SecureZeroMemory(&options, sizeof(libreplace_flags_t));

	options.dry_run = dry_run;
	options.case_insensitive = case_insensitive;

	if(!(output_context = alloc_memory_output(expected_len + 2U)))
	{
		goto cleanup;
	}

	needle_expanded = expand_wildcards((BYTE*)needle, needle_len, globbing ? &MY_WILDCARD : NULL);
	if(!needle_expanded)
	{
		goto cleanup;
	}

	init_io_functions(&io_functions, memory_read_byte, memory_write_byte, (DWORD_PTR)&input_context, (DWORD_PTR)output_context);

	if(!libreplace_search_and_replace(&io_functions, NULL, needle_expanded, needle_len, (const BYTE*)replacement, replacement_len, &options, &replacement_count, &g_abort_requested))
	{
		goto cleanup;
	}
	
	if(output_context->flushed == expected_len)
	{
		success = (lstrcmpA((LPCSTR)output_context->buffer, expected) == 0L);
	}

cleanup:

	if(output_context)
	{
		LocalFree((HLOCAL)output_context);
	}

	if(needle_expanded)
	{
		LocalFree((HLOCAL)needle_expanded);
	}

	return success;
}

/* ======================================================================= */
/* Self-test                                                               */
/* ======================================================================= */

static BOOL self_test(const HANDLE log_output)
{
	BOOL success = TRUE;

	RUN_TEST( 1, FALSE, FALSE, FALSE, "LTs3kx", "XJbf4A", "",       ""      );
	RUN_TEST( 2, FALSE, FALSE, FALSE, "LTs3kx", "XJbf4A", "LTs3k",  "LTs3k" );
	RUN_TEST( 3, FALSE, FALSE, FALSE, "LTs3kx", "XJbf4A", "LTs3kx", "XJbf4A");

	RUN_TEST( 4, FALSE, FALSE, FALSE, "LTs3kx", "XJbf3A",
		"KJnsbsniWReHocwWghHKmtwue7zLXvT9Ai3twkgmHRahFxTV3EggbHptv7toJhdKWCyJ93vPmUqXVtwCuJvpvY9Avu4cojuRwknv7HCYpyNvzJWtdwvEEpsNNyq9JAay",
		"KJnsbsniWReHocwWghHKmtwue7zLXvT9Ai3twkgmHRahFxTV3EggbHptv7toJhdKWCyJ93vPmUqXVtwCuJvpvY9Avu4cojuRwknv7HCYpyNvzJWtdwvEEpsNNyq9JAay");

	RUN_TEST( 5, FALSE, FALSE, FALSE, "LTs3kx", "XJbf3B",
		"KJnsbsniWReHocwWghHKmtwue7zLXvT9Ai3twkgmHRahFxTV3EggbHpLTs3kxhdKWCyJ93vPmUqXVtwCuJvpvY9Avu4cojuRwknv7HCYpyNvzJWtdwvEEpsNNyq9JAay",
		"KJnsbsniWReHocwWghHKmtwue7zLXvT9Ai3twkgmHRahFxTV3EggbHpXJbf3BhdKWCyJ93vPmUqXVtwCuJvpvY9Avu4cojuRwknv7HCYpyNvzJWtdwvEEpsNNyq9JAay");

	RUN_TEST( 6, FALSE, FALSE, FALSE, "LTs3kx", "",
		"KJnsbsniWReHocwWghHKmtwue7zLXvT9Ai3twkgmHRahFxTV3EggbHpLTs3kxhdKWCyJ93vPmUqXVtwCuJvpvY9Avu4cojuRwknv7HCYpyNvzJWtdwvEEpsNNyq9JAay",
		"KJnsbsniWReHocwWghHKmtwue7zLXvT9Ai3twkgmHRahFxTV3EggbHphdKWCyJ93vPmUqXVtwCuJvpvY9Avu4cojuRwknv7HCYpyNvzJWtdwvEEpsNNyq9JAay");

	RUN_TEST( 7, FALSE, FALSE, FALSE, "LTs3kx", "H4n3zWoHKfbX",
		"KJnsbsniWReHocwWghHKmtwue7zLXvT9Ai3twkgmHRahFxTV3EggbHpLTs3kxhdKWCyJ93vPmUqXVtwCuJvpvY9Avu4cojuRwknv7HCYpyNvzJWtdwvEEpsNNyq9JAay",
		"KJnsbsniWReHocwWghHKmtwue7zLXvT9Ai3twkgmHRahFxTV3EggbHpH4n3zWoHKfbXhdKWCyJ93vPmUqXVtwCuJvpvY9Avu4cojuRwknv7HCYpyNvzJWtdwvEEpsNNyq9JAay");

	RUN_TEST( 8, FALSE, FALSE, FALSE, "ababaa", "YJbg3A",
		"aaabaaaabbbaaabbaaaabaaaaabbabbaaaa3aabbbabbaabbbbabbabbbbbbbaabaaaabbaaabbbaaabbbaaaaababaaaaabaaabababaabbabbbabaabaAabaaabbaa",
		"aaabaaaabbbaaabbaaaabaaaaabbabbaaaa3aabbbabbaabbbbabbabbbbbbbaabaaaabbaaabbbaaabbbaaaaYJbg3AaaabaaabYJbg3AbbabbbabaabaAabaaabbaa");

	RUN_TEST( 9, FALSE, FALSE, FALSE, "abcabd", "XJcf3A",
		"abbaccddbcccacbddabcabcdacdbcabcdcccbcdcadbdddcabbcadcdccbabaabacccccabcababcabddbcbbcaadccab4dbaddbdccbdcdbcXccbbbcabbaabdcadccd",
		"abbaccddbcccacbddabcabcdacdbcabcdcccbcdcadbdddcabbcadcdccbabaabacccccabcabXJcf3Adbcbbcaadccab4dbaddbdccbdcdbcXccbbbcabbaabdcadccd");
	
	RUN_TEST(10, FALSE, FALSE, FALSE, "bcbbab", "XIbf3A",
		"cbcaccbcaaaacccbaacaaccc3cbccbbbcaacbbcbabEaabaacccccbccbcbabacabbbcbbcbacccbabcabaccaabaaabbabcaababaabacbccbbccbaccccaccbcbbab",
		"cbcaccbcaaaacccbaacaaccc3cbccbbbcaacbbcbabEaabaacccccbccbcbabacabbbcbbcbacccbabcabaccaabaaabbabcaababaabacbccbbccbaccccaccXIbf3A");

	RUN_TEST(11, FALSE, FALSE, FALSE, "bcbbab", "WJbf3A",
		"cbcaccbcaaaacccbaacaaccc3cbccbbbcaacbbcbabEaabaacccccbccbcbabacabbbcbbcbacccbabcabaccaabaaabbabcaababaabacbccbbccbaccccaccbbcbba",
		"cbcaccbcaaaacccbaacaaccc3cbccbbbcaacbbcbabEaabaacccccbccbcbabacabbbcbbcbacccbabcabaccaabaaabbabcaababaabacbccbbccbaccccaccbbcbba");

	RUN_TEST(12, FALSE, FALSE, FALSE, "bcbbab", "XJbf2A",
		"bcbbacbcaccbcaaaacccbaacaaccc3cbccbbbcaacbbcbabEaabaacccccbccbcbabacabbbcbbcbacccbabcabaccaabaaabbabcaababaabacbccbbccbaccccaccb",
		"bcbbacbcaccbcaaaacccbaacaaccc3cbccbbbcaacbbcbabEaabaacccccbccbcbabacabbbcbbcbacccbabcabaccaabaaabbabcaababaabacbccbbccbaccccaccb");

	RUN_TEST(13, FALSE, FALSE, FALSE, "kokos", "XJbf4",
		"xxxkokofxxxkokosnussxxxkokokoxxxxxxkokofxxxkokosnussxxxkokokoxxxxxxkokofxxxkokosnussxxxkokokoxxxxxxkokofxxxkokosnussxxxkokokoxxx",
		"xxxkokofxxxXJbf4nussxxxkokokoxxxxxxkokofxxxXJbf4nussxxxkokokoxxxxxxkokofxxxXJbf4nussxxxkokokoxxxxxxkokofxxxXJbf4nussxxxkokokoxxx");

	RUN_TEST(14, TRUE, TRUE, FALSE, "caa", "XjQ",
		"7ccCAbbCACcAbAcbcbAbCbaCAbbbcAAbcWCibcCaACabCabCcCAbacAcAAcaCCbCbCcCCccbaaAaCAaaAcbCCCcAbaAcccAaAAbCcCCCAabbCACccCCCAcCacAAcCccC",
		"7ccCAbbCACcAbAcbcbAbCbaCAbbbcAAbcWCibcCaACabCabCcCAbacAcAAcaCCbCbCcCCccbaaAaCAaaAcbCCCcAbaAcccAaAAbCcCCCAabbCACccCCCAcCacAAcCccC");

	RUN_TEST(15, FALSE, TRUE, FALSE, "caa", "XjQ",
		"7ccCAbbCACcAbAcbcbAbCbaCAbbbcAAbcWCibcCaACabCabCcCAbacAcAAcaCCbCbCcCCccbaaAaCAaaAcbCCCcAbaAcccAaAAbCcCCCAabbCACccCCCAcCacAAcCccC",
		"7ccCAbbCACcAbAcbcbAbCbaCAbbbXjQbcWCibcXjQCabCabCcCAbacAXjQcaCCbCbCcCCccbaaAaXjQaAcbCCCcAbaAccXjQAAbCcCCXjQbbCACccCCCAcCaXjQcCccC");

	RUN_TEST(16, FALSE, FALSE, TRUE, "a?c", "Jbf",
		"cabbababbbbcabcacacbabbbbbbccaacabbbcbccbabcbbbccaabbcbbccbcccbbabccaabbbbbbbcbabcaccabcbaccbaaccbcbacbabbbcacaccaccaaacacaabaac",
		"cabbababbbbcJbfacacbabbbbbbccJbfabbbcbccbJbfbbbccaabbcbbccbcccbbJbfcaabbbbbbbcbJbfJbfJbfbJbfbJbfcbcbacbabbbcacJbfJbfaJbfacaabJbf");

	return success;
}

#endif /*INC_SELFTEST_H*/
