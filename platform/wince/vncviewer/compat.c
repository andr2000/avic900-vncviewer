#include <stdio.h>
#include <tchar.h>
#include <winbase.h>

#include "compat.h"
#include "getopt.h"

char *
strerror(int errno)
{
	static char buf[32];
	/* TODO: get error string */
	sprintf(buf, "error %d", errno);
	return buf;
}

void
args_to_asciiz(LPTSTR _lpCmdLine, int *argc, char ***argv)
{
	char* arg;
	int index;
	/* TODO: these statics look weird, find a better solution */
	static char lpCmdLine[MAX_PATH + 1];
	static char exe_name[MAX_PATH + 1];
	_TCHAR filename[MAX_PATH + 1];

	wcstombs(lpCmdLine, _lpCmdLine, sizeof(lpCmdLine));
	*argc = 1;
	arg = lpCmdLine;

	while (arg[0] != 0) {
		while (arg[0] != 0 && arg[0] == ' ') {
			arg++;
		}
		if (arg[0] != 0) {
			argc++;
			while (arg[0] != 0 && arg[0] != ' ') {
				arg++;
			}
		}
	}
	/* tokenize the arguments */
	*argv = (char **)malloc(*argc * sizeof(char *));
	arg = lpCmdLine;
	index = 1;
	while (arg[0] != 0) {
		while (arg[0] != 0 && arg[0] == ' ') {
			arg++;
		}
		if (arg[0] != 0) {
			*argv[index] = arg;
			index++;
			while (arg[0] != 0 && arg[0] != ' ') {
				arg++;
			}
			if (arg[0] != 0) {
				arg[0] = 0;
				arg++;
			}
		}
	}
	/* put the program name into argv[0] */
	GetModuleFileName(NULL, filename, MAX_PATH);
	wcstombs(exe_name, filename, sizeof(exe_name));
	*argv[0] = exe_name;
}
