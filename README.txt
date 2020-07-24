Replace, by LoRd_MuldeR <MuldeR2@GMX.de>
This work has been released under the CC0 1.0 Universal license!

Replaces all occurences of '<needle>' in '<input_file>' with '<replacement>'.
The modified contents are then written to '<output_file>'.

Usage:
   replace.exe <needle> <replacement> [<input_file>] [<output_file>]

Please note:
1. If *only* '<input_file>' is specified, the file will be modified in-place!
2. If both file names are omitted, reads from STDIN and writes to STDOUT.
3. Either file can be specified as "-" to read from STDIN or write to STDOUT.
