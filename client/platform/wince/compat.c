#include <stdio.h>
#include <tchar.h>
#include <winbase.h>

#include "compat.h"

char *
strerror(int errno)
{
	static char buf[32];
	/* TODO: get error string */
	sprintf(buf, "error %d", errno);
	return buf;
}
