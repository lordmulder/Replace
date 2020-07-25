Replace, by LoRd_MuldeR <MuldeR2@GMX.de>

Replaces all occurences of '<needle>' in '<input_file>' with '<replacement>'.
The modified contents are then written to '<output_file>'.

Usage:
  replace.exe [options] <needle> <replacement> [<input_file>] [<output_file>]

Options:
  -i  Perform case-insensitive matching for the characters 'A' to 'Z'
  -s  Single replacement; replace only the *first* occurence instead of all
  -a  Process input using ANSI codepage (CP-1252) instead of UTF-8
  -f  Force immediate flushing of output buffers (may degrade performance)
  -h  Display this help and exit

Notes:
  1. If *only* an '<input_file>' is specified, the file is modified in-place!
  2. If file names are omitted, reads from STDIN and writes to STDOUT.
  3. File name can be specified as "-" to read from STDIN or write to STDOUT.

Examples:
  replace.exe "foo" "bar" "input.txt" "output.txt"
  replace.exe "foo" "bar" "modify-me.txt"
  type "input.txt" | replace.exe "foo" "bar" > "output.txt"
