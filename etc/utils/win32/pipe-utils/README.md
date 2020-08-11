**Pipe-utils, by LoRd_MuldeR &lt;MuldeR2@GMX.de&gt;**  
This work has been released under the CC0 1.0 Universal license!

<https://github.com/lordmulder/pipe-utils>

---

    mkpipe, by LoRd_MuldeR <MuldeR2@GMX.de>
    
    Connect N processes via pipe(s), with configurable pipe buffer size.
    
    Usage:
       mkpipe.exe ["<" infile] <command_1> "|" ... "|" <command_n> [">" outfile]

    Examples:
       mkpipe.exe program1.exe -foo "|" program2.exe -bar
       mkpipe.exe "<" in.txt program1.exe -foo "|" program2.exe -bar ">" out.txt
    
    Use the environment variable MKPIPE_BUFFSIZE to override the buffer size.
    Default buffer size, if not specified, is 1048576 bytes.
    
    The operators "|", "<" and ">" must be *quoted* when running from the shell!
    Otherwise, the shell (e.g. cmd.exe) itself interprets these operators.

---

    pv, by LoRd_MuldeR <MuldeR2@GMX.de>
    
    Measure the throughput of a pipe and the amount of data transferred.
    Set environment variable PV_FORCE_NOWAIT=1 to force "async" mode.

---

    rand, by LoRd_MuldeR <MuldeR2@GMX.de>
    
    Fast generator of pseudo-random bytes, using the "xorwow" method.
    Output has been verified to pass the Dieharder test suite.
