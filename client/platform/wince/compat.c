#include <stdio.h>

#include "compat.h"

#ifdef WINCE
char *
strerror(int errno)
{
	static char buf[32];
	/* TODO: get error string */
	sprintf(buf, "error %d", errno);
	return buf;
}
#endif
