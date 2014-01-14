#include "compat.h"

extern char *g_exe_name;

char * get_exe_name(void) {
	return g_exe_name;
}
