Pipe-utils, by LoRd_MuldeR <MuldeR2@GMX.de>
This work has been released under the CC0 1.0 Universal license!

mkpipe - connect N processes via pipe(s), with configurable pipe buffer size
pv     - measure the throughput of a pipe and the amount of data transferred
rand   - fast generator of pseudo-random bytes, using "xorwow" method

Example:
  set MKPIPE_BUFFSIZE=1048576
  mkpipe.exe rand.exe "|" pv.exe ">" NUL
