Replace, by LoRd_MuldeR <MuldeR2@GMX.de>

Replaces any occurrence of '<needle>' in '<input_file>' with '<replacement>'.
The modified contents are then written to '<output_file>'.

Usage:
  replace.exe [options] <needle> <replacement> [<input_file>] [<output_file>]

Options:
  -i  Perform case-insensitive matching for the characters 'A' to 'Z'
  -s  Single replacement; replace only the *first* occurrence instead of all
  -a  Process input using ANSI codepage (CP-1252) instead of UTF-8
  -e  Enable interpretation of backslash escape sequences in all parameters
  -f  Force immediate flushing of file buffers (may degrade performance)
  -b  Binary mode; parameters '<needle>' and '<replacement>' are Hex strings
  -y  Try to overwrite read-only files; i.e. clears the read-only flag
  -v  Enable verbose mode; print additional diagnostic information to STDERR
  -t  Run self-test and exit
  -h  Display this help text and exit

Notes:
  1. If *only* an '<input_file>' is specified, the file is modified in-place!
  2. If file names are omitted, reads from STDIN and writes to STDOUT.
  3. File name can be specified as "-" to read from STDIN or write to STDOUT.
  4. The length of a Hex string must be *even*; with optional '0x' prefix.

Examples:
  replace.exe "foobar" "quux" "input.txt" "output.txt"
  replace.exe -e "foo\nbar" "qu\tux" "input.txt" "output.txt"
  replace.exe "foobar" "quux" "modified.txt"
  replace.exe -b 0xDEADBEEF 0xCAFEBABE "input.bin" "output.bin"
  type "from.txt" | replace.exe "foo" "bar" > "to.txt"
