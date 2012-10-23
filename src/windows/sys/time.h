#include <time.h>
// A version of timersub for use on Windows (under mingw-w64).
// Adapted from Hannes Pouseele
#define timersub(a, b, res) 							\
	(res)->tv_sec = ((a)->tv_sec - (b)->tv_sec); 		\
	(res)->tv_usec = ((a)->tv_usec - (b)->tv_usec); 	\
	if ((res)->tv_usec < 0) {							\
		(res)->tv_sec--;								\
		(res)->tv_usec += 1000000;						\
	}
