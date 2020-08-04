#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

#define BUFFSIZE (8192U / sizeof(DWORD))
#define BUFFSIZE_BYTES (sizeof(DWORD) * BUFFSIZE)

static DWORD buffer[BUFFSIZE];

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

static __forceinline DWORD random_next(random_t *const state)
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

static int _main(void)
{
	random_t state;
	DWORD bytes_written = 0U;

	const HANDLE std_out = GetStdHandle(STD_OUTPUT_HANDLE);
	if (std_out == INVALID_HANDLE_VALUE)
	{
		return 1;
	}

	random_seed(&state);

	for(;;)
	{
		for (DWORD i = 0U; i < BUFFSIZE; ++i)
		{
			buffer[i] = random_next(&state);
		}
		for (DWORD pos = 0U; pos < BUFFSIZE_BYTES; pos += bytes_written)
		{
			if (!WriteFile(std_out, ((BYTE*)buffer) + pos, BUFFSIZE_BYTES - pos, &bytes_written, NULL))
			{
				goto exit_loop;
			}
			if (bytes_written < 1U)
			{
				goto exit_loop;
			}
		}
	}

exit_loop:

	return 0;
}

void startup(void)
{
	SetErrorMode(SetErrorMode(0x3) | 0x3);
	ExitProcess(_main());
}
