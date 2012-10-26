Velvet 1.2.08 for Windows
=========================

Source Code Modifications
-------------------------

Velvet was designed for compilation under Unix/POSIX-compliant systems.  To compile it under Windows, some changes to its source code were made:

1.	MinGW is missing the POSIX standard library function `timersub`, which subtracts two time values, from the system header file `<sys/time.h>`.  A custom implementation via a macro was provided (`windows/sys/time.h`), and

		#include "windows/sys/time.h"
		
was added to each source file in which `timersub` is used, namely

	graphReConstruction.c
	scaffold.c
	splayTable.c
	utility.c

2.	MinGW lacks the `sysconf` function, which is used to get the values of system environment variables at runtime.  `allocArray.c` uses `sysconf` to detect the OS virtual memory page size.  The call to `sysconf` was replaced on Windows with a custom `get_pagesize()` function, declared in `windows/pagesize.h` and implemented in `windows/pagesize.c`, which calls Windows' `GetNativeSystemInfo()` function.  `windows/pagesize.h` was included in `allocArray.c`
3.	`autoOpen.c` contains many Unix-specific API calls.  The implementations of functions `openFileAuto()` and `closeFileAuto()` were replaced with simpler versions that do not use these Unix-specific features.  As a side effect, automatic decompression of compressed fastA/Q files is no longer supported.
4.	In `run.c`, there is a conditional inclusion of `<uce-dirent.h>` on Windows and `<dirent.h>` on UNIX.  Since MinGW provides `<dirent.h>`, this inclusion was fixed so that the correct header is included on MinGW.
5.	In `run.c`, around line 225, the Unix API call `mkdir(directory, 0777)` was replaced by the corresponding Windows API call, `_mkdir(directory)`.
6.	Velvet contained Windows-specific conditional includes of `zlib.h` in `binarySequences.c` and `readSet.c`:

		#if !defined(BUNDLEDZLIB)
		#include <zlib.h>
		#elif defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
		#include "../third-party/zlib-1.2.3/Win32/include/zlib.h"
		#else
		#include "../third-party/zlib-1.2.3/zlib.h"
		#endif
		
	The Windows-specific path to `zlib.h` above is incorrect, and `zlib` can be easily installed as a part of MinGW, so use of the bundled library is unnecessary under MinGW.  Hence, the conditional inclusion was simplified to

		#if !defined(BUNDLEDZLIB)
		#include <zlib.h>
		#else
		#include "../third-party/zlib-1.2.3/zlib.h"
		#endif

Compilation Settings
--------------------

The Velvet `Makefile` was modified to ease multi-platform compilation.  In particular:

1.	Several long lists of object files to be linked were factored out into macro variables so that OS-specific object files could be added.
2.	Windows-specific object files were added to these lists (on Windows only).
3.	CFLAGS were modified so that 32-bit and 64-bit compilations can be explicitly invoked by defining either `MAKE32` or `MAKE64` on the command line.