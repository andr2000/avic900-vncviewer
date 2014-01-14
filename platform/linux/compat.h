#ifndef _COMPAT_H
#define _COMPAT_H

#if(defined __cplusplus)
extern "C"
{
#endif

#ifndef MAX_PATH
#define MAX_PATH 4096
#endif

char *get_exe_name(void);

#if(defined __cplusplus)
}
#endif

#endif /* _COMAPT_H */
